/*
 * ppu.cc
 *
 *  Created on: Oct 9, 2014
 *      Author: Dale
 */
#include <cassert>
#include "ppu.h"
#include "nes.h"

const uint32_t PPU::rgbLookupTable[64] =
{
    0x545454, 0x001E74, 0x081090, 0x300088, 0x440064, 0x5C0030, 0x540400, 0x3C1800, 0x202A00, 0x083A00, 0x004000, 0x003C00, 0x00323C, 0x000000, 0x000000, 0x000000,
    0x989698, 0x084CC4, 0x3032EC, 0x5C1EE4, 0x8814B0, 0xA01464, 0x982220, 0x783C00, 0x545A00, 0x287200, 0x087C00, 0x007628, 0x006678, 0x000000, 0x000000, 0x000000,
    0xECEEEC, 0x4C9AEC, 0x787CEC, 0xB062EC, 0xE454EC, 0xEC58B4, 0xEC6A64, 0xD48820, 0xA0AA00, 0x74C400, 0x4CD020, 0x38CC6C, 0x38B4CC, 0x3C3C3C, 0x000000, 0x000000,
    0xECEEEC, 0xA8CCEC, 0xBCBCEC, 0xD4B2EC, 0xECAEEC, 0xECAED4, 0xECB4B0, 0xE4C490, 0xCCD278, 0xB4DE78, 0xA8E290, 0x98E2B4, 0xA0D6E4, 0xA0A2A0, 0x000000, 0x000000
};

const uint32_t PPU::resetDelay = 88974;

void PPU::GetNameTable(int table, uint8_t* pixels)
{
    uint16_t tableIndex;
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

    for (uint32_t i = 0; i < 30; ++i)
    {
        for (uint32_t f = 0; f < 32; ++f)
        {
            uint16_t address = (tableIndex | (i << 5) | f);
            uint8_t ntByte = ReadNameTable(0x2000 | address);
            uint8_t atShift = (((address & 0x0002) >> 1) | ((address & 0x0040) >> 5)) * 2;
            uint8_t atByte = ReadNameTable(0x23C0 | (address & 0x0C00) | ((address >> 4) & 0x38) | ((address >> 2) & 0x07));
            atByte = (atByte >> atShift) & 0x3;
            uint16_t patternAddress = static_cast<uint16_t>(ntByte)* 16;

            for (uint32_t h = 0; h < 8; ++h)
            {
                uint8_t tileLow = cart->ChrRead(baseBackgroundTableAddress + patternAddress + h);
                uint8_t tileHigh = cart->ChrRead(baseBackgroundTableAddress + patternAddress + h + 8);

                for (uint32_t g = 0; g < 8; ++g)
                {
                    uint16_t pixel = 0x0003 & ((((tileLow << g) & 0x80) >> 7) | (((tileHigh << g) & 0x80) >> 6));
                    uint16_t paletteIndex = 0x3F00 | (atByte << 2) | pixel;
                    if ((paletteIndex & 0x3) == 0) paletteIndex = 0x3F00;
                    uint32_t rgb = rgbLookupTable[Read(paletteIndex)];
                    uint8_t red = (rgb & 0xFF0000) >> 16;
                    uint8_t green = (rgb & 0x00FF00) >> 8;
                    uint8_t blue = (rgb & 0x0000FF);

                    uint32_t index = ((24 * f) + (g * 3)) + ((6144 * i) + (h * 768));

                    pixels[index] = red;
                    pixels[index + 1] = green;
                    pixels[index + 2] = blue;
                }
            }
        }
    }
}

void PPU::GetPatternTable(int table, int palette, uint8_t* pixels)
{
    uint16_t tableIndex;
    if (table == 0)
    {
        tableIndex = 0x0000;
    }
    else
    {
        tableIndex = 0x1000;
    }

    for (uint32_t i = 0; i < 16; ++i)
    {
        for (uint32_t f = 0; f < 16; ++f)
        {
            uint32_t patternIndex = (16 * f) + (256 * i);

            for (uint32_t g = 0; g < 8; ++g)
            {
                uint8_t tileLow = cart->ChrRead(tableIndex + patternIndex + g);
                uint8_t tileHigh = cart->ChrRead(tableIndex + patternIndex + g + 8);

                for (uint32_t h = 0; h < 8; ++h)
                {
                    uint16_t pixel = 0x0003 & ((((tileLow << h) & 0x80) >> 7) | (((tileHigh << h) & 0x80) >> 6));
                    uint16_t paletteIndex = (0x3F00 + (4 * (palette % 8))) | pixel;
                    uint32_t rgb = rgbLookupTable[Read(paletteIndex)];
                    uint8_t red = (rgb & 0xFF0000) >> 16;
                    uint8_t green = (rgb & 0x00FF00) >> 8;
                    uint8_t blue = (rgb & 0x0000FF);

                    uint32_t index = ((24 * f) + (h * 3)) + ((3072 * i) + (g * 384));

                    pixels[index] = red;
                    pixels[index + 1] = green;
                    pixels[index + 2] = blue;
                }
            }
        }
    }
}

