/*
 * ppu.cc
 *
 *  Created on: Oct 9, 2014
 *      Author: Dale
 */

#include "ppu.h"
#include "nes.h"

const unsigned int PPU::rgbLookupTable[64] =
{
	0x545454, 0x001E74, 0x081090, 0x300088, 0x440064, 0x5C0030, 0x540400, 0x3C1800, 0x202A00, 0x083A00, 0x004000, 0x003C00, 0x00323C, 0x000000, 0x000000, 0x000000,
	0x989698, 0x084CC4, 0x3032EC, 0x5C1EE4, 0x8814B0, 0xA01464, 0x982220, 0x783C00, 0x545A00, 0x287200, 0x087C00, 0x007628, 0x006678, 0x000000, 0x000000, 0x000000,
	0xECEEEC, 0x4C9AEC, 0x787CEC, 0xB062EC, 0xE454EC, 0xEC58B4, 0xEC6A64, 0xD48820, 0xA0AA00, 0x74C400, 0x4CD020, 0x38CC6C, 0x38B4CC, 0x3C3C3C, 0x000000, 0x000000,
	0xECEEEC, 0xA8CCEC, 0xBCBCEC, 0xD4B2EC, 0xECAEEC, 0xECAED4, 0xECB4B0, 0xE4C490, 0xCCD278, 0xB4DE78, 0xA8E290, 0x98E2B4, 0xA0D6E4, 0xA0A2A0, 0x000000, 0x000000
};

void PPU::GetNameTable(int table, unsigned char* pixels)
{
	unsigned short int tableIndex;
	if (table == 0)
	{
		tableIndex = 0x000;
	}
	else if (table == 1)
	{
		tableIndex = 0x400;
	}
	else if (table == 2)
	{
		tableIndex = 0x800;
	}
	else
	{
		tableIndex = 0xC00;
	}

	for (unsigned int i = 0; i < 30; ++i)
	{
		for (unsigned int f = 0; f < 32; ++f)
		{
			unsigned short int address = (tableIndex | (i << 5) | f);
			unsigned char ntByte = ReadNameTable(0x2000 | address);
			unsigned char atShift = (((address & 0x0002) >> 1) | ((address & 0x0040) >> 5)) * 2;
			unsigned char atByte = ReadNameTable(0x23C0 | (address & 0x0C00) | ((address >> 4) & 0x38) | ((address >> 2) & 0x07));
			atByte = (atByte >> atShift) & 0x3;
			unsigned short int patternAddress = static_cast<unsigned short int>(ntByte) * 16;

			for (unsigned int h = 0; h < 8; ++h)
			{
				unsigned char tileLow = cart.ChrRead(baseBackgroundTableAddress + patternAddress + h);
				unsigned char tileHigh = cart.ChrRead(baseBackgroundTableAddress + patternAddress + h + 8);

				for (unsigned int g = 0; g < 8; ++g)
				{
					unsigned short int pixel = 0x0003 & ((((tileLow << g) & 0x80) >> 7) | (((tileHigh << g) & 0x80) >> 6));
					unsigned short int paletteIndex = 0x3F00 | (atByte << 2) | pixel;
					unsigned int rgb = rgbLookupTable[Read(paletteIndex)];
					unsigned char red = (rgb & 0xFF0000) >> 16;
					unsigned char green = (rgb & 0x00FF00) >> 8;
					unsigned char blue = (rgb & 0x0000FF);

					unsigned int index = ((24 * f) + (g * 3)) + ((6144 * i) + (h * 768));

					pixels[index] = red;
					pixels[index + 1] = green;
					pixels[index + 2] = blue;
				}
			}
		}
	}
}

void PPU::GetPatternTable(int table, int palette, unsigned char* pixels)
{
	unsigned short int tableIndex;
	if (table == 0)
	{
		tableIndex = 0x0000;
	}
	else
	{
		tableIndex = 0x1000;
	}

	for (unsigned int i = 0; i < 16; ++i)
	{
		for (unsigned int f = 0; f < 16; ++f)
		{
			unsigned int patternIndex = (16 * f) + (256 * i);

			for (unsigned int g = 0; g < 8; ++g)
			{
				unsigned char tileLow = cart.ChrRead(tableIndex + patternIndex + g);
				unsigned char tileHigh = cart.ChrRead(tableIndex + patternIndex + g + 8);

				for (unsigned int h = 0; h < 8; ++h)
				{
					unsigned short int pixel = 0x0003 & ((((tileLow << h) & 0x80) >> 7) | (((tileHigh << h) & 0x80) >> 6));
					unsigned short int paletteIndex = (0x3F00 + (4 * (palette % 8))) | pixel;
					unsigned int rgb = rgbLookupTable[Read(paletteIndex)];
					unsigned char red = (rgb & 0xFF0000) >> 16;
					unsigned char green = (rgb & 0x00FF00) >> 8;
					unsigned char blue = (rgb & 0x0000FF);

					unsigned int index = ((24 * f) + (h * 3)) + ((3072 * i) + (g * 384));

					pixels[index] = red;
					pixels[index + 1] = green;
					pixels[index + 2] = blue;
				}
			}
		}
	}
}

