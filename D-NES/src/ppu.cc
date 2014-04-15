/*
 * ppu.cc
 *
 *  Created on: Mar 22, 2014
 *      Author: Dale
 */
#include "ppu.h"

PPU::PPU(VRAM* vram):
	vram(vram),
	ppuctrl(0),
	ppumask(0),
	ppustatus(0),
	oamaddr(0),
	ppuscroll(0),
	ppuaddr(0),
	tempaddr(0),
	finex(0),
	addrlatch(0),
	primary(new char[256]),
	secondaryOAM(new char[32]),
	OAMCounter(0),
	even(true),
	scanline(0),
	dot(0),
	bgBitmapLow(0),
	bgBitmapHigh(0),
	spBitmapLow(new char[8]),
	spBitmapHigh(new char[8]),
	spAttribute(new char[8]),
	spXPos(new char[8]),
	spriteCounter(0),
	patternTableAddr(0),
	activeAttribute(0),
	tileBitmapLow(0),
	tileBitmapHigh(0),
	buffer(new char[0x3F00])
{}

int PPU::Run(int cyc)
{
	while (cyc > 0)
	{
		bool rendering = true;

		if (((ppumask & 0x18) >> 3) != 0)
		{
			rendering = false;
		}

		// *************************************************************************************************
		// End VBLANK and Start Rendering
		// *************************************************************************************************
		if (scanline >= -1 && scanline <= 239) // Visible scan lines
		{
			if (scanline == -1)
			{
				ppustatus = ppustatus & 0x7f; // Clear vblank
			}

			// *************************************************************************************************
			// Render Visible Pixels and Evaluate Sprites
			// *************************************************************************************************
			if (dot >= 1 && dot <= 256 && rendering)
			{
				if (((dot - 1) % 8) == 0) // Nametable byte fetch
				{
					patternTableAddr = vram->read(0x2000 | (ppuaddr & 0x0FFF));

					if (dot != 1) // reload shift registers
					{
						bgBitmapLow = bgBitmapLow | (((short int) tileBitmapHigh) << 8);
						bgBitmapHigh = bgBitmapHigh | (((short int) tileBitmapLow) << 8);
					}
				}
				else if (((dot - 1) % 8) == 2) // Attribute byte fetch
				{
					char temp = vram->read(0x23C0 | (ppuaddr & 0x0C00) | ((ppuaddr >> 4) & 0x38) | ((ppuaddr >> 2) & 0x07));
					short int coarseX = ppuaddr & 0x001F;
					short int coarseY = (ppuaddr & 0x03E0) >> 5;

					// Use Coarse X and Coarse Y to determine correct attribute
					// Quadrant 1 = Bits 0 and 1
					// Quadrant 2 = Bits 2 and 3
					// Quadrant 3 = Bits 4 and 5
					// Quadrant 4 = Bits 6 and 7

					if ((coarseX % 4) <= 1 && (coarseY % 4) <= 1)
					{
						activeAttribute = temp & 0x03;
					}
					else if ((coarseX % 4) <= 1 && (coarseY % 4) > 1)
					{
						activeAttribute = (temp & 0x0C) >> 2;
					}
					else if ((coarseX % 4) > 1 && (coarseY % 4) <= 1)
					{
						activeAttribute = (temp & 0x20) >> 4;
					}
					else if ((coarseX % 4) > 1 && (coarseY % 4) > 1)
					{
						activeAttribute = (temp & 0xC0) >> 6;
					}
				}
				else if (((dot - 1) % 8) == 4) // fetch bitmap low byte
				{
					if (((ppuctrl & 0x10) >> 4) == 0)
					{
						tileBitmapLow = vram->read(patternTableAddr * 16);
					}
					else
					{
						tileBitmapLow = vram->read((patternTableAddr * 16) | 0x1000);
					}
				}
				else if (((dot - 1) % 8) == 6) // fetch bitmap high byte
				{
					if (((ppuctrl & 0x10) >> 4) == 0)
					{
						tileBitmapHigh = vram->read((patternTableAddr * 16) + 8);
					}
					else
					{
						tileBitmapHigh = vram->read(((patternTableAddr * 16) + 8) | 0x1000);
					}

					// Increment coarse X and switch nametable on overflow
					if ((ppuaddr & 0x001F) == 31)
					{
						ppuaddr = ppuaddr & 0xFFE0;
						ppuaddr = ppuaddr ^ 0x0400;
					}
					else
					{
						ppuaddr++;
					}
				}

				// Increment coarse Y
				if (dot == 256)
				{
					if ((ppuaddr & 0x7000) != 0x7000)	// if fine Y < 7
					{
					  ppuaddr += 0x1000;				// increment fine Y
					}
					else
					{
					  ppuaddr &= ~0x7000;				// fine Y = 0
					  int coarseY = (ppuaddr & 0x03E0) >> 5;	// let y = coarse Y
					  if (coarseY == 29)
					  {
						  coarseY = 0;                          // coarse Y = 0
					    ppuaddr ^= 0x0800;				// switch ppuaddrertical nametable
					  }
					  else if (coarseY == 31)
					  {
						  coarseY = 0;							// coarse Y = 0, nametable not switched
					  }
					  else
					  {
						  coarseY += 1;							// increment coarse Y
					  }
					  ppuaddr = (ppuaddr & ~0x03E0) | (coarseY << 5);     // put coarse Y back into ppuaddr
					}
					spriteCounter = 0;
					OAMCounter = 0;
				}

				if (scanline != -1)
				{
					render();
				}
			}

			// *************************************************************************************************
			// Load Sprites for the next scan line
			// *************************************************************************************************
			if (dot >= 257 && dot <= 320 && rendering && scanline != -1) // Load sprite registers from secondary OAM
			{
				if (((dot - 1) % 8) == 2) // load sprite attribute
				{
					spAttribute[spriteCounter] = secondaryOAM[(spriteCounter * 4) + 2];
				}
				else if (((dot - 1) % 8) == 3) // sprite X position counter
				{
					spXPos[spriteCounter] = secondaryOAM[(spriteCounter * 4) + 3];
				}
				else if (((dot - 1) % 8) == 4) // Low byte fetch
				{
					// if sprite size is 8x8 use ppuctrl page bit otherwise use
					// bit 1 of the index number to determine the page
					if (((ppuctrl & 0x20) >> 5) == 0)
					{
						if (((ppuctrl & 0x08) >> 3) == 0)
						{
							spBitmapLow[spriteCounter] = vram->read((secondaryOAM[(spriteCounter * 4) + 1] * 16) + 8);
						}
						else
						{
							spBitmapLow[spriteCounter] = vram->read(((secondaryOAM[(spriteCounter * 4) + 1] * 16) + 8) | 0x1000);
						}
					}
					else
					{
						char temp = secondaryOAM[(spriteCounter * 4) + 1];
						if ((temp & 0x01) == 0)
						{
							spBitmapLow[spriteCounter] = vram->read(temp >> 1);
						}
						else // high bit fetch
						{
							spBitmapLow[spriteCounter] = vram->read(0x1000 | (temp >> 1));
						}
					}
				}
				else if (((dot - 1) % 8) == 6) // high byte fetch
				{
					if (((ppuctrl & 0x20) >> 5) == 0)
					{
						if (((ppuctrl & 0x08) >> 3) == 0)
						{
							spBitmapHigh[spriteCounter] = vram->read(secondaryOAM[(spriteCounter * 8) + 8] * 16);
						}
						else
						{
							spBitmapHigh[spriteCounter] = vram->read((secondaryOAM[(spriteCounter * 8) + 8] * 16)| 0x1000);
						}
					}
					else
					{
						char temp = secondaryOAM[(spriteCounter * 4) + 1];
						if ((temp & 0x01) == 0)
						{
							spBitmapHigh[spriteCounter] = vram->read((temp >> 1) + 8);
						}
						else
						{
							spBitmapHigh[spriteCounter] = vram->read(0x1000 | ((temp >> 1) + 8));
						}
					}

					spriteCounter++;
				}
			}


			// *************************************************************************************************
			// Load First two tiles for next scan line
			// *************************************************************************************************
			if (dot >= 321 && dot <= 336 && rendering)
			{
				if (((dot - 1) % 8) == 0) // Nametable byte fetch
				{
					patternTableAddr = vram->read(0x2000 | (ppuaddr & 0x0FFF));

					if (dot != 321) // reload shift registers
					{
						// load low bites since rendering doesn't start until the next line
						bgBitmapLow = bgBitmapLow | ((short int) tileBitmapHigh);
						bgBitmapHigh = bgBitmapHigh | ((short int) tileBitmapLow);
					}
				}
				else if (((dot - 1) % 8) == 2) // Attribute byte fetch
				{
					char temp = vram->read(0x23C0 | (ppuaddr & 0x0C00) | ((ppuaddr >> 4) & 0x38) | ((ppuaddr >> 2) & 0x07));
					short int coarseX = ppuaddr & 0x001F;
					short int coarseY = (ppuaddr & 0x03E0) >> 5;

					// Use Coarse X and Coarse Y to determine correct attribute
					// Quadrant 1 = Bits 0 and 1
					// Quadrant 2 = Bits 2 and 3
					// Quadrant 3 = Bits 4 and 5
					// Quadrant 4 = Bits 6 and 7

					if ((coarseX % 4) <= 1 && (coarseY % 4) <= 1)
					{
						activeAttribute = temp & 0x03;
					}
					else if ((coarseX % 4) <= 1 && (coarseY % 4) > 1)
					{
						activeAttribute = (temp & 0x0C) >> 2;
					}
					else if ((coarseX % 4) > 1 && (coarseY % 4) <= 1)
					{
						activeAttribute = (temp & 0x20) >> 4;
					}
					else if ((coarseX % 4) > 1 && (coarseY % 4) > 1)
					{
						activeAttribute = (temp & 0xC0) >> 6;
					}

					// Increment coarse X and switch nametable on overflow
					if ((ppuaddr & 0x001F) == 31)
					{
						ppuaddr = ppuaddr & 0xFFE0;
						ppuaddr = ppuaddr ^ 0x0400;
					}
					else
					{
						ppuaddr++;
					}
				}
				else if (((dot - 1) % 8) == 4) // fetch bitmap low byte
				{
					if (((ppuctrl & 0x10) >> 4) == 0)
					{
						tileBitmapLow = vram->read(patternTableAddr * 16);
					}
					else
					{
						tileBitmapLow = vram->read((patternTableAddr * 16) | 0x1000);
					}
				}
				else if (((dot - 1) % 8) == 6) // fetch bitmap high byte
				{
					if (((ppuctrl & 0x10) >> 4) == 0)
					{
						tileBitmapHigh = vram->read((patternTableAddr * 16) + 8);
					}
					else
					{
						tileBitmapHigh = vram->read(((patternTableAddr * 16) + 8) | 0x1000);
					}
				}
			}

			// *************************************************************************************************
			// Dummy Name Table fetches
			// *************************************************************************************************
			if (dot >= 337 && dot <= 340 && rendering)
			{
				if (((dot - 1) % 8) == 0) // Nametable byte fetch
				{
					vram->read(0x2000 | (ppuaddr & 0x0FFF));

					// load high bytes for second tile of next scanline
					bgBitmapLow = bgBitmapLow | (((short int) tileBitmapHigh) << 8);
					bgBitmapHigh = bgBitmapHigh | (((short int) tileBitmapLow) << 8);
				}
				else if (((dot - 1) % 8) == 2)
				{
					vram->read(0x2000 | (ppuaddr & 0x0FFF));
				}
			}
		}

		// *************************************************************************************************
		// Begin VBLANK
		// *************************************************************************************************
		if (scanline == 241 && dot == 2) // vertical blanking lines
		{
			ppustatus = ppustatus | 0x80; // set vblank flag
			// implement NMI
		}
	}
}