void PPU::GetPalette(int palette, uint8_t* pixels)
{
    uint16_t baseAddress;
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

    for (uint32_t i = 0; i < 4; ++i)
    {
        uint16_t paletteAddress = baseAddress + i;

        for (uint32_t g = 0; g < 16; ++g)
        {
            for (uint32_t h = 0; h < 16; ++h)
            {
                uint32_t rgb = rgbLookupTable[Read(paletteAddress)];
                uint8_t red = (rgb & 0xFF0000) >> 16;
                uint8_t green = (rgb & 0x00FF00) >> 8;
                uint8_t blue = (rgb & 0x0000FF);

                uint32_t index = ((48 * i) + (h * 3)) + (192 * g);

                pixels[index] = red;
                pixels[index + 1] = green;
                pixels[index + 2] = blue;
            }
        }
    }
}

void PPU::GetPrimaryOAM(int sprite, uint8_t* pixels)
{
    uint8_t byteOne = primaryOAM[(0x4 * sprite) + 1];
    uint8_t byteTwo = primaryOAM[(0x4 * sprite) + 2];

    uint16_t tableIndex = baseSpriteTableAddress;
    uint16_t patternIndex = byteOne * 16;
    uint8_t palette = (byteTwo & 0x03) + 4;

    for (uint32_t i = 0; i < 8; ++i)
    {
        uint8_t tileLow = cart->ChrRead(tableIndex + patternIndex + i);
        uint8_t tileHigh = cart->ChrRead(tableIndex + patternIndex + i + 8);

        for (uint32_t f = 0; f < 8; ++f)
        {
            uint16_t pixel = 0x0003 & ((((tileLow << f) & 0x80) >> 7) | (((tileHigh << f) & 0x80) >> 6));
            uint16_t paletteIndex = (0x3F00 + (4 * (palette % 8))) | pixel;
            uint32_t rgb = rgbLookupTable[Read(paletteIndex)];
            uint8_t red = (rgb & 0xFF0000) >> 16;
            uint8_t green = (rgb & 0x00FF00) >> 8;
            uint8_t blue = (rgb & 0x0000FF);

            uint32_t index = (24 * i) + (f * 3);

            pixels[index] = red;
            pixels[index + 1] = green;
            pixels[index + 2] = blue;
        }
    }
}

void PPU::UpdateState()
{
    //assert(registerBuffer.size() > 0 ? std::get<0>(registerBuffer.front()) >= clock : true);

	if (registerBuffer.size() > 0 && std::get<0>(registerBuffer.front()) == clock)
	{
		uint8_t value = std::get<1>(registerBuffer.front());
		Register reg = std::get<2>(registerBuffer.front());
		registerBuffer.pop();

		switch (reg)
		{
		case PPUCTRL:
			ppuTempAddress = (ppuTempAddress & 0x73FF) | ((0x3 & static_cast<uint16_t>(value)) << 10); // High bits of NameTable address
			ppuAddressIncrement = ((0x4 & value) >> 2) != 0;
			baseSpriteTableAddress = 0x1000 * ((0x8 & value) >> 3);
			baseBackgroundTableAddress = 0x1000 * ((0x10 & value) >> 4);
			spriteSize = ((0x20 & value) >> 5) != 0;
			nmiEnabled = ((0x80 & value) >> 7) != 0;
			lowerBits = (0x1F & value);

            if (nmiEnabled == false && line == 241 && (dot - 1) >= 1 && (dot - 1) < 2)
            {
                interruptActive = false;
            }

			break;
		case PPUMASK:
			grayscale = (0x1 & value);
			showBackgroundLeft = ((0x2 & value) >> 1) != 0;
			showSpritesLeft = ((0x4 & value) >> 2) != 0;
			showBackground = ((0x8 & value) >> 3) != 0;
			showSprites = ((0x10 & value) >> 4) != 0;
			intenseRed = ((0x20 & value) >> 5) != 0;
			intenseGreen = ((0x40 & value) >> 6) != 0;
			intenseBlue = ((0x80 & value) >> 7) != 0;
			lowerBits = (0x1F & value);
			break;
		case PPUSCROLL:
			if (addressLatch)
			{
				ppuTempAddress = (ppuTempAddress & 0x0FFF) | ((0x7 & value) << 12);
				ppuTempAddress = (ppuTempAddress & 0x7C1F) | ((0xF8 & value) << 2);
			}
			else
			{
				fineXScroll = static_cast<uint8_t>(0x7 & value);
				ppuTempAddress = (ppuTempAddress & 0x7FE0) | ((0xF8 & value) >> 3);
			}

			addressLatch = !addressLatch;
			lowerBits = static_cast<uint8_t>(0x1F & value);
			break;
		case OAMADDR:
			oamAddress = value;
			lowerBits = (0x1F & oamAddress);
			break;
		case PPUADDR:
			addressLatch ? ppuTempAddress = (ppuTempAddress & 0x7F00) | value : ppuTempAddress = (ppuTempAddress & 0x00FF) | ((0x3F & value) << 8);
			if (addressLatch) ppuAddress = ppuTempAddress;
			addressLatch = !addressLatch;
			lowerBits = static_cast<uint8_t>(0x1F & value);
			break;
		case OAMDATA:
			primaryOAM[oamAddress++] = value;
			lowerBits = (0x1F & value);
			break;
		case PPUDATA:
			Write(ppuAddress, value);
			ppuAddressIncrement ? ppuAddress = (ppuAddress + 32) & 0x7FFF : ppuAddress = (ppuAddress + 1) & 0x7FFF;
			lowerBits = (0x1F & value);
			break;
		default:
			break;
		}
	}
}