void PPU::GetPalette(int palette, unsigned char* pixels)
{
	unsigned short int baseAddress;
	if (palette == 0)
	{
		baseAddress = 0x3F00;
	}
	else if (palette == 1)
	{
		baseAddress = 0x3F04;
	}
	else if (palette == 2)
	{
		baseAddress = 0x3F08;
	}
	else if (palette == 3)
	{
		baseAddress = 0x3F0C;
	}
	else if (palette == 4)
	{
		baseAddress = 0x3F10;
	}
	else if (palette == 5)
	{
		baseAddress = 0x3F14;
	}
	else if (palette == 6)
	{
		baseAddress = 0x3F18;
	}
	else
	{
		baseAddress = 0x3F1C;
	}

	for (unsigned int i = 0; i < 4; ++i)
	{
		unsigned short int paletteAddress = baseAddress + i;

		for (unsigned int g = 0; g < 16; ++g)
		{
			for (unsigned int h = 0; h < 16; ++h)
			{
				unsigned int rgb = rgbLookupTable[Read(paletteAddress)];
				unsigned char red = (rgb & 0xFF0000) >> 16;
				unsigned char green = (rgb & 0x00FF00) >> 8;
				unsigned char blue = (rgb & 0x0000FF);

				unsigned int index = ((48 * i) + (h * 3)) + (192 * g);

				pixels[index] = red;
				pixels[index + 1] = green;
				pixels[index + 2] = blue;
			}
		}
	}
}

void PPU::GetPrimaryOAM(int sprite, unsigned char* pixels)
{
	unsigned char byteOne = primaryOAM[(0x4 * sprite) + 1];
	unsigned char byteTwo = primaryOAM[(0x4 * sprite) + 2];

	unsigned short int tableIndex = baseSpriteTableAddress; //(byteOne & 0x01) ? 0x1000 : 0x0000;
	unsigned short int patternIndex = byteOne; //(byteOne & 0xFE) >> 1;
	unsigned char palette = (byteTwo & 0xFC) + 4;

	for (unsigned int i = 0; i < 8; ++i)
	{
		unsigned char tileLow = cart.ChrRead(tableIndex + patternIndex + i);
		unsigned char tileHigh = cart.ChrRead(tableIndex + patternIndex + i + 8);

		for (unsigned int f = 0; f < 8; ++f)
		{
			unsigned short int pixel = 0x0003 & ((((tileLow << f) & 0x80) >> 7) | (((tileHigh << f) & 0x80) >> 6));
			unsigned short int paletteIndex = (0x3F00 + (4 * (palette % 8))) | pixel;
			unsigned int rgb = rgbLookupTable[Read(paletteIndex)];
			unsigned char red = (rgb & 0xFF0000) >> 16;
			unsigned char green = (rgb & 0x00FF00) >> 8;
			unsigned char blue = (rgb & 0x0000FF);

			unsigned int index = (24 * i) + (f * 3);

			pixels[index] = red;
			pixels[index + 1] = green;
			pixels[index + 2] = blue;
		}
	}
}

void PPU::GetSecondaryOAM(int sprite, unsigned char* pixels)
{
	unsigned char byteOne = secondaryOAM[(0x4 * sprite) + 1];
	unsigned char byteTwo = secondaryOAM[(0x4 * sprite) + 2];

	unsigned short int tableIndex = baseSpriteTableAddress; //(byteOne & 0x01) ? 0x1000 : 0x0000;
	unsigned short int patternIndex = byteOne;//(byteOne & 0xFE) >> 1;
	unsigned char palette = (byteTwo & 0xFC) + 4;

	for (unsigned int i = 0; i < 8; ++i)
	{
		unsigned char tileLow = cart.ChrRead(tableIndex + patternIndex + i);
		unsigned char tileHigh = cart.ChrRead(tableIndex + patternIndex + i + 8);

		for (unsigned int f = 0; f < 8; ++f)
		{
			unsigned short int pixel = 0x0003 & ((((tileLow << f) & 0x80) >> 7) | (((tileHigh << f) & 0x80) >> 6));
			unsigned short int paletteIndex = (0x3F00 + (4 * (palette % 8))) | pixel;
			unsigned int rgb = rgbLookupTable[Read(paletteIndex)];
			unsigned char red = (rgb & 0xFF0000) >> 16;
			unsigned char green = (rgb & 0x00FF00) >> 8;
			unsigned char blue = (rgb & 0x0000FF);

			unsigned int index = (24 * i) + (f * 3);

			pixels[index] = red;
			pixels[index + 1] = green;
			pixels[index + 2] = blue;
		}
	}
}