void PPU::render() {}

void PPU::Reset() {}

unsigned char PPU::getPPUCTRL()
{
	return 0xFF;
}

unsigned char PPU::getPPUMASK()
{
	return 0xFF;
}

unsigned char PPU::getPPUSTATUS()
{
	addrlatch = 0;
	char temp = ppustatus;
	ppustatus = ppustatus & 0x7F;
	return temp;
}

unsigned char PPU::getOAMADDR()
{
	return 0xFF;
}

unsigned char PPU::getOAMDATA()
{
	return primary[(unsigned char) oamaddr];
}

unsigned char PPU::getPPUSCROLL()
{
	return 0xFF;
}

unsigned char PPU::getPPUADDR()
{
	return 0xFF;
}

unsigned char PPU::getPPUDATA()
{
	char temp;
	if (ppuaddr < 0x3F00)
	{
		temp = buffer[ppuaddr];
		buffer[ppuaddr] = vram->read(ppuaddr);
	}
	else
	{
		temp = vram->read(ppuaddr);
	}

	(((ppuctrl & 0x02) >> 1) == 0) ? ppuaddr += 1 : ppuaddr += 32;
	return temp;
}

void PPU::setPPUCTRL(unsigned char M)
{
	ppuctrl = M;
	tempaddr = tempaddr | (((short int) M) << 9);
}