// This is sort of a bastardized version of the PPU sprite evaluation
// It is not cycle accurate unfortunately, but I don't suspect that will
// cause any issues (but what do I know really?)
void PPU::SpriteEvaluation()
{
	uint8_t glitchCount = 0;
	spriteCount = 0;
	sprite0SecondaryOAM = false;

    for (int i = 0; i < 8; ++i)
    {
        uint8_t index = i * 4;
        secondaryOAM[index] = 0xFF;
        secondaryOAM[index + 1] = 0xFF;
        secondaryOAM[index + 2] = 0xFF;
        secondaryOAM[index + 3] = 0xFF;
    }

    for (int i = 0; i < 64; ++i)
    {
        uint8_t size = spriteSize ? 16 : 8;
        uint8_t index = i * 4; // Index of one of 64 sprites in Primary OAM
        uint8_t spriteY = primaryOAM[index + glitchCount]; // The sprites Y coordinate, glitchCount is used to replicate a hardware glitch

        if (spriteCount < 8) // If fewer than 8 sprites have been found
        {
            // If a sprite is in range, copy it to the next spot in secondary OAM
            if (spriteY <= line && spriteY + size > line && line != 239)
            {
                if (index == 0)
                {
                    sprite0SecondaryOAM = true;
                }

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
            if (spriteY <= line && spriteY + size > line && !spriteOverflow)
            {
                spriteOverflow = true;
            }
            else if (!spriteOverflow) // If no sprite in range the increment glitchCount
            {
                glitchCount++;
            }
        }
    }
}

void PPU::Render()
{
	if (!showSprites && !showBackground)
	{
		if (ppuAddress >= 0x3F00 && ppuAddress <= 0x3FFF)
		{
			display.NextPixel(rgbLookupTable[Read(ppuAddress)]);
		}
		else
		{
			display.NextPixel(rgbLookupTable[Read(0x3F00)]);
		}
	}
	else
	{
		uint16_t bgPixel = (((backgroundShift0 << fineXScroll) & 0x8000) >> 15) | (((backgroundShift1 << fineXScroll) & 0x8000) >> 14);
		uint8_t bgAttribute = (((backgroundAttributeShift0 << fineXScroll) & 0x80) >> 7) | (((backgroundAttributeShift1 << fineXScroll) & 0x80) >> 6);
		uint16_t bgPaletteIndex = 0x3F00 | (bgAttribute << 2) | bgPixel;

		if (!showBackground || (!showBackgroundLeft && dot <= 8)) bgPixel = 0;

		backgroundAttributeShift0 = (backgroundAttributeShift0 << 1) | (backgroundAttribute & 0x1);
		backgroundAttributeShift1 = (backgroundAttributeShift1 << 1) | ((backgroundAttribute & 0x2) >> 1);
		backgroundShift0 = backgroundShift0 << 1;
		backgroundShift1 = backgroundShift1 << 1;

		bool active[8] = { false, false, false, false, false, false, false, false };

		for (int f = 0; f < 8; ++f)
		{
			if (spriteCounter[f] == 0)
			{
				active[f] = true;
			}
			else
			{
				--spriteCounter[f];
			}
		}

		bool spriteFound = false;
		uint16_t spPixel = 0;
		uint16_t spPaletteIndex = 0x3F10;
		uint8_t spPriority = 1;

		for (int f = 0; f < 8; ++f)
		{
			if (active[f])
			{
				bool horizontalFlip = ((spriteAttribute[f] & 0x40) >> 6) != 0;
				uint16_t pixel;

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

					if (!showSprites || (!showSpritesLeft && dot <= 8)) spPixel = 0;

					// Detect a sprite 0 hit
					if (sprite0SecondaryOAM && f == 0)
					{
						if (dot > 1 && spPixel != 0 && bgPixel != 0 && dot != 256)
						{
							sprite0Hit = true;
						}
					}
				}
			}
		}

		uint16_t paletteIndex;

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
        uint16_t coarseY = (ppuAddress & 0x03E0) >> 5; // get coarse Y

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

void PPU::IncrementClock()
{
    bool renderingEnabled = showSprites || showBackground;

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

uint8_t PPU::Read(uint16_t address)
{
    uint16_t addr = address % 0x4000;

    if (addr < 0x2000)
    {
        return cart->ChrRead(addr);
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

void PPU::Write(uint16_t address, uint8_t value)
{
    uint16_t addr = address % 0x4000;

    if (addr < 0x2000)
    {
        cart->ChrWrite(value, addr);
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

uint8_t PPU::ReadNameTable(uint16_t address)
{
    uint16_t nametableaddr = address % 0x2000;

    Cart::MirrorMode mode = cart->GetMirrorMode();

    switch (mode)
    {
    case Cart::MirrorMode::SINGLE_SCREEN_A:

        return nameTable0[nametableaddr % 0x400];

    case Cart::MirrorMode::SINGLE_SCREEN_B:

        return nameTable1[nametableaddr % 0x400];

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

void PPU::WriteNameTable(uint16_t address, uint8_t value)
{
    uint16_t nametableaddr = address % 0x2000;

    Cart::MirrorMode mode = cart->GetMirrorMode();

    switch (mode)
    {
    case Cart::MirrorMode::SINGLE_SCREEN_A:

        nameTable0[nametableaddr % 0x400] = 0;
        break;

    case Cart::MirrorMode::SINGLE_SCREEN_B:

        nameTable1[nametableaddr % 0x400] = 0;
        break;

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

PPU::PPU(NES& nes, IDisplay& display) :
	nes(nes),
	display(display),
	cpu(0),
	cart(0),
	intervalStart(boost::chrono::high_resolution_clock::now()),
	limitTo60FPS(true),
	clock(0),
	dot(0),
	line(0),
	even(false),
	//reset(true),
	suppressNMI(false),
    interruptActive(false),
    nmiOccuredCycle(0),
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
    spriteOverflow(false),
    sprite0Hit(false),
    inVBLANK(false),
    nmiOccured(false),
    sprite0SecondaryOAM(false),
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
    spriteCount(0)
{
	for (int i = 0; i < 0x400; ++i)
	{
		nameTable0[i] = 0;
		nameTable1[i] = 0;
	}

	for (int i = 0; i < 0x100; ++i)
	{
		primaryOAM[i] = 0;
	}

	for (int i = 0; i < 0x20; ++i)
	{
		secondaryOAM[i] = 0;
		palettes[i] = 0;
	}

    for (int i = 0; i < 8; ++i)
    {
        spriteShift0[i] = 0;
        spriteShift1[i] = 0;
        spriteAttribute[i] = 0;
        spriteCounter[i] = 0;
    }
}

void PPU::AttachCPU(CPU& cpu)
{
	this->cpu = &cpu;
}

void PPU::AttachCart(Cart& cart)
{
	this->cart = &cart;
}

void PPU::EnableFrameLimit()
{
	limitTo60FPS = true;
}

void PPU::DisableFrameLimit()
{
	limitTo60FPS = false;
}

uint8_t PPU::ReadPPUStatus()
{
    if (cpu->GetClock() != clock)
    {
        Run();
    }

    uint8_t vB = static_cast<uint8_t>(nmiOccured);
    uint8_t sp0 = static_cast<uint8_t>(sprite0Hit);
    uint8_t spOv = static_cast<uint8_t>(spriteOverflow);

	if (line == 241 && (dot - 1) == 0)
	{
		suppressNMI = true; // Suppress interrupt
	}

    if (line == 241 && (dot - 1) >= 1 && (dot - 1) <= 2)
    {
        interruptActive = false;
    }

    nmiOccured = false;
    addressLatch = false;

    return  (vB << 7) | (sp0 << 6) | (spOv << 5) | lowerBits;
}

uint8_t PPU::ReadOAMData()
{
    if (cpu->GetClock() != clock)
    {
        Run();
    }

    return primaryOAM[oamAddress];
}

uint8_t PPU::ReadPPUData()
{
    if (cpu->GetClock() != clock)
    {
        Run();
    }

    uint16_t addr = ppuAddress % 0x4000;
    ppuAddressIncrement ? ppuAddress = (ppuAddress + 32) & 0x7FFF : ppuAddress = (ppuAddress + 1) & 0x7FFF;
    uint8_t value = dataBuffer;

    if (addr < 0x2000)
    {
        dataBuffer = cart->ChrRead(addr);
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

void PPU::WritePPUCTRL(uint8_t M)
{
    if (cpu->GetClock() > resetDelay)
    {
		if (inVBLANK)
		{
			ppuTempAddress = (ppuTempAddress & 0x73FF) | ((0x3 & static_cast<uint16_t>(M)) << 10); // High bits of NameTable address
			ppuAddressIncrement = ((0x4 & M) >> 2) != 0;
			baseSpriteTableAddress = 0x1000 * ((0x8 & M) >> 3);
			baseBackgroundTableAddress = 0x1000 * ((0x10 & M) >> 4);
			spriteSize = ((0x20 & M) >> 5) != 0;
			nmiEnabled = ((0x80 & M) >> 7) != 0;
			lowerBits = (0x1F & M);

            if (nmiEnabled == false && line == 241 && (dot - 1) == 1)
            {
                interruptActive = false;
            }
		}
		else
		{
			registerBuffer.push(std::tuple<uint64_t, uint8_t, Register>(cpu->GetClock(), M, PPUCTRL));
		}
    }
}

void PPU::WritePPUMASK(uint8_t M)
{
    if (cpu->GetClock() > resetDelay)
    {
		if (inVBLANK || cpu->GetClock() == clock - 1)
		{
			grayscale = (0x1 & M);
			showBackgroundLeft = ((0x2 & M) >> 1) != 0;
			showSpritesLeft = ((0x4 & M) >> 2) != 0;
			showBackground = ((0x8 & M) >> 3) != 0;
			showSprites = ((0x10 & M) >> 4) != 0;
			intenseRed = ((0x20 & M) >> 5) != 0;
			intenseGreen = ((0x40 & M) >> 6) != 0;
			intenseBlue = ((0x80 & M) >> 7) != 0;
			lowerBits = (0x1F & M);
		}
		else
		{
			registerBuffer.push(std::tuple<uint64_t, uint8_t, Register>(cpu->GetClock(), M, PPUMASK));
		}
    }
}

void PPU::WriteOAMADDR(uint8_t M)
{
    if (inVBLANK || cpu->GetClock() == clock - 1)
	{
		oamAddress = M;
		lowerBits = (0x1F & oamAddress);
	}
	else
	{
		registerBuffer.push(std::tuple<uint64_t, uint8_t, Register>(cpu->GetClock(), M, OAMADDR));
	}
}

void PPU::WriteOAMDATA(uint8_t M)
{
    if (inVBLANK || cpu->GetClock() == clock - 1)
	{
		primaryOAM[oamAddress++] = M;
		lowerBits = (0x1F & M);
	}
	else
	{
		registerBuffer.push(std::tuple<uint64_t, uint8_t, Register>(cpu->GetClock(), M, OAMDATA));
	}
}

void PPU::WritePPUSCROLL(uint8_t M)
{
    if (cpu->GetClock() > resetDelay)
    {
        if (inVBLANK || cpu->GetClock() == clock - 1)
		{
			if (addressLatch)
			{
				ppuTempAddress = (ppuTempAddress & 0x0FFF) | ((0x7 & M) << 12);
				ppuTempAddress = (ppuTempAddress & 0x7C1F) | ((0xF8 & M) << 2);
			}
			else
			{
				fineXScroll = static_cast<uint8_t>(0x7 & M);
				ppuTempAddress = (ppuTempAddress & 0x7FE0) | ((0xF8 & M) >> 3);
			}

			addressLatch = !addressLatch;
			lowerBits = static_cast<uint8_t>(0x1F & M);
		}
		else
		{
			registerBuffer.push(std::tuple<uint64_t, uint8_t, Register>(cpu->GetClock(), M, PPUSCROLL));
		}
		
    }
}

void PPU::WritePPUADDR(uint8_t M)
{
    if (cpu->GetClock() > resetDelay)
    {
        if (inVBLANK || cpu->GetClock() == clock - 1)
		{
			addressLatch ? ppuTempAddress = (ppuTempAddress & 0x7F00) | M : ppuTempAddress = (ppuTempAddress & 0x00FF) | ((0x3F & M) << 8);
			if (addressLatch) ppuAddress = ppuTempAddress;
			addressLatch = !addressLatch;
			lowerBits = static_cast<uint8_t>(0x1F & M);
		}
		else
		{
			registerBuffer.push(std::tuple<uint64_t, uint8_t, Register>(cpu->GetClock(), M, PPUADDR));
		}
		
    }
}

void PPU::WritePPUDATA(uint8_t M)
{
    if (inVBLANK || cpu->GetClock() == clock - 1)
	{
		Write(ppuAddress, M);
		ppuAddressIncrement ? ppuAddress = (ppuAddress + 32) & 0x7FFF : ppuAddress = (ppuAddress + 1) & 0x7FFF;
		lowerBits = (0x1F & M);
	}
	else
	{
		uint64_t clock = cpu->GetClock();
		registerBuffer.push(std::tuple<uint64_t, uint8_t, Register>(cpu->GetClock(), M, PPUDATA));
	}
	
}

// At this point this function just gives two points where the PPU should sync with the CPU
// If the PPU is in vBlank or the Pre-Render line then it should give an interval guaranteed to land in the visible frame (7169 in this case)
// Otherwise the next Sync happens when the next NMI will occur.
// I chose to do it this way because it simplifies the issue of variable frame length, something that is subject to change until after the pre-render line.
int PPU::ScheduleSync()
{
    if ((line >= 242 && line <= 261) || (line == 241 && dot >= 2)) // Any point in VBLANK or the Pre-Render line
    {
        if (nmiOccured)
        {
            return 0;
        }
        else
        {
            return 7169; // Guaranteed to be after the pre-render line
        }
    }
    else
    {
        return ((240 - line) * 341) + (341 - dot + 1) + 1; // Time to next NMI
    }
/*    return 0;*/
}

bool PPU::CheckInterrupt(uint64_t& occurredCycle)
{
    occurredCycle = nmiOccuredCycle;
    return interruptActive;
}

void PPU::Run()
{
	while (cpu->GetClock() >= clock)
	{
		while (((line >= 0 && line <= 239) || line == 261) && cpu->GetClock() >= clock)
		{
			if (dot == 0)
			{
				UpdateState();

                if (line == 0)
                {
                    even = !even;
                }

				IncrementClock();

				if (cpu->GetClock() < clock) return;
			}

			while (dot >= 1 && dot <= 256 && cpu->GetClock() >= clock)
			{
				UpdateState();

                if (dot == 1 && line == 261)
                {
                    inVBLANK = false;
                    nmiOccured = false;
                    interruptActive = false;
                    sprite0Hit = false;
                }

				//*************************************************************************************
				// Background Fetch Phase
				//*************************************************************************************
				uint8_t cycle = (dot - 1) % 8; // The current point in the background fetch cycle

				if (showSprites || showBackground)
				{

					// Reload background shift registers
					if (cycle == 0 && dot != 1)
					{
						backgroundShift0 = (backgroundShift0 & 0xFF00) | tileBitmapLow;
						backgroundShift1 = (backgroundShift1 & 0xFF00) | tileBitmapHigh;
						backgroundAttribute = attributeByte;
					}

// 					if (cycle % 2 == 0)
// 					{
// 						if (line != 261) // Render on all visible lines
// 						{
// 							Render();
// 						}
// 
// 						IncrementClock();
// 
//                         if (cpu->GetClock() < clock) return;
// 
// 						UpdateState();
// 						++cycle;
// 					}

					// Perform fetches
					if (cycle == 1)
					{
						nameTableByte = ReadNameTable(0x2000 | (ppuAddress & 0x0FFF)); // Get the Name Table byte, a pointer into the pattern table
					}
					else if (cycle == 3)
					{
						// Get the attribute byte, determines the palette to use when rendering
						attributeByte = ReadNameTable(0x23C0 | (ppuAddress & 0x0C00) | ((ppuAddress >> 4) & 0x38) | ((ppuAddress >> 2) & 0x07));
						uint8_t attributeShift = (((ppuAddress & 0x0002) >> 1) | ((ppuAddress & 0x0040) >> 5)) * 2;
						attributeByte = (attributeByte >> attributeShift) & 0x3;
					}
					else if (cycle == 5)
					{
						uint8_t fineY = ppuAddress >> 12; // Get fine y scroll bits from address
						uint16_t patternAddress = static_cast<uint16_t>(nameTableByte)* 16; // Get pattern address, independent of the table
						tileBitmapLow = cart->ChrRead(baseBackgroundTableAddress + patternAddress + fineY); // Read pattern byte
					}
					else if (cycle == 7)
					{
						uint8_t fineY = ppuAddress >> 12; // Get fine y scroll
						uint16_t patternAddress = static_cast<uint16_t>(nameTableByte)* 16; // Get pattern address, independent of the table
						tileBitmapHigh = cart->ChrRead(baseBackgroundTableAddress + patternAddress + fineY + 8); // Read pattern byte
					}
				}

				if (line != 261) // Render on all visible lines
				{
					Render();
				}

				if (showSprites || showBackground)
				{
					// Every 8 dots increment the coarse X scroll
					if (cycle == 7)
					{
						IncrementXScroll();
					}

					// Exactly at dot 256 increment the Y scroll
					if (dot == 256)
					{
						IncrementYScroll();
					}
				}

				if (line == 239 && dot == 256 && limitTo60FPS)
				{
					using namespace boost::chrono;

					high_resolution_clock::time_point now = high_resolution_clock::now();
					nanoseconds span = duration_cast<nanoseconds>(now - intervalStart);

					while (span.count() < 16666667)
					{
						now = high_resolution_clock::now();
						span = duration_cast<nanoseconds>(now - intervalStart);
					}

					intervalStart = high_resolution_clock::now();
				}

				IncrementClock();
			}

			while (dot >= 257 && dot <= 320 && cpu->GetClock() >= clock)
			{
				UpdateState();

				if (showSprites || showBackground)
				{
					//*************************************************************************************
					// Sprite Fetch Phase
					//*************************************************************************************

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

					uint8_t cycle = (dot - 257) % 8; // The current point in the sprite fetch cycle
					uint8_t sprite = (dot - 257) / 8;

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
							bool flipVertical = ((0x80 & spriteAttribute[sprite]) >> 7) != 0;
							uint8_t spriteY = secondaryOAM[(sprite * 4)];

							if (spriteSize) // if spriteSize is 8x16
							{
								uint16_t base = (0x1 & secondaryOAM[(sprite * 4) + 1]) ? 0x1000 : 0; // Get base pattern table from bit 0 of pattern address
								uint16_t patternIndex = base + ((secondaryOAM[(sprite * 4) + 1] >> 1) * 32); // Index of the beginning of the pattern
								uint16_t offset = flipVertical ? 15 - (line - spriteY) : (line - spriteY); // Offset from base index
								if (offset >= 8) offset += 8;

								spriteShift0[sprite] = cart->ChrRead(patternIndex + offset);
							}
							else
							{
								uint16_t patternIndex = baseSpriteTableAddress + (secondaryOAM[(sprite * 4) + 1] * 16); // Index of the beginning of the pattern
								uint16_t offset = flipVertical ? 7 - (line - spriteY) : (line - spriteY); // Offset from base index

								spriteShift0[sprite] = cart->ChrRead(patternIndex + offset);
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
							bool flipVertical = ((0x80 & spriteAttribute[sprite]) >> 7) != 0;
							uint8_t spriteY = secondaryOAM[(sprite * 4)];

							if (spriteSize) // if spriteSize is 8x16
							{
								uint16_t base = (0x1 & secondaryOAM[(sprite * 4) + 1]) ? 0x1000 : 0; // Get base pattern table from bit 0 of pattern address
								uint16_t patternIndex = base + ((secondaryOAM[(sprite * 4) + 1] >> 1) * 32); // Index of the beginning of the pattern
								uint16_t offset = flipVertical ? 15 - (line - spriteY) : (line - spriteY); // Offset from base index
								if (offset >= 8) offset += 8;

								spriteShift1[sprite] = cart->ChrRead(patternIndex + offset + 8);
							}
							else
							{
								uint16_t patternIndex = baseSpriteTableAddress + (secondaryOAM[(sprite * 4) + 1] * 16); // Index of the beginning of the pattern
								uint16_t offset = flipVertical ? 7 - (line - spriteY) : (line - spriteY); // Offset from base index

								spriteShift1[sprite] = cart->ChrRead(patternIndex + offset + 8);
							}
						}
						else
						{
							spriteShift1[sprite] = 0x00;
						}
					}
				}

				IncrementClock();
			}

			while (dot >= 321 && dot <= 336 && cpu->GetClock() >= clock)
			{
				UpdateState();

				if (showSprites || showBackground)
				{
					//*************************************************************************************
					// Background Fetch Phase
					//*************************************************************************************
					uint8_t cycle = (dot - 1) % 8; // The current point in the background fetch cycle

					// Reload background shift registers
					if (cycle == 0 && dot != 1)
					{
						backgroundShift0 = (backgroundShift0 & 0xFF00) | tileBitmapLow;
						backgroundShift1 = (backgroundShift1 & 0xFF00) | tileBitmapHigh;
						backgroundAttribute = attributeByte;
					}

// 					if (cycle % 2 == 0)
// 					{
// 						IncrementClock();
// 
//                         if (cpu->GetClock() < clock) return;
// 
// 						UpdateState();
// 						++cycle;
// 					}

					// Perform fetches
					if (cycle == 1)
					{
						nameTableByte = ReadNameTable(0x2000 | (ppuAddress & 0x0FFF)); // Get the Name Table byte, a pointer into the pattern table
					}
					else if (cycle == 3)
					{
						// Get the attribute byte, determines the palette to use when rendering
						attributeByte = ReadNameTable(0x23C0 | (ppuAddress & 0x0C00) | ((ppuAddress >> 4) & 0x38) | ((ppuAddress >> 2) & 0x07));
						uint8_t attributeShift = (((ppuAddress & 0x0002) >> 1) | ((ppuAddress & 0x0040) >> 5)) * 2;
						attributeByte = (attributeByte >> attributeShift) & 0x3;
					}
					else if (cycle == 5)
					{
						uint8_t fineY = ppuAddress >> 12; // Get fine y scroll bits from address
						uint16_t patternAddress = static_cast<uint16_t>(nameTableByte)* 16; // Get pattern address, independent of the table
						tileBitmapLow = cart->ChrRead(baseBackgroundTableAddress + patternAddress + fineY); // Read pattern byte
					}
					else if (cycle == 7)
					{
						uint8_t fineY = ppuAddress >> 12; // Get fine y scroll
						uint16_t patternAddress = static_cast<uint16_t>(nameTableByte)* 16; // Get pattern address, independent of the table
						tileBitmapHigh = cart->ChrRead(baseBackgroundTableAddress + patternAddress + fineY + 8); // Read pattern byte
					}

					// Every 8 dots increment the coarse X scroll
					if (cycle == 7)
					{
						IncrementXScroll();
					}
				}

				IncrementClock();
			}

			while (dot >= 337 && dot <= 340 && cpu->GetClock() >= clock)
			{
				UpdateState();

				if (showSprites || showBackground)
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

					uint8_t cycle = (dot - 337) % 2;

// 					if (cycle == 0)
// 					{
// 						IncrementClock();
// 
//                         if (cpu->GetClock() < clock) return;
// 
// 						UpdateState();
// 						++cycle;
// 					}

					if (cycle == 1)
					{
						ReadNameTable(0x2000 | (ppuAddress & 0x0FFF));
					}
				}

				IncrementClock();
			}
		}

		while (line >= 240 && line <= 260 && cpu->GetClock() >= clock)
		{
            if (line == 240 || (dot == 0 && line == 241))
            {
                UpdateState();
                IncrementClock();
            }
            else if (dot == 1 && line == 241 && clock > resetDelay)
			{
				UpdateState();

				if (dot == 1 && line == 241 && !suppressNMI)
				{
					nmiOccured = true;

                    if (nmiOccured && nmiEnabled)
                    {
                        nmiOccuredCycle = clock;
                        interruptActive = true;
                    }
				}

                suppressNMI = false;

				IncrementClock();
			}
//             else if (inVBLANK)
// 			{
// 				UpdateState();
// 
//                 uint64_t timeToSync = cpu->GetClock() - ppuClock + 1;
//                 uint64_t timeToEndVBlank = (341 - dot) + ((260 - line) * 341) + 1;
// 
//                 if (timeToEndVBlank > timeToSync)
//                 {
//                     line += static_cast<int16_t>(timeToSync / 341);
//                     dot += static_cast<int16_t>(timeToSync % 341);
// 
//                     if (dot > 340)
//                     {
//                         dot %= 341;
//                         ++line;
//                     }
// 
//                     ppuClock += timeToSync;
//                 }
//                 else
//                 {
//                     ppuClock += timeToEndVBlank;
//                     dot = 1;
//                     line = 261;
//                 }
//             }
            else
            {
                if (!nmiOccured || !nmiEnabled)
                {
                    interruptActive = false;
                }

                UpdateState();

                if (nmiOccured && nmiEnabled)
                {
                    if (!interruptActive)
                    {
                        nmiOccuredCycle = clock;
                        interruptActive = true;
                    }
                }

                IncrementClock();
            }
		}
	}
}

PPU::~PPU() {}