void PPU::Tick()
{
	UpdateState(1);
	bool renderingEnabled = showSprites || showBackground;

	if ((line >= 0 && line <= 239) || line == 261) // Visible Scanlines (also pre-render scanline)
	{
		if (dot == 1 && line == 261)
		{
			inVBLANK = false;
			nmiOccured = false;
			sprite0Hit = false;
		}

		if (dot >= 1 && dot <= 256 && line != 261 && !renderingEnabled)
		{
			display.NextPixel(rgbLookupTable[Read(0x3F00)]);
		}

		if (((dot >= 1 && dot <= 256) || (dot >= 321 && dot <= 336)) && renderingEnabled)
		{
			unsigned char cycle = (dot - 1) % 8; // The current point in the background fetch cycle

			// Reload background shift registers
			if (cycle == 0 && dot != 1)
			{
				backgroundShift0 |= tileBitmapLow;
				backgroundShift1 |= tileBitmapHigh;
				backgroundAttribute = attributeByte;
			}

			// Perform fetches
			if (cycle == 1)
			{
				nameTableByte = ReadNameTable(0x2000 | (ppuAddress & 0x0FFF)); // Get the Name Table byte, a pointer into the pattern table
			}
			else if (cycle == 3)
			{
				// Get the attribute byte, determines the palette to use when rendering
				attributeByte = ReadNameTable(0x23C0 | (ppuAddress & 0x0C00) | ((ppuAddress >> 4) & 0x38) | ((ppuAddress >> 2) & 0x07));
				unsigned char attributeShift = (((ppuAddress & 0x0002) >> 1) | ((ppuAddress & 0x0040) >> 5)) * 2;
				attributeByte = (attributeByte >> attributeShift) & 0x3;
			}
			else if (cycle == 5)
			{
				unsigned char fineY = ppuAddress >> 12; // Get fine y scroll bits from address
				unsigned short int patternAddress = static_cast<unsigned short int>(nameTableByte) * 16; // Get pattern address, independent of the table
				//unsigned short int baseAddress = spriteSize ? baseBackgroundTableAddress : 0; // Get base pattern table address
				tileBitmapLow = cart.ChrRead(baseBackgroundTableAddress + patternAddress + fineY); // Read pattern byte
			}
			else if (cycle == 7)
			{
				unsigned char fineY = ppuAddress >> 12; // Get fine y scroll
				unsigned short int patternAddress = static_cast<unsigned short int>(nameTableByte) * 16; // Get pattern address, independent of the table
				//spriteSize ? patternAddress += 8 : patternAddress += 16; // Offset to next half of pattern, sprites can have width 8 or 16
				//unsigned short int baseAddress = spriteSize ? baseBackgroundTableAddress : 0; // Get base pattern table address
				tileBitmapHigh = cart.ChrRead(baseBackgroundTableAddress + patternAddress + fineY + 8); // Read pattern byte
			}

			if (line != 261 && dot >= 1 && dot <= 256) // Evaluate Sprites and Render on all visible lines
			{
				Render();
			}

			// Every 8 dots increment the coarse X scroll
			if (cycle == 7)
			{
				if ((ppuAddress & 0x001F) == 31) // Reach end of Name Table
				{
					ppuAddress &= 0xFFE0; // Set Coarse X to 0
					ppuAddress ^= 0x400; // Switch horizontal Name Table
				}
				else
				{
					ppuAddress++; // Increment Coarse X
				}
			}

			// Exactly at dot 256 increment the Y scroll
			if (dot == 256)
			{
				if ((ppuAddress & 0x7000) != 0x7000) // if the fine Y < 7
				{
					ppuAddress += 0x1000; // increment fine Y
				}
				else
				{
					ppuAddress &= 0x8FFF; // Set fine Y to 0
					unsigned short int coarseY = (ppuAddress & 0x03E0) >> 5; // get coarse Y

					if (coarseY == 29) // End of name table
					{
						coarseY = 0; // set coarse Y to 0
						ppuAddress ^= 0x0800; // Switch Name Table
					}
					else if (coarseY == 31) // End of attribute table
					{
						coarseY = 0; // set coarse Y to 0
					}
					else
					{
						coarseY++; // Increment coarse &
					}

					ppuAddress = (ppuAddress & 0xFC1F) | (coarseY << 5); // Combine values into new address
				}
			}
		}
		else if (dot >= 257 && dot <= 320 && renderingEnabled)
		{
			oamAddress = 0;

			if (dot == 257 && line != 261)
			{
				SpriteEvaluation();
			}

			// Exactly at dot 257 copy all horizontal position bits to ppuAddress from ppuTempAddress
			if (dot == 257)
			{
				ppuAddress = (ppuAddress & 0x7BE0) | (ppuTempAddress & 0x041F);
			}

			// From dot 280 to 304 of the pre-render line copy all vertical position bits to ppuAddress from ppuTempAddress
			if (dot >= 280 && dot <= 304 && line == 261)
			{
				ppuAddress = (ppuAddress & 0x041F) | (ppuTempAddress & 0x7BE0);
			}

			unsigned char cycle = (dot - 257) % 8; // The current point in the sprite fetch cycle
			unsigned char sprite = (dot - 257) / 8;

			if (cycle == 2)
			{
				spriteAttribute[sprite] = secondaryOAM[(sprite * 4) + 2];
			}
			else if (cycle == 3)
			{
				spriteCounter[sprite] = secondaryOAM[(sprite * 4) + 3];
			}
			else if (cycle == 5)
			{
				if (sprite < spriteCount)
				{
					bool flipVertical = (0x80 & spriteAttribute[sprite]) >> 7;
					unsigned char frameY = (((ppuAddress & 0x03E0) >> 2) | ((ppuAddress & 0x7000) >> 12)); // The current Y position in the frame
					unsigned char spriteY = secondaryOAM[(sprite * 4)] + 1;

					if (spriteSize) // if spriteSize is 8x16
					{
						unsigned short int base = (0x1 & secondaryOAM[(sprite * 4) + 1]) ? 0x1000 : 0; // Get base pattern table from bit 0 of pattern address
						unsigned short int patternIndex = base + ((secondaryOAM[(sprite * 4) + 1] >> 1) * 16); // Index of the beginning of the pattern
						unsigned short int offset = flipVertical ? (spriteY + 15) - (frameY - spriteY) : (frameY - spriteY); // Offset from base index

						spriteShift0[sprite] = cart.ChrRead(patternIndex + offset);
					}
					else
					{
						unsigned short int patternIndex = baseSpriteTableAddress + secondaryOAM[(sprite * 4) + 1] * 16; // Index of the beginning of the pattern
						unsigned short int offset = flipVertical ? (spriteY + 7) - (frameY - spriteY) : (frameY - spriteY); // Offset from base index

						spriteShift0[sprite] = cart.ChrRead(patternIndex + offset);
					}
				}
				else
				{
					spriteShift0[sprite] = 0x00;
				}
			}
			else if (cycle == 7)
			{
				if (sprite < spriteCount)
				{
					bool flipVertical = (0x80 & spriteAttribute[sprite]) >> 7;
					unsigned char frameY = (((ppuAddress & 0x03E0) >> 2) | ((ppuAddress & 0x7000) >> 12)); // The current Y position in the frame
					unsigned char spriteY = secondaryOAM[(sprite * 4)] + 1;

					if (spriteSize) // if spriteSize is 8x16
					{
						unsigned short int base = (0x1 & secondaryOAM[(sprite * 4) + 1]) ? 0x1000 : 0; // Get base pattern table from bit 0 of pattern address
						unsigned short int patternIndex = base + ((secondaryOAM[(sprite * 4) + 1] >> 1) * 16); // Index of the beginning of the pattern
						unsigned short int offset = flipVertical ? (spriteY + 15) - (frameY - spriteY) : (frameY - spriteY); // Offset from base index

						spriteShift1[sprite] = cart.ChrRead(patternIndex + offset + 16);
					}
					else
					{
						unsigned short int patternIndex = baseSpriteTableAddress + secondaryOAM[(sprite * 4) + 1] * 16; // Index of the beginning of the pattern
						unsigned short int offset = flipVertical ? (spriteY + 7) - (frameY - spriteY) : (frameY - spriteY); // Offset from base index

						spriteShift1[sprite] = cart.ChrRead(patternIndex + offset + 8);
					}
				}
				else
				{
					spriteShift1[sprite] = 0x00;
				}
			}
		}
		else if (dot >= 337 && dot <= 340 && renderingEnabled)
		{
			if (dot == 337) // The final load of the background shifters occurs here
			{
				// These shifters haven't been changed since the last load, so shift them 8 places.
				backgroundShift0 = backgroundShift0 << 8;
				backgroundShift1 = backgroundShift1 << 8;

				backgroundShift0 |= tileBitmapLow;
				backgroundShift1 |= tileBitmapHigh;

				backgroundAttributeShift0 = 0;
				backgroundAttributeShift1 = 0;

				for (int i = 0; i < 8; ++i)
				{
					backgroundAttributeShift0 = backgroundAttributeShift0 | ((backgroundAttribute & 0x1) << i);
					backgroundAttributeShift1 = backgroundAttributeShift1 | (((backgroundAttribute & 0x2) >> 1) << i);
				}

				backgroundAttribute = attributeByte;
			}

			unsigned char cycle = (dot - 337) % 2;

			if (cycle == 1)
			{
				ReadNameTable(0x2000 | (ppuAddress & 0x0FFF));
			}
		}
	}
	else // VBlank and post-render scanline
	{
		if (dot == 1 && line == 241)
		{
			even = !even;
			inVBLANK = true;
			nmiOccured = true;

			if (nmiOccured && nmiEnabled)
			{
				nes.RaiseNMI();
			}
		}
	}

	// Increment Scanline pixel
	if (dot == 339 && line == 261 && !even && renderingEnabled)
	{
		dot = line = 0;
	}
	else if (dot == 340 && line == 261)
	{
		dot = line = 0;
	}
	else if (dot == 340)
	{
		dot = 0;
		++line;
	}
	else
	{
		++dot;
	}

	++clock;
}