void PPU::setPPUMASK(unsigned char M)
{
	ppumask = M;
}

void PPU::setOAMADDR(unsigned char M)
{
	oamaddr = M;
}

void PPU::setOAMDATA(unsigned char M)
{
	primary[(unsigned char) oamaddr] = M;
	oamaddr += 1;
}

void PPU::setPPUSCROLL(unsigned char M)
{
	if (addrlatch == 0)
	{
		finex = M;
		tempaddr = tempaddr | (((short int) M) >> 3);
		addrlatch = 1;
	}
	else
	{
		tempaddr = tempaddr | (((short int) (M & 0xFC)) << 2);
		tempaddr = tempaddr | (((short int) (M & 0x04)) << 12);
		addrlatch = 0;
	}
}

void PPU::setPPUADDR(unsigned char M)
{
	if (addrlatch == 0)
	{
		tempaddr = tempaddr | (((short int) (M & 0x3F)) << 8);
		addrlatch = 1;
	}
	else
	{
		tempaddr = tempaddr | ((short int) M);
		ppuaddr = tempaddr;
		addrlatch = 0;
	}
}

void PPU::setPPUDATA(unsigned char M)
{
	vram->write(M, ppuaddr);
}

PPU::~PPU()
{
	delete[] primary;
	delete[] secondaryOAM;
	delete[] buffer;
}