void PPU::UpdateState(int cycles)
{

	while (cycles != 0)
	{
		if (ctrlBuffer.size() > 0 && ctrlBuffer.front().first == clock - cycles)
		{
			unsigned char value = ctrlBuffer.front().second;
			ctrlBuffer.pop();

			ppuTempAddress = (ppuTempAddress & 0xF3FF) | ((0x3 & static_cast<unsigned short int>(value)) << 10); // High bits of NameTable address
			ppuAddressIncrement = (0x4 & value) >> 2;
			baseSpriteTableAddress = 0x1000 * ((0x8 & value) >> 3);
			baseBackgroundTableAddress = 0x1000 * ((0x10 & value) >> 4);
			spriteSize = (0x20 & value) >> 5;
			nmiEnabled = (0x80 & value) >> 7;

			lowerBits = (0x1F & value);
		}
		else if (maskBuffer.size() > 0 && maskBuffer.front().first == clock - cycles)
		{
			unsigned char value = maskBuffer.front().second;
			maskBuffer.pop();

			grayscale = (0x1 & value);
			showBackgroundLeft = (0x2 & value) >> 1;
			showSpritesLeft = (0x4 & value) >> 2;
			showBackground = (0x8 & value) >> 3;
			showSprites = (0x10 & value) >> 4;
			intenseRed = (0x20 & value) >> 5;
			intenseGreen = (0x40 & value) >> 6;
			intenseBlue = (0x80 & value) >> 7;

			lowerBits = (0x1F & value);
		}
		else if (scrollBuffer.size() > 0 && scrollBuffer.front().first == clock - cycles)
		{
			unsigned short int value = scrollBuffer.front().second;
			scrollBuffer.pop();

			if (addressLatch)
			{
				ppuTempAddress = (ppuTempAddress & 0x0FFF) | ((0x7 & value) << 12);
				ppuTempAddress = (ppuTempAddress & 0xFC1F) | ((0xF8 & value) << 2);
			}
			else
			{
				fineXScroll = static_cast<unsigned char>(0x7 & value);
				ppuTempAddress = (ppuTempAddress & 0xFFE0) | ((0xF8 & value) >> 3);
			}

			addressLatch = !addressLatch;
			lowerBits = static_cast<unsigned char>(0x1F & value);
		}
		else if (oamAddrBuffer.size() > 0 && oamAddrBuffer.front().first == clock - cycles)
		{
			oamAddress = oamAddrBuffer.front().second;
			oamAddrBuffer.pop();

			lowerBits = (0x1F & oamAddress);
		}
		else if (ppuAddrBuffer.size() > 0 && ppuAddrBuffer.front().first == clock - cycles)
		{
			unsigned short int value = ppuAddrBuffer.front().second;
			ppuAddrBuffer.pop();

			addressLatch ? ppuTempAddress = (ppuTempAddress & 0xFF00) | value : ppuTempAddress = (ppuTempAddress & 0x00FF) | ((0x3F & value) << 8);
			if (addressLatch) ppuAddress = ppuTempAddress;

			addressLatch = !addressLatch;
			lowerBits = static_cast<unsigned char>(0x1F & value);
		}
		else if (oamDataBuffer.size() > 0 && oamDataBuffer.front().first == clock - cycles)
		{
			unsigned char value = oamDataBuffer.front().second;
			oamDataBuffer.pop();

			primaryOAM[oamAddress++] = value;
			lowerBits = (0x1F & value);
		}
		else if (ppuDataBuffer.size() > 0 && ppuDataBuffer.front().first == clock - cycles)
		{
			unsigned char value = ppuDataBuffer.front().second;
			ppuDataBuffer.pop();

			Write(ppuAddress, value);
			ppuAddressIncrement ? ppuAddress = (ppuAddress + 32) & 0x7FFF : ppuAddress = (ppuAddress + 1) & 0x7FFF;
			lowerBits = (0x1F & value);
		}

		cycles -= 1;
	}
}

// This is sort of a bastardized version of the PPU sprite evaluation
// It is not cycle accurate unfortunately, but I don't suspect that will
// cause any issues (but what do I know really?)
void PPU::SpriteEvaluation()
{
	for (int i = 0; i < 8; ++i)
	{
		spriteCount = 0;
		glitchCount = 0;
		unsigned char index = i * 4;
		secondaryOAM[index] = 0xFF;
		secondaryOAM[index + 1] = 0xFF;
		secondaryOAM[index + 2] = 0xFF;
		secondaryOAM[index + 3] = 0xFF;
	}

	for (int i = 0; i < 64; ++i)
	{
		unsigned char size = spriteSize ? 8 : 16;
		unsigned char index = i * 4; // Index of one of 64 sprites in Primary OAM
		unsigned char spriteY = primaryOAM[index + glitchCount]; // The sprites Y coordinate, glitchCount is used to replicated a hardware glitch
		unsigned char frameY = ((ppuAddress & 0x03E0) >> 2) | ((ppuAddress & 0x7000) >> 12); // The current Y position in the frame

		if (spriteCount < 8) // If fewer than 8 sprites have been found
		{
			// If a sprite is in range, copy it to the next spot in secondary OAM
			if (spriteY <= frameY && spriteY + size > frameY)
			{
				secondaryOAM[spriteCount * 4] = primaryOAM[index];
				secondaryOAM[(spriteCount * 4) + 1] = primaryOAM[index + 1];
				secondaryOAM[(spriteCount * 4) + 2] = primaryOAM[index + 2];
				secondaryOAM[(spriteCount * 4) + 3] = primaryOAM[index + 3];
				spriteCount++;
			}
		}
		else // 8 sprites have already been found
		{
			// If the sprite is in range, set the sprite overflow, due to glitch Count this may actually be incorrect
			if (spriteY <= frameY && spriteY + size >= frameY && !spriteOverflow)
			{
				spriteOverflow = true;
			}
			else if(!spriteOverflow) // If no sprite in range the increment glitchCount
			{
				glitchCount++;
			}
		}
	}
}

void PPU::Render()
{
	unsigned short int bgPixel = (((backgroundShift0 << fineXScroll) & 0x8000) >> 15) | (((backgroundShift1 << fineXScroll) & 0x8000) >> 14);
	unsigned char bgAttribute = ((backgroundAttributeShift0 & 0x80) >> 7) | ((backgroundAttributeShift1 & 0x80) >> 6);
	unsigned short int bgPaletteIndex = 0x3F00 | (bgAttribute << 2) | bgPixel;

	backgroundAttributeShift0 = (backgroundAttributeShift0 << 1) | (backgroundAttribute & 0x1);
	backgroundAttributeShift1 = (backgroundAttributeShift1 << 1) | ((backgroundAttribute & 0x2) >> 1);
	backgroundShift0 = backgroundShift0 << 1;
	backgroundShift1 = backgroundShift1 << 1;

	bool active[8] = {false, false, false, false, false, false, false, false};

	for (int f = 0; f < 8; ++f)
	{
		if (spriteCounter[f] != 0)
		{
			--spriteCounter[f];
		}

		active[f] = (spriteCounter[f] == 0);
	}

	bool spriteFound = false;
	unsigned short int spPixel = 0;
	unsigned short int spPaletteIndex = 0x3F10;
	unsigned char spPriority = 1;

	for (int f = 0; f < 8; ++f)
	{
		if (active[f])
		{
			bool horizontalFlip = (spriteAttribute[f] & 0x40) >> 6;
			unsigned short int pixel;

			if (horizontalFlip)
			{
				pixel = (spriteShift0[f] & 0x1) | ((spriteShift1[f] & 0x1) << 1);
				spriteShift0[f] = spriteShift0[f] >> 1;
				spriteShift1[f] = spriteShift1[f] >> 1;
			}
			else
			{
				pixel = ((spriteShift0[f] & 0x80) >> 7) | ((spriteShift1[f] & 0x80) >> 6);
				spriteShift0[f] = spriteShift0[f] << 1;
				spriteShift1[f] = spriteShift1[f] << 1;
			}

			if (!spriteFound && pixel != 0)
			{
				spPixel = pixel;
				spPriority = (spriteAttribute[f] & 0x20) >> 5;
				spPaletteIndex |= ((spriteAttribute[f] & 0x03) << 2) | spPixel;
				spriteFound = true;

				// Detect a sprite 0 hit
				if (dot >= 1 && f == 0 && spPixel != 0 && bgPixel != 0)
				{
					sprite0Hit = true;
				}
			}
		}
	}

	unsigned short int paletteIndex;

	if (bgPixel == 0 && spPixel == 0)
	{
		paletteIndex = 0x3F00;
	}
	else if (bgPixel == 0 && spPixel != 0)
	{
		paletteIndex = spPaletteIndex;
	}
	else if (bgPixel != 0 && spPixel == 0)
	{
		paletteIndex = bgPaletteIndex;
	}
	else if (spPriority == 0)
	{
		paletteIndex = spPaletteIndex;
	}
	else
	{
		paletteIndex = bgPaletteIndex;
	}

	if ((paletteIndex & 0x3) == 0)
	{
		paletteIndex = 0x3F00;
	}

	display.NextPixel(rgbLookupTable[Read(paletteIndex)]);
}

void PPU::TileFetch()
{

	backgroundShift0 = backgroundShift0 << 8;
	backgroundShift1 = backgroundShift1 << 8;

	backgroundShift0 |= tileBitmapLow;
	backgroundShift1 |= tileBitmapHigh;
	backgroundAttribute = attributeByte;

	// Perform fetches
	nameTableByte = ReadNameTable(0x2000 | (ppuAddress & 0x0FFF)); // Get the Name Table byte, a pointer into the pattern table

	// Get the attribute byte, determines the palette to use when rendering
	attributeByte = ReadNameTable(0x23C0 | (ppuAddress & 0x0C00) | ((ppuAddress >> 4) & 0x38) | ((ppuAddress >> 2) & 0x07));
	unsigned char attributeShift = (((ppuAddress & 0x0002) >> 1) | ((ppuAddress & 0x0040) >> 5)) * 2;
	attributeByte = (attributeByte >> attributeShift) & 0x3;

	unsigned char fineY = ppuAddress >> 12; // Get fine y scroll bits from address
	unsigned short int patternAddress = static_cast<unsigned short int>(nameTableByte) * 16; // Get pattern address, independent of the table
	//unsigned short int baseAddress = spriteSize ? baseBackgroundTableAddress : 0; // Get base pattern table address
	tileBitmapLow = cart.ChrRead(baseBackgroundTableAddress + patternAddress + fineY); // Read pattern byte

	fineY = ppuAddress >> 12; // Get fine y scroll
	patternAddress = static_cast<unsigned short int>(nameTableByte) * 16; // Get pattern address, independent of the table
	//spriteSize ? patternAddress += 8 : patternAddress += 16; // Offset to next half of pattern, sprites can have width 8 or 16
	//unsigned short int baseAddress = spriteSize ? baseBackgroundTableAddress : 0; // Get base pattern table address
	tileBitmapHigh = cart.ChrRead(baseBackgroundTableAddress + patternAddress + fineY + 8); // Read pattern byte
}

void PPU::SpriteFetch()
{
	unsigned char sprite = (dot - 257) / 8;

	spriteAttribute[sprite] = secondaryOAM[(sprite * 4) + 2];

	spriteCounter[sprite] = secondaryOAM[(sprite * 4) + 3];

	if (sprite < spriteCount)
	{
		bool flipVertical = (0x80 & spriteAttribute[sprite]) >> 7;
		unsigned char frameY = ((ppuAddress & 0x01E0) >> 2) | ((ppuAddress & 0x7000) >> 7); // The current Y position in the frame
		unsigned char spriteY = secondaryOAM[(sprite * 4)];

		if (spriteSize) // if spriteSize is 8x16
		{
			unsigned short int base = (0x1 & secondaryOAM[(sprite * 4) + 1]) ? 0x1000 : 0; // Get base pattern table from bit 0 of pattern address
			unsigned short int patternIndex = base + ((secondaryOAM[(sprite * 4) + 1] >> 1) * 16); // Index of the beginning of the pattern
			unsigned short int offset = flipVertical ? (spriteY + 15) - (frameY - spriteY) : (frameY - spriteY); // Offset from base index

			spriteShift0[sprite] = cart.ChrRead(patternIndex + offset);
		}
		else
		{
			unsigned short int patternIndex = baseSpriteTableAddress + secondaryOAM[(sprite * 4) + 1] * 16; // Index of the beginning of the pattern
			unsigned short int offset = flipVertical ? (spriteY + 7) - (frameY - spriteY) : (frameY - spriteY); // Offset from base index

			spriteShift0[sprite] = cart.ChrRead(patternIndex + offset);
		}
	}
	else
	{
		spriteShift0[sprite] = 0x00;
	}

	if (sprite < spriteCount)
	{
		bool flipVertical = (0x80 & spriteAttribute[sprite]) >> 7;
		unsigned char frameY = ((ppuAddress & 0x01E0) >> 2) | ((ppuAddress & 0x7000) >> 7); // The current Y position in the frame
		unsigned char spriteY = secondaryOAM[(sprite * 4)];

		if (spriteSize) // if spriteSize is 8x16
		{
			unsigned short int base = (0x1 & secondaryOAM[(sprite * 4) + 1]) ? 0x1000 : 0; // Get base pattern table from bit 0 of pattern address
			unsigned short int patternIndex = base + ((secondaryOAM[(sprite * 4) + 1] >> 1) * 16); // Index of the beginning of the pattern
			unsigned short int offset = flipVertical ? (spriteY + 15) - (frameY - spriteY) : (frameY - spriteY); // Offset from base index

			spriteShift1[sprite] = cart.ChrRead(patternIndex + offset + 16);
		}
		else
		{
			unsigned short int patternIndex = baseSpriteTableAddress + secondaryOAM[(sprite * 4) + 1] * 16; // Index of the beginning of the pattern
			unsigned short int offset = flipVertical ? (spriteY + 7) - (frameY - spriteY) : (frameY - spriteY); // Offset from base index

			spriteShift1[sprite] = cart.ChrRead(patternIndex + offset + 8);
		}
	}
	else
	{
		spriteShift1[sprite] = 0x00;
	}

}

void PPU::IncrementXScroll()
{
	if ((ppuAddress & 0x001F) == 31) // Reach end of Name Table
	{
		ppuAddress &= 0xFFE0; // Set Coarse X to 0
		ppuAddress ^= 0x400; // Switch horizontal Name Table
	}
	else
	{
		ppuAddress++; // Increment Coarse X
	}
}

void PPU::IncrementYScroll()
{
	if ((ppuAddress & 0x7000) != 0x7000) // if the fine Y < 7
	{
		ppuAddress += 0x1000; // increment fine Y
	}
	else
	{
		ppuAddress &= 0x8FFF; // Set fine Y to 0
		unsigned short int coarseY = (ppuAddress & 0x03E0) >> 5; // get coarse Y

		if (coarseY == 29) // End of name table
		{
			coarseY = 0; // set coarse Y to 0
			ppuAddress ^= 0x0800; // Switch Name Table
		}
		else if (coarseY == 31) // End of attribute table
		{
			coarseY = 0; // set coarse Y to 0
		}
		else
		{
			coarseY++; // Increment coarse Y
		}

		ppuAddress = (ppuAddress & 0xFC1F) | (coarseY << 5); // Combine values into new address
	}
}

void PPU::IncrementClock(unsigned int increment)
{
	bool renderingEnabled = showSprites || showBackground;

	clock += increment;
	dot += increment;

	// Increment Scanline pixel
	if (dot >= 339 && line == 261 && !even && renderingEnabled)
	{
		dot = dot - 339;
		line = 0;
	}
	else if (dot >= 340 && line == 261)
	{
		dot = dot - 340;
		line = 0;
	}
	else if (dot >= 340)
	{
		dot = dot - 340;
		++line;
	}
}

unsigned char PPU::Read(unsigned short int address)
{
	unsigned short int addr =  address % 0x4000;

	if (addr < 0x2000)
	{
		return cart.ChrRead(addr);
	}
	else if (addr >= 0x2000 && addr < 0x3F00)
	{
		return ReadNameTable(addr);
	}
	else
	{
		if (addr % 4 == 0) addr &= 0xFF0F;  // Ignore second nibble if addr is a multiple of 4
		return palettes[addr % 0x20];
	}
}

void PPU::Write(unsigned short int address, unsigned char value)
{
	unsigned short int addr =  address % 0x4000;

	if (addr < 0x2000)
	{
		cart.ChrWrite(value, addr);
	}
	else if (addr >= 0x2000 && addr < 0x3F00)
	{
		WriteNameTable(addr, value);
	}
	else
	{
		if (addr % 4 == 0) addr &= 0xFF0F;  // Ignore second nibble if addr is a multiple of 4
		palettes[addr % 0x20] = value & 0x3F; // Ignore bits 6 and 7
	}
}

unsigned char PPU::ReadNameTable(unsigned short int address)
{
	unsigned short int nametableaddr = address % 0x2000;

	Cart::MirrorMode mode = cart.GetMirrorMode();

	switch (mode)
	{
	case Cart::MirrorMode::HORIZONTAL:

		if (nametableaddr < 0x800)
		{
			return nameTable0[nametableaddr % 0x400];
		}
		else
		{
			return nameTable1[nametableaddr % 0x400];
		}

	case Cart::MirrorMode::VERTICAL:

		if (nametableaddr < 0x400 || (nametableaddr >= 0x800 && nametableaddr < 0xC00))
		{
			return nameTable0[nametableaddr % 0x400];
		}
		else
		{
			return nameTable1[nametableaddr % 0x400];
		}
	}

	return 0;
}

void PPU::WriteNameTable(unsigned short int address, unsigned char value)
{
	unsigned short int nametableaddr = address % 0x2000;

	Cart::MirrorMode mode = cart.GetMirrorMode();

	switch (mode)
	{
	case Cart::MirrorMode::HORIZONTAL:

		if (nametableaddr < 0x800)
		{
			nameTable0[nametableaddr % 0x400] = value;
		}
		else
		{
			nameTable1[nametableaddr % 0x400] = value;
		}
		break;

	case Cart::MirrorMode::VERTICAL:

		if (nametableaddr < 0x400 || (nametableaddr >= 0x800 && nametableaddr < 0xC00))
		{
			nameTable0[nametableaddr % 0x400] = value;
		}
		else
		{
			nameTable1[nametableaddr % 0x400] = value;
		}
		break;
	}
}

PPU::PPU(NES& nes, Cart& cart, IDisplay& display)
	: nes(nes),
	  cart(cart),
	  display(display),
	  clock(0),
	  dot(0),
	  line(241),
	  even(true),
	  reset(true),
	  ppuAddressIncrement(false),
	  baseSpriteTableAddress(0),
	  baseBackgroundTableAddress(0),
	  spriteSize(false),
	  nmiEnabled(false),
	  grayscale(false),
	  showBackgroundLeft(0),
	  showSpritesLeft(0),
	  showBackground(0),
	  showSprites(0),
	  intenseRed(0),
	  intenseGreen(0),
	  intenseBlue(0),
	  lowerBits(0x06),
	  spriteOverflow(0),
	  sprite0Hit(0),
	  inVBLANK(0),
	  nmiOccured(0),
	  oamAddress(0),
	  ppuAddress(0),
	  ppuTempAddress(0),
	  fineXScroll(0),
	  addressLatch(false),
	  dataBuffer(0),
	  nameTableByte(0),
	  attributeByte(0),
	  tileBitmapLow(0),
	  tileBitmapHigh(0),
	  backgroundShift0(0),
	  backgroundShift1(0),
	  backgroundAttributeShift0(0),
	  backgroundAttributeShift1(0),
	  backgroundAttribute(0),
	  spriteCount(0),
	  glitchCount(0)
{
	for (int i = 0x2000; i < 0x3000; ++i)
	{
		Write(i, 0x00);
	}

	for (int i = 0x3F00; i < 0x3F20; ++i)
	{
		Write(i, 0x1D);
	}

	for (int i = 0; i < 0x100; ++i)
	{
		primaryOAM[i] = 0;
	}

	for (int i = 0; i < 0x20; ++i)
	{
		secondaryOAM[i] = 0;
	}

	for (int i = 0; i < 8; ++i)
	{
		spriteShift0[i] = 0;
		spriteShift1[i] = 0;
		spriteAttribute[i] = 0;
		spriteCounter[i] = 0;
	}
}

unsigned char PPU::ReadPPUStatus()
{
	if (nes.GetClock() != clock)
	{
		Sync();
	}

	unsigned char vB = static_cast<unsigned char>(inVBLANK);
	unsigned char sp0 = static_cast<unsigned char>(sprite0Hit);
	unsigned char spOv = static_cast<unsigned char>(spriteOverflow);

	inVBLANK = false;
	nmiOccured = false;
	addressLatch = false;

	return  (vB << 7) | (sp0 << 6) | (spOv << 5) | lowerBits;
}

unsigned char PPU::ReadOAMData()
{
	if (nes.GetClock() != clock)
	{
		Sync();
	}

	return primaryOAM[oamAddress];
}

unsigned char PPU::ReadPPUData()
{
	if (nes.GetClock() != clock)
	{
		Sync();
	}

	unsigned short int addr =  ppuAddress % 0x4000;
	ppuAddressIncrement ? ppuAddress = (ppuAddress + 32) & 0x7FFF : ppuAddress = (ppuAddress + 1) & 0x7FFF;
	unsigned char value = dataBuffer;

	if (addr < 0x2000)
	{
		dataBuffer = 0x00;//cart.ChrRead(addr);
	}
	else if (addr >= 0x2000 && addr < 0x4000)
	{
		dataBuffer = ReadNameTable(addr);
	}

	if (addr >= 0x3F00)
	{
		if (addr % 4 == 0) addr &= 0xFF0F; // Ignore second nibble if addr is a multiple of 4
		value = palettes[addr % 0x20];
	}

	return value;
}

void PPU::WritePPUCTRL(unsigned char M)
{
	if (nes.GetClock() > 88974 && reset)
	{
		reset = false;
	}

	if (!reset)
	{
		ctrlBuffer.push(std::pair<int, unsigned char>(nes.GetClock(), M));
	}
}

void PPU::WritePPUMASK(unsigned char M)
{
	if (nes.GetClock() > 88974 && reset)
	{
		reset = false;
	}

	if (!reset)
	{
		maskBuffer.push(std::pair<int, unsigned char>(nes.GetClock(), M));
	}
}

void PPU::WriteOAMADDR(unsigned char M)
{
	oamAddrBuffer.push(std::pair<int, unsigned char>(nes.GetClock(), M));
}

void PPU::WriteOAMDATA(unsigned char M)
{
	oamDataBuffer.push(std::pair<int, unsigned char>(nes.GetClock(), M));
}

void PPU::WritePPUSCROLL(unsigned char M)
{
	if (nes.GetClock() > 88974 && reset)
	{
		reset = false;
	}

	if (!reset)
	{
		scrollBuffer.push(std::pair<int, unsigned char>(nes.GetClock(), M));
	}
}

void PPU::WritePPUADDR(unsigned char M)
{
	if (nes.GetClock() > 88974 && reset)
	{
		reset = false;
	}

	if (!reset)
	{
		ppuAddrBuffer.push(std::pair<int, unsigned char>(nes.GetClock(), M));
	}
}

void PPU::WritePPUDATA(unsigned char M)
{
	ppuDataBuffer.push(std::pair<int, unsigned char>(nes.GetClock(), M));
}

// At this point this function just gives two points where the PPU should sync with the CPU
// If the PPU is in vBlank or the Pre-Render line then it should give an interval guaranteed to land in the visible frame (7161 in this case)
// Otherwise the next Sync happens when the next NMI will occur.
// I chose to do it this way because it simplifies the issue of variable frame length, something that is subject to change until after the pre-render line.
int PPU::ScheduleSync()
{
	if ((line >= 242 && line <= 261) || (line == 241 && dot >= 1)) // Any point in VBLANK or the Pre-Render line
	{
		return 7169; // Guaranteed to be after the pre-render line
	}
	else
	{
		return (((241 - line - 1) * 341) + (341 - dot + 1) + 1);// - (nes.GetClock() - clock); // Time to next NMI
	}
}

void PPU::Sync()
{
	while (nes.GetClock() != clock)
	{
		Tick();
	}

	/*while (nes.GetClock() != clock)
	{
		bool renderingEnabled = showSprites || showBackground;
		unsigned int increment = 0;
		unsigned int difference = nes.GetClock() - clock;

		if (dot == 1 && line == 261)
		{
			inVBLANK = false;
			nmiOccured = false;
			sprite0Hit = false;
		}


		if (dot == 0) // First dot of a line is always an idle cycle
		{
			increment = 1;
		}
		else if ((line >= 0 && line <= 239) || line == 261)
		{
			if (!renderingEnabled)
			{
				if (dot >= 1 && dot <= 256) display.NextPixel(rgbLookupTable[Read(0x3F00)]);
				increment = 1;
			}
			else if (dot >= 1 && dot <= 256)
			{
				if (difference < 8) break;
				TileFetch();
				IncrementXScroll();
				if (dot == 256) IncrementYScroll();
				if (line != 261) Render();
				increment = 8;
			}
			else if (dot >= 257 && dot <= 320)
			{
				if (difference < 8) break;
				if (line != 261 && dot == 257) SpriteEvaluation();
				SpriteFetch();
				increment = 8;
			}
			else if (dot >= 321 && dot <= 336)
			{
				if (difference < 8) break;
				TileFetch();
				IncrementXScroll();
				increment = 8;
			}
			else if (dot >= 337 && dot <= 340)
			{
				if (difference < 4) break;
				ReadNameTable(0x2000 | (ppuAddress & 0x0FFF));
				increment = 4;
			}
		}
		else if (line >= 240 && line <= 260)
		{
			if (dot == 1 && line == 241)
			{
				even = !even;
				inVBLANK = true;
				nmiOccured = true;

				if (nmiOccured && nmiEnabled)
				{
					nes.RaiseNMI();
				}
			}
			increment = 1;
		}

		IncrementClock(increment);
		UpdateState(increment);
	}*/
}

PPU::~PPU() {}
