/*
 * ppu.cc
 *
 *  Created on: Oct 9, 2014
 *      Author: Dale
 */

#include <cstring>
#include "ppu.h"
#include "cpu.h"
#include "apu.h"
#include "video/video_backend.h"

static constexpr uint32_t RgbLookupTable[64] =
{
    0x545454, 0x001E74, 0x081090, 0x300088, 0x440064, 0x5C0030, 0x540400, 0x3C1800, 0x202A00, 0x083A00, 0x004000, 0x003C00, 0x00323C, 0x000000, 0x000000, 0x000000,
    0x989698, 0x084CC4, 0x3032EC, 0x5C1EE4, 0x8814B0, 0xA01464, 0x982220, 0x783C00, 0x545A00, 0x287200, 0x087C00, 0x007628, 0x006678, 0x000000, 0x000000, 0x000000,
    0xECEEEC, 0x4C9AEC, 0x787CEC, 0xB062EC, 0xE454EC, 0xEC58B4, 0xEC6A64, 0xD48820, 0xA0AA00, 0x74C400, 0x4CD020, 0x38CC6C, 0x38B4CC, 0x3C3C3C, 0x000000, 0x000000,
    0xECEEEC, 0xA8CCEC, 0xBCBCEC, 0xD4B2EC, 0xECAEEC, 0xECAED4, 0xECB4B0, 0xE4C490, 0xCCD278, 0xB4DE78, 0xA8E290, 0x98E2B4, 0xA0D6E4, 0xA0A2A0, 0x000000, 0x000000
};

static constexpr uint32_t ResetDelay = 88974;

static constexpr uint16_t SecondaryOamOffset = 0x100;

struct PPUState
{
    Cart* Cartridge{nullptr};

    uint64_t Clock{0};
    int32_t Dot{1};
    int32_t Line{241};
    bool Even{true};
    bool SuppressNmi{false};
    bool InterruptActive{false};

    // Main Registers

    // Controller Register Fields
    bool PpuAddressIncrement{false};
    uint16_t BaseSpriteTableAddress{0};
    uint16_t BaseBackgroundTableAddress{0};
    bool SpriteSizeSwitch{false};
    bool NmiEnabled{false};

    // Mask Register Fields
    bool GrayScaleEnabled{false};
    bool ShowBackgroundLeft{false};
    bool ShowSpritesLeft{false};
    bool ShowBackground{false};
    bool ShowSprites{false};
    bool IntenseRed{false};
    bool IntenseGreen{false};
    bool IntenseBlue{false};

    // Status Register Fields
    uint8_t LowerBits;
    bool SpriteOverflowFlag{false};
    bool SpriteZeroHitFlag{false};
    bool NmiOccuredFlag{false};
    bool SpriteZeroOnNextLine{false};
    bool SpriteZeroOnCurrentLine{false};

    uint16_t OamAddress;
    uint16_t PpuAddress;
    uint16_t PpuTempAddress;
    uint8_t FineXScroll;

    bool AddressLatch{false};

    bool RenderingEnabled{false};
    bool RenderStateDelaySlot{false};

    // PPU Data Read Buffer
    uint8_t DataBuffer;

    // Oam Data Bus
    uint8_t OamData;
    uint8_t SecondaryOamIndex;

    // Object Attribute Memory (OAM)
    uint8_t Oam[0x120]{0};

    // Name Table RAM
    uint8_t NameTable0[0x400]{0};
    uint8_t NameTable1[0x400]{0};

    // Palette RAM
    uint8_t PaletteTable[0x20]{0};

    // Temporary Values
    uint8_t NameTableByte{0};
    uint8_t AttributeByte{0};
    uint8_t TileBitmapLow{0};
    uint8_t TileBitmapHigh{0};

    // Shift Registers, Latches and Counters
    uint16_t BackgroundShift0{0};
    uint16_t BackgroundShift1{0};
    uint16_t BackgroundAttributeShift0{0};
    uint16_t BackgroundAttributeShift1{0};
    uint8_t BackgroundAttribute{0};

    uint8_t SpriteCount{0};
    uint8_t SpriteShift0[8]{0};
    uint8_t SpriteShift1[8]{0};
    uint8_t SpriteAttribute[8]{0};
    int16_t SpriteCounter[8]{0};

    uint32_t FrameBufferIndex{0};
    uint32_t FrameBuffer[256 * 240]{0};

    uint16_t PpuBusAddress{0};

    uint8_t SpriteEvaluationCopyCycles{0};
    bool SpriteEvaluationRunning{true};
    bool SpriteEvaluationSpriteZero{true};
};

void SetBusAddress(PPUState* ppuState, uint16_t address)
{
    ppuState->PpuBusAddress = address & 0x3FFF;

    if (ppuState->PpuBusAddress < 0x3F00)
    {
        ppuState->Cartridge->SetPpuAddress(ppuState->PpuBusAddress);
    }
}

uint8_t ReadPalette(PPUState* ppuState, uint16_t address)
{
    // Ignore second nibble if addr is a multiple of 4
    if (address % 4 == 0)
    {
        address &= 0xFF0F;
    }

    return ppuState->PaletteTable[address % 0x20];
}

void WritePalette(PPUState* ppuState, uint8_t value, uint16_t address)
{
    // Ignore second nibble if addr is a multiple of 4
    if (address % 4 == 0)
    {
        address &= 0xFF0F;
    }

    ppuState->PaletteTable[address % 0x20] = value & 0x3F; // Ignore bits 6 and 7
}

uint8_t Read(PPUState* ppuState)
{
    if (ppuState->PpuBusAddress < 0x3F00)
    {
        return ppuState->Cartridge->PpuRead();
    }
    else
    {
        return ReadPalette(ppuState, ppuState->PpuBusAddress);
    }
}

void Write(PPUState* ppuState, uint8_t value)
{
    if (ppuState->PpuBusAddress < 0x3F00)
    {
        ppuState->Cartridge->PpuWrite(value);
    }
    else
    {
        WritePalette(ppuState, value, ppuState->PpuBusAddress);
    }
}

uint8_t Peek(PPUState* ppuState, uint16_t address)
{
    if (address < 0x3F00)
    {
        return ppuState->Cartridge->PpuPeek(address);
    }
    else
    {
        return ReadPalette(ppuState, address);
    }
}


void StepSpriteEvaluation(PPUState* ppuState)
{
    if (ppuState->Dot <= 64)
    {
        if ((ppuState->Dot % 2) != 0)
        {
            ppuState->OamData = 0xFF;
        }
        else
        {
            ppuState->Oam[SecondaryOamOffset + ppuState->SecondaryOamIndex] = ppuState->OamData;
            ppuState->SecondaryOamIndex = (ppuState->SecondaryOamIndex + 1) % 32;
        }
    }
    else
    {
        if ((ppuState->Dot % 2) != 0)
        {
            ppuState->OamData = ppuState->Oam[ppuState->OamAddress];
        }
        else
        {
            if (ppuState->SpriteEvaluationRunning)
            {
                uint8_t size = ppuState->SpriteSizeSwitch ? 16 : 8;
                if (ppuState->SpriteEvaluationCopyCycles == 0 && ppuState->OamData <= ppuState->Line && ppuState->OamData + size > ppuState->Line)
                {
                    if (ppuState->SpriteCount == 8)
                    {
                        ppuState->SpriteOverflowFlag = true;
                    }

                    if (ppuState->SpriteEvaluationSpriteZero)
                    {
                        ppuState->SpriteZeroOnNextLine = true;
                    }

                    ppuState->SpriteEvaluationCopyCycles = 4;
                }
                
                ppuState->SpriteEvaluationSpriteZero = false;

                if (ppuState->SpriteCount < 8)
                {
                    ppuState->Oam[SecondaryOamOffset + ppuState->SecondaryOamIndex] = ppuState->OamData;
                }
                else
                {
                    ppuState->OamData = ppuState->Oam[SecondaryOamOffset + ppuState->SecondaryOamIndex];
                }

                if (ppuState->SpriteEvaluationCopyCycles == 0)
                {
                    ppuState->OamAddress += 4;
                    if (ppuState->SpriteCount == 8)
                    {
                        ppuState->OamAddress = (ppuState->OamAddress & ~3) | ((ppuState->OamAddress + 1) & 3);
                    }
                }
                else
                {
                    ppuState->SpriteEvaluationCopyCycles--;

                    ppuState->OamAddress++;

                    if (ppuState->SpriteCount < 8)
                    {
                        ppuState->SecondaryOamIndex = (ppuState->SecondaryOamIndex + 1) % 32;
                        if (ppuState->SpriteEvaluationCopyCycles == 0)
                        {
                            ppuState->SpriteCount++;
                        }
                    }
                    else if (ppuState->SpriteEvaluationCopyCycles == 0)
                    {
                        ppuState->SpriteEvaluationRunning = false;
                    }

                    if (ppuState->OamAddress >= 0x100)
                    {
                        ppuState->SpriteEvaluationCopyCycles = 0;
                    }
                }

                if (ppuState->SpriteEvaluationCopyCycles == 0 && !ppuState->SpriteEvaluationRunning)
                {
                    ppuState->OamAddress = 0;
                }
            }
            else
            {
                ppuState->OamAddress = (ppuState->OamAddress + 4) % 0x100;
                ppuState->OamData = ppuState->Oam[SecondaryOamOffset + ppuState->SecondaryOamIndex];
            }
        }
    }
}

void DecodePixel(PPUState* ppuState, uint16_t colour)
{
   ppuState->FrameBuffer[ppuState->FrameBufferIndex++] = RgbLookupTable[colour];
}

void RenderPixel(PPUState* ppuState)
{
    uint16_t bgPixel = 0;
    uint16_t bgPaletteIndex = 0x3F00;

    if (ppuState->ShowBackground && (ppuState->ShowBackgroundLeft || ppuState->Dot > 8))
    {
        uint16_t bgAttribute = (((ppuState->BackgroundAttributeShift0 << ppuState->FineXScroll) & 0x8000) >> 13) | (((ppuState->BackgroundAttributeShift1 << ppuState->FineXScroll) & 0x8000) >> 12);
        bgPixel = (((ppuState->BackgroundShift0 << ppuState->FineXScroll) & 0x8000) >> 15) | (((ppuState->BackgroundShift1 << ppuState->FineXScroll) & 0x8000) >> 14);
        bgPaletteIndex |= bgAttribute | bgPixel;
    }

    uint16_t spPixel = 0;
    uint16_t spPaletteIndex = 0x3F10;
    bool spPriority = true;
    bool spriteFound = false;


    if (ppuState->ShowSprites && (ppuState->ShowSpritesLeft || ppuState->Dot > 8))
    {
        for (int i = 0; i < 8; ++i)
        {
            if (ppuState->SpriteCounter[i] <= 0 && ppuState->SpriteCounter[i] >= -7)
            {
                uint8_t spShift0 = ppuState->SpriteShift0[i];
                uint8_t spShift1 = ppuState->SpriteShift1[i];
                uint8_t spAttribute = ppuState->SpriteAttribute[i];

                uint16_t pixel;
                if (spAttribute & 0x40)
                {
                    pixel = (spShift0 & 0x1) | ((spShift1 & 0x1) << 1);
                }
                else
                {
                    pixel = ((spShift0 & 0x80) >> 7) | ((spShift1 & 0x80) >> 6);
                }

                if (pixel != 0)
                {
                    spPixel = pixel;
                    spPriority = !!(spAttribute & 0x20);
                    spPaletteIndex |= ((spAttribute & 0x03) << 2) | spPixel;

                    if (ppuState->SpriteZeroOnCurrentLine && !ppuState->SpriteZeroHitFlag && i == 0)
                    {
                        ppuState->SpriteZeroHitFlag = (ppuState->Dot > 1) & (ppuState->Dot != 256) & (spPixel != 0) & (bgPixel != 0);
                    }

                    break;
                }
            }
        }
    }

    uint16_t colour;
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
    else if (!spPriority)
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

    colour = ReadPalette(ppuState, paletteIndex);

    DecodePixel(ppuState, colour);
}

void RenderPixelIdle(PPUState* ppuState)
{
    uint16_t colour;

    if (ppuState->PpuAddress >= 0x3F00 && ppuState->PpuAddress <= 0x3FFF)
    {
        colour = ReadPalette(ppuState, ppuState->PpuAddress);
    }
    else
    {
        colour = ReadPalette(ppuState, 0x3F00);
    }

    DecodePixel(ppuState, colour);
}


void IncrementXScroll(PPUState* ppuState)
{
    if ((ppuState->PpuAddress & 0x001F) == 31) // Reach end of Name Table
    {
        ppuState->PpuAddress &= 0xFFE0; // Set Coarse X to 0
        ppuState->PpuAddress ^= 0x400; // Switch horizontal Name Table
    }
    else
    {
        ppuState->PpuAddress++; // Increment Coarse X
    }
}

void IncrementYScroll(PPUState* ppuState)
{
    if ((ppuState->PpuAddress & 0x7000) != 0x7000) // if the fine Y < 7
    {
        ppuState->PpuAddress += 0x1000; // increment fine Y
    }
    else
    {
        ppuState->PpuAddress &= 0x8FFF; // Set fine Y to 0
        uint16_t coarseY = (ppuState->PpuAddress & 0x03E0) >> 5; // get coarse Y

        if (coarseY == 29) // End of name table
        {
            coarseY = 0; // set coarse Y to 0
            ppuState->PpuAddress ^= 0x0800; // Switch Name Table
        }
        else if (coarseY == 31) // End of attribute table
        {
            coarseY = 0; // set coarse Y to 0
        }
        else
        {
            coarseY++; // Increment coarse Y
        }

        ppuState->PpuAddress = (ppuState->PpuAddress & 0xFC1F) | (coarseY << 5); // Combine values into new address
    }
}

void IncrementClock(PPUState* ppuState)
{
    if (ppuState->Line == 261 && !ppuState->Even && (ppuState->ShowSprites || ppuState->ShowBackground) && ppuState->Dot == 339)
    {
        ppuState->Dot = ppuState->Line = 0;
    }
    else if (ppuState->Line == 261 && ppuState->Dot == 340)
    {
        ppuState->Dot = ppuState->Line = 0;
    }
    else if (ppuState->Dot == 340)
    {
        ppuState->Dot = 0;
        ++ppuState->Line;
    }
    else
    {
        ++ppuState->Dot;
    }

    ++ppuState->Clock;
}

void LoadBackgroundShiftRegisters(PPUState* ppuState)
{
    ppuState->BackgroundShift0 = ppuState->BackgroundShift0 | ppuState->TileBitmapLow;
    ppuState->BackgroundShift1 = ppuState->BackgroundShift1 | ppuState->TileBitmapHigh;
    ppuState->BackgroundAttributeShift0 = ppuState->BackgroundAttributeShift0 | ((ppuState->AttributeByte & 0x1) ? 0xFF : 0x00);
    ppuState->BackgroundAttributeShift1 = ppuState->BackgroundAttributeShift1 | ((ppuState->AttributeByte & 0x2) ? 0xFF : 0x00);
}

void SetNameTableAddress(PPUState* ppuState)
{
    SetBusAddress(ppuState, 0x2000 | (ppuState->PpuAddress & 0x0FFF));
}

void DoNameTableFetch(PPUState* ppuState)
{
    ppuState->NameTableByte = Read(ppuState);
}

void SetBackgroundAttributeAddress(PPUState* ppuState)
{
    SetBusAddress(ppuState, 0x23C0 | (ppuState->PpuAddress & 0x0C00) | ((ppuState->PpuAddress >> 4) & 0x38) | ((ppuState->PpuAddress >> 2) & 0x07));
}

void DoBackgroundAttributeFetch(PPUState* ppuState)
{
    // Get the attribute byte, determines the palette to use when rendering
    ppuState->AttributeByte = Read(ppuState);
    uint8_t attributeShift = (((ppuState->PpuAddress & 0x0002) >> 1) | ((ppuState->PpuAddress & 0x0040) >> 5)) << 1;
    ppuState->AttributeByte = (ppuState->AttributeByte >> attributeShift) & 0x3;
}

void SetBackgroundLowByteAddress(PPUState* ppuState)
{
    uint8_t fineY = ppuState->PpuAddress >> 12; // Get fine y scroll bits from address
    uint16_t patternAddress = static_cast<uint16_t>(ppuState->NameTableByte) << 4; // Get pattern address, independent of the table

    SetBusAddress(ppuState, ppuState->BaseBackgroundTableAddress + patternAddress + fineY);
}

void DoBackgroundLowByteFetch(PPUState* ppuState)
{
    ppuState->TileBitmapLow = Read(ppuState);
}

void SetBackgroundHighByteAddress(PPUState* ppuState)
{
    uint8_t fineY = ppuState->PpuAddress >> 12; // Get fine y scroll
    uint16_t patternAddress = static_cast<uint16_t>(ppuState->NameTableByte) << 4; // Get pattern address, independent of the table

    SetBusAddress(ppuState, ppuState->BaseBackgroundTableAddress + patternAddress + fineY + 8);
}

void DoBackgroundHighByteFetch(PPUState* ppuState)
{
    ppuState->TileBitmapHigh = Read(ppuState);
}

void DoSpriteAttributeFetch(PPUState* ppuState)
{
    uint8_t sprite = (ppuState->Dot - 257) / 8;
    ppuState->SpriteAttribute[sprite] = ppuState->Oam[SecondaryOamOffset + (sprite * 4) + 2];
}

void DoSpriteXCoordinateFetch(PPUState* ppuState)
{
    uint8_t sprite = (ppuState->Dot - 257) / 8;
    ppuState->SpriteCounter[sprite] = ppuState->Oam[SecondaryOamOffset + (sprite * 4) + 3];
}

void SetSpriteLowByteAddress(PPUState* ppuState)
{
    uint8_t sprite = (ppuState->Dot - 257) / 8;
    if (sprite < ppuState->SpriteCount)
    {
        bool flipVertical = !!(0x80 & ppuState->SpriteAttribute[sprite]);
        uint8_t spriteY = ppuState->Oam[SecondaryOamOffset + (sprite * 4)];

        if (ppuState->SpriteSizeSwitch) // if spriteSize is 8x16
        {
            uint16_t base = (0x1 & ppuState->Oam[SecondaryOamOffset + (sprite * 4) + 1]) ? 0x1000 : 0; // Get base pattern table from bit 0 of pattern address
            uint16_t patternIndex = base + ((ppuState->Oam[SecondaryOamOffset + (sprite * 4) + 1] >> 1) << 5); // Index of the beginning of the pattern
            uint16_t offset = flipVertical ? 15 - (ppuState->Line - spriteY) : (ppuState->Line - spriteY); // Offset from base index
            if (offset >= 8) offset += 8;

            SetBusAddress(ppuState, patternIndex + offset);
        }
        else
        {
            uint16_t patternIndex = ppuState->BaseSpriteTableAddress + (ppuState->Oam[SecondaryOamOffset + (sprite * 4) + 1] << 4); // Index of the beginning of the pattern
            uint16_t offset = flipVertical ? 7 - (ppuState->Line - spriteY) : (ppuState->Line - spriteY); // Offset from base index

            SetBusAddress(ppuState, patternIndex + offset);
        }
    }
    else
    {
        if (ppuState->SpriteSizeSwitch)
        {
            SetBusAddress(ppuState, 0x1FF0);
        }
        else
        {
            SetBusAddress(ppuState, ppuState->BaseSpriteTableAddress + 0xFF0);
        }
    }
}

void DoSpriteLowByteFetch(PPUState* ppuState)
{
    uint8_t sprite = (ppuState->Dot - 257) / 8;
    uint8_t bitmap = Read(ppuState);

    ppuState->SpriteShift0[sprite] = sprite < ppuState->SpriteCount ? bitmap : 0x00; 
}

void SetSpriteHighByteAddress(PPUState* ppuState)
{
    uint8_t sprite = (ppuState->Dot - 257) / 8;
    if (sprite < ppuState->SpriteCount)
    {
        bool flipVertical = !!(0x80 & ppuState->SpriteAttribute[sprite]);
        uint8_t spriteY = ppuState->Oam[SecondaryOamOffset + (sprite * 4)];

        if (ppuState->SpriteSizeSwitch) // if spriteSize is 8x16
        {
            uint16_t base = (0x1 & ppuState->Oam[SecondaryOamOffset + (sprite * 4) + 1]) ? 0x1000 : 0; // Get base pattern table from bit 0 of pattern address
            uint16_t patternIndex = base + ((ppuState->Oam[SecondaryOamOffset + (sprite * 4) + 1] >> 1) << 5); // Index of the beginning of the pattern
            uint16_t offset = flipVertical ? 15 - (ppuState->Line - spriteY) : (ppuState->Line - spriteY); // Offset from base index
            if (offset >= 8) offset += 8;

            SetBusAddress(ppuState, patternIndex + offset + 8);
        }
        else
        {
            uint16_t patternIndex = ppuState->BaseSpriteTableAddress + (ppuState->Oam[SecondaryOamOffset + (sprite * 4) + 1] << 4); // Index of the beginning of the pattern
            uint16_t offset = flipVertical ? 7 - (ppuState->Line - spriteY) : (ppuState->Line - spriteY); // Offset from base index

            SetBusAddress(ppuState, patternIndex + offset + 8);
        }
    }
    else
    {
        if (ppuState->SpriteSizeSwitch)
        {
            SetBusAddress(ppuState, 0x1FF0);
        }
        else
        {
            SetBusAddress(ppuState, ppuState->BaseSpriteTableAddress + 0xFF0);
        }
    }
}

void DoSpriteHighByteFetch(PPUState* ppuState)
{
    uint8_t sprite = (ppuState->Dot - 257) / 8;
    uint8_t bitmap = Read(ppuState);

    ppuState->SpriteShift1[sprite] = sprite < ppuState->SpriteCount ? bitmap : 0x00; 
}

void Step(PPUState* ppuState)
{
    // Visible Lines and Pre-Render Line
    if ((ppuState->Line >= 0 && ppuState->Line <= 239) || ppuState->Line == 261)
    {
        if (ppuState->Dot == 1)
        {
            if (ppuState->Line == 261)
            {
                ppuState->NmiOccuredFlag = false;
                ppuState->InterruptActive = false;
                ppuState->SpriteZeroHitFlag = false;
                ppuState->SpriteOverflowFlag = false;
            }

            ppuState->SpriteCount = 0;
            ppuState->SpriteZeroOnCurrentLine = ppuState->SpriteZeroOnNextLine;
            ppuState->SpriteZeroOnNextLine = false;
            ppuState->SpriteEvaluationRunning = true;
            ppuState->SpriteEvaluationSpriteZero = true;
            ppuState->SecondaryOamIndex = 0;
        }

        if (ppuState->RenderingEnabled)
        {
            if ((ppuState->Dot >= 2 && ppuState->Dot <= 257) || (ppuState->Dot >= 322 && ppuState->Dot <= 337))
            {
                ppuState->BackgroundShift0 <<= 1;
                ppuState->BackgroundShift1 <<= 1;
                ppuState->BackgroundAttributeShift0 <<= 1;
                ppuState->BackgroundAttributeShift1 <<= 1;

                if ((ppuState->Dot - 1) % 8 == 0)
                {
                    LoadBackgroundShiftRegisters(ppuState);
                }

                if (ppuState->Dot <= 257)
                {
                    for (int i = 0; i < 8; ++i)
                    {
                        if (ppuState->SpriteCounter[i] <= 0 && ppuState->SpriteCounter[i] >= -7)
                        {
                            if (ppuState->SpriteAttribute[i] & 0x40)
                            {
                                ppuState->SpriteShift0[i] >>= 1;
                                ppuState->SpriteShift1[i] >>= 1;
                            }
                            else
                            {
                                ppuState->SpriteShift0[i] <<= 1;
                                ppuState->SpriteShift1[i] <<= 1;
                            }
                        }

                        --(ppuState->SpriteCounter[i]);
                    }
                }
            }

            if ((ppuState->Dot >= 1 && ppuState->Dot <= 256) || (ppuState->Dot >= 321 && ppuState->Dot <= 336))
            {
                uint8_t cycle = (ppuState->Dot - 1) % 8;
                switch (cycle)
                {
                case 0:
                    SetNameTableAddress(ppuState);
                    break;
                case 1:
                    DoNameTableFetch(ppuState);
                    break;
                case 2:
                    SetBackgroundAttributeAddress(ppuState);
                    break;
                case 3:
                    DoBackgroundAttributeFetch(ppuState);
                    break;
                case 4:
                    SetBackgroundLowByteAddress(ppuState);
                    break;
                case 5:
                    DoBackgroundLowByteFetch(ppuState);
                    break;
                case 6:
                    SetBackgroundHighByteAddress(ppuState);
                    break;
                case 7:
                    DoBackgroundHighByteFetch(ppuState);
                    IncrementXScroll(ppuState);
                    break;
                }

                if (ppuState->Dot == 256)
                {
                    IncrementYScroll(ppuState);
                }

                if (ppuState->Line != 261 && ppuState->Dot <= 256)
                {
                    RenderPixel(ppuState);
                }
            }
            else if (ppuState->Dot >= 257 && ppuState->Dot <= 320)
            {
                if (ppuState->Dot == 257)
                {
                    ppuState->PpuAddress = (ppuState->PpuAddress & 0x7BE0) | (ppuState->PpuTempAddress & 0x041F);
                }

                ppuState->OamAddress = 0;

                uint8_t cycle = (ppuState->Dot - 1) % 8;
                switch (cycle)
                {
                case 0:
                    SetNameTableAddress(ppuState);
                    break;
                case 1:
                    DoNameTableFetch(ppuState);
                    break;
                case 2:
                    SetBackgroundAttributeAddress(ppuState);
                    DoSpriteAttributeFetch(ppuState);
                    break;
                case 3:
                    DoBackgroundAttributeFetch(ppuState);
                    DoSpriteXCoordinateFetch(ppuState);
                    break;
                case 4:
                    SetSpriteLowByteAddress(ppuState);
                    break;
                case 5:
                    DoSpriteLowByteFetch(ppuState);
                    break;
                case 6:
                    SetSpriteHighByteAddress(ppuState);
                    break;
                case 7:
                    DoSpriteHighByteFetch(ppuState);
                    break;
                }
            }
            else if (ppuState->Dot >= 337 && ppuState->Dot <= 340)
            {
                uint8_t cycle = (ppuState->Dot - 1) % 4;
                switch (cycle)
                {
                    case 0: case 2:
                        SetNameTableAddress(ppuState);
                        break;
                    case 1: case 3:
                        DoNameTableFetch(ppuState);
                        break;
                }
            }

            // From dot 280 to 304 of the pre-render line copy all vertical position bits to ppuAddress from ppuTempAddress
            if (ppuState->Line == 261 && ppuState->Dot >= 280 && ppuState->Dot <= 304)
            {
                ppuState->PpuAddress = (ppuState->PpuAddress & 0x041F) | (ppuState->PpuTempAddress & 0x7BE0);
            }

            // Skip the last cycle of the pre-render line on odd frames
            if (!ppuState->Even && ppuState->Line == 261 && ppuState->Dot == 339)
            {
                ++(ppuState->Dot);
            }
        }
        else
        {
            if (ppuState->Line != 261 && ppuState->Dot >= 1 && ppuState->Dot <= 256)
            {
                RenderPixelIdle(ppuState);
            }
        }
    }
    // VBlank Lines
    else if (ppuState->Line >= 241 && ppuState->Line <= 260)
    {
        if (ppuState->Line == 241 && ppuState->Dot == 1 && ppuState->Clock > ResetDelay)
        {
            if (!ppuState->SuppressNmi)
            {
                ppuState->NmiOccuredFlag = true;
            }

            ppuState->SuppressNmi = false;
        }

        if (ppuState->Line != 241 || ppuState->Dot != 0)
        {
            ppuState->InterruptActive = ppuState->NmiOccuredFlag && ppuState->NmiEnabled;
        }
    }

    if (ppuState->RenderingEnabled && ppuState->Line < 240 && ppuState->Dot >= 1 && ppuState->Dot <= 256)
    {
        StepSpriteEvaluation(ppuState);
    }

    ppuState->RenderingEnabled = ppuState->RenderStateDelaySlot;
    ppuState->RenderStateDelaySlot = ppuState->ShowBackground || ppuState->ShowSprites;

    ppuState->Dot = (ppuState->Dot + 1) % 341;
    ++ppuState->Clock;



    
}


PPU::PPU(VideoBackend* vout, NESCallback* callback)
    : Cpu(nullptr)
    , Cartridge(nullptr)
    , VideoOut(vout)
    , Callback(callback)
    , ppuState(new PPUState)
    // , Clock(0)
    // , Dot(1)
    // , Line(241)
    // , Even(true)
    // , SuppressNmi(false)
    // , InterruptActive(false)
    // , PpuAddressIncrement(false)
    // , BaseSpriteTableAddress(0)
    // , BaseBackgroundTableAddress(0)
    // , SpriteSizeSwitch(false)
    // , NmiEnabled(false)
    // , GrayScaleEnabled(false)
    // , ShowBackgroundLeft(0)
    // , ShowSpritesLeft(0)
    // , ShowBackground(0)
    // , ShowSprites(0)
    // , IntenseRed(0)
    // , IntenseGreen(0)
    // , IntenseBlue(0)
    // , LowerBits(0x06)
    // , SpriteOverflowFlag(false)
    // , SpriteZeroHitFlag(false)
    // , NmiOccuredFlag(false)
    // , SpriteZeroOnNextLine(false)
    // , SpriteZeroOnCurrentLine(false)
    // , OamAddress(0)
    // , OamData(0)
    // , SecondaryOamIndex(0)
    // , PpuAddress(0)
    // , PpuTempAddress(0)
    // , FineXScroll(0)
    // , AddressLatch(false)
    // , RenderingEnabled(false)
    // , RenderStateDelaySlot(false)
    // , DataBuffer(0)
    // , NameTableByte(0)
    // , AttributeByte(0)
    // , TileBitmapLow(0)
    // , TileBitmapHigh(0)
    // , BackgroundShift0(0)
    // , BackgroundShift1(0)
    // , BackgroundAttributeShift0(0)
    // , BackgroundAttributeShift1(0)
    // , BackgroundAttribute(0)
    // , SpriteCount(0)
	// , FrameBufferIndex(0)
    // , SpriteEvaluationCopyCycles(0)
    // , SpriteEvaluationRunning(true)
    // , SpriteEvaluationSpriteZero(true)
{
    // memset(NameTable0, 0, sizeof(uint8_t) * 0x400);
    // memset(NameTable1, 0, sizeof(uint8_t) * 0x400);
    // memset(Oam, 0, sizeof(uint8_t) * 0x120);
    // memset(PaletteTable, 0, sizeof(uint8_t) * 0x20);
    // memset(SpriteShift0, 0, sizeof(uint8_t) * 8);
    // memset(SpriteShift1, 0, sizeof(uint8_t) * 8);
    // memset(SpriteAttribute, 0, sizeof(uint8_t) * 8);
    // memset(SpriteCounter, 0, sizeof(uint8_t) * 8);
}

PPU::~PPU() { delete ppuState; }

void PPU::AttachCPU(CPU* cpu)
{
    Cpu = cpu;
}

void PPU::AttachCart(Cart* cart)
{
    Cartridge = cart;
    ppuState->Cartridge = Cartridge;
}

int32_t PPU::GetCurrentDot()
{
    // Adjust the value of Dot to match Nintedulator's logs
    if (ppuState->Dot == 0)
    {
        return 340;
    }
    else
    {
        return ppuState->Dot - 1;
    }
}

int32_t PPU::GetCurrentScanline()
{
    // Adjust the value of Line to match Nintedulator's logs
    if (ppuState->Dot == 0)
    {
        if (ppuState->Line == 0)
        {
            return -1;
        }
        else
        {
            return ppuState->Line - 1;
        }
    }
    else
    {
        if (ppuState->Line == 261)
        {
            return -1;
        }
        else 
        {
            return ppuState->Line - 1;
        }
    }
}

uint64_t PPU::GetClock()
{
    return ppuState->Clock;
}

void PPU::Step()
{
    ::Step(ppuState);

    if (ppuState->Dot == 0)
    {
        ppuState->Line = (ppuState->Line + 1) % 262;

        if (ppuState->Line == 0) {
            // Toggle even flag
            ppuState->Even = !ppuState->Even;

            ppuState->FrameBufferIndex = 0;

            if (VideoOut != nullptr)
            {
                VideoOut->SubmitFrame(reinterpret_cast<uint8_t*>(ppuState->FrameBuffer));
            }

            if (Callback != nullptr) {
                Callback->OnFrameComplete();
            }
        }
    }
}


bool PPU::GetNMIActive()
{
    return ppuState->InterruptActive;
}

uint8_t PPU::ReadPPUStatus()
{
    uint8_t vB = static_cast<uint8_t>(ppuState->NmiOccuredFlag);
    uint8_t sp0 = static_cast<uint8_t>(ppuState->SpriteZeroHitFlag);
    uint8_t spOv = static_cast<uint8_t>(ppuState->SpriteOverflowFlag);

    if (ppuState->Line == 241 && ppuState->Dot == 1)
    {
        ppuState->SuppressNmi = true; // Suppress interrupt
    }

    if (ppuState->Line == 241 && ppuState->Dot >= 2 && ppuState->Dot <= 3)
    {
        ppuState->InterruptActive = false;
    }

    ppuState->NmiOccuredFlag = false;
    ppuState->AddressLatch = false;

    return  (vB << 7) | (sp0 << 6) | (spOv << 5) | ppuState->LowerBits;
}

uint8_t PPU::ReadOAMData()
{
    //return Oam[OamAddress];
    return ppuState->OamData;
}

uint8_t PPU::ReadPPUData()
{
    uint8_t value = ppuState->DataBuffer;
    ppuState->DataBuffer = Read(ppuState);
    ppuState->PpuAddressIncrement ? ppuState->PpuAddress = (ppuState->PpuAddress + 32) & 0x7FFF : ppuState->PpuAddress = (ppuState->PpuAddress + 1) & 0x7FFF;
    SetBusAddress(ppuState, ppuState->PpuAddress);

    return value;
}

void PPU::WritePPUCTRL(uint8_t M)
{
    if (Cpu->GetClock() > ResetDelay)
    {
        ppuState->PpuTempAddress = (ppuState->PpuTempAddress & 0x73FF) | ((0x3 & static_cast<uint16_t>(M)) << 10); // High bits of NameTable address
        ppuState->PpuAddressIncrement = ((0x4 & M) >> 2) != 0;
        ppuState->BaseSpriteTableAddress = 0x1000 * ((0x8 & M) >> 3);
        ppuState->BaseBackgroundTableAddress = 0x1000 * ((0x10 & M) >> 4);
        ppuState->SpriteSizeSwitch = ((0x20 & M) >> 5) != 0;
        ppuState->NmiEnabled = ((0x80 & M) >> 7) != 0;

        if (ppuState->NmiEnabled == false && ppuState->Line == 241 && (ppuState->Dot - 1) == 1)
        {
            ppuState->InterruptActive = false;
        }
    }

    ppuState->LowerBits = (0x1F & M);
}

void PPU::WritePPUMASK(uint8_t M)
{
    if (Cpu->GetClock() > ResetDelay)
    {
        ppuState->GrayScaleEnabled = (0x1 & M);
        ppuState->ShowBackgroundLeft = ((0x2 & M) >> 1) != 0;
        ppuState->ShowSpritesLeft = ((0x4 & M) >> 2) != 0;
        ppuState->ShowBackground = ((0x8 & M) >> 3) != 0;
        ppuState->ShowSprites = ((0x10 & M) >> 4) != 0;
        ppuState->IntenseRed = ((0x20 & M) >> 5) != 0;
        ppuState->IntenseGreen = ((0x40 & M) >> 6) != 0;
        ppuState->IntenseBlue = ((0x80 & M) >> 7) != 0;
    }

    ppuState->LowerBits = (0x1F & M);
}

void PPU::WriteOAMADDR(uint8_t M)
{
    ppuState->OamAddress = M;
    ppuState->OamData = ppuState->Oam[ppuState->OamAddress];
    ppuState->LowerBits = (0x1F & ppuState->OamAddress);
}

void PPU::WriteOAMDATA(uint8_t M)
{
    if (ppuState->RenderingEnabled && (ppuState->Line < 240 || ppuState->Line == 261))
    {
        return;
    }

    ppuState->OamData = M;
    ppuState->Oam[ppuState->OamAddress] = M;
    ppuState->OamAddress = (ppuState->OamAddress + 1) % 0x100;

    ppuState->LowerBits = (0x1F & M);
}

void PPU::WritePPUSCROLL(uint8_t M)
{
    if (Cpu->GetClock() > ResetDelay)
    {
        if (ppuState->AddressLatch)
        {
            ppuState->PpuTempAddress = (ppuState->PpuTempAddress & 0x0FFF) | ((0x7 & M) << 12);
            ppuState->PpuTempAddress = (ppuState->PpuTempAddress & 0x7C1F) | ((0xF8 & M) << 2);
        }
        else
        {
            ppuState->FineXScroll = static_cast<uint8_t>(0x7 & M);
            ppuState->PpuTempAddress = (ppuState->PpuTempAddress & 0x7FE0) | ((0xF8 & M) >> 3);
        }

        ppuState->AddressLatch = !ppuState->AddressLatch;
    }

    ppuState->LowerBits = static_cast<uint8_t>(0x1F & M);
}

void PPU::WritePPUADDR(uint8_t M)
{
    if (Cpu->GetClock() > ResetDelay)
    {      
        if (ppuState->AddressLatch)
        {
            ppuState->PpuTempAddress = (ppuState->PpuTempAddress & 0x7F00) | M;
            ppuState->PpuAddress = ppuState->PpuTempAddress;
            SetBusAddress(ppuState, ppuState->PpuAddress);
        }
        else
        {
            ppuState->PpuTempAddress = (ppuState->PpuTempAddress & 0x00FF) | ((0x3F & M) << 8);
        }
        
        ppuState->AddressLatch = !ppuState->AddressLatch;
    }

    ppuState->LowerBits = static_cast<uint8_t>(0x1F & M);
}

void PPU::WritePPUDATA(uint8_t M)
{
    Write(ppuState, M);
    ppuState->PpuAddressIncrement ? ppuState->PpuAddress = (ppuState->PpuAddress + 32) & 0x7FFF : ppuState->PpuAddress = (ppuState->PpuAddress + 1) & 0x7FFF;
    SetBusAddress(ppuState, ppuState->PpuAddress);

    ppuState->LowerBits = (0x1F & M);
}

int PPU::GetFrameRate()
{
    return 0;
}

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
            uint8_t ntByte = Peek(ppuState, 0x2000 | address);
            uint8_t atShift = (((address & 0x0002) >> 1) | ((address & 0x0040) >> 5)) * 2;
            uint8_t atByte = Peek(ppuState, 0x23C0 | (address & 0x0C00) | ((address >> 4) & 0x38) | ((address >> 2) & 0x07));
            atByte = (atByte >> atShift) & 0x3;
            uint16_t patternAddress = static_cast<uint16_t>(ntByte) * 16;

            for (uint32_t h = 0; h < 8; ++h)
            {
                uint8_t tileLow = Peek(ppuState, ppuState->BaseBackgroundTableAddress + patternAddress + h);
                uint8_t tileHigh = Peek(ppuState, ppuState->BaseBackgroundTableAddress + patternAddress + h + 8);

                for (uint32_t g = 0; g < 8; ++g)
                {
                    uint16_t pixel = 0x0003 & ((((tileLow << g) & 0x80) >> 7) | (((tileHigh << g) & 0x80) >> 6));
                    uint16_t paletteIndex = 0x3F00 | (atByte << 2) | pixel;
                    if ((paletteIndex & 0x3) == 0) paletteIndex = 0x3F00;
                    uint32_t rgb = RgbLookupTable[Peek(ppuState, paletteIndex)];
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
                uint8_t tileLow = Peek(ppuState, tableIndex + patternIndex + g);
                uint8_t tileHigh = Peek(ppuState, tableIndex + patternIndex + g + 8);

                for (uint32_t h = 0; h < 8; ++h)
                {
                    uint16_t pixel = 0x0003 & ((((tileLow << h) & 0x80) >> 7) | (((tileHigh << h) & 0x80) >> 6));
                    uint16_t paletteIndex = (0x3F00 + (4 * (palette % 8))) | pixel;
                    uint32_t rgb = RgbLookupTable[Peek(ppuState, paletteIndex)];
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
                uint32_t rgb = RgbLookupTable[Peek(ppuState, paletteAddress)];
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
    uint8_t byteOne = ppuState->Oam[(0x4 * sprite) + 1];
    uint8_t byteTwo = ppuState->Oam[(0x4 * sprite) + 2];

    uint16_t tableIndex = ppuState->BaseSpriteTableAddress;
    uint16_t patternIndex = byteOne * 16;
    uint8_t palette = (byteTwo & 0x03) + 4;

    for (uint32_t i = 0; i < 8; ++i)
    {
        uint8_t tileLow = Peek(ppuState, tableIndex + patternIndex + i);
        uint8_t tileHigh = Peek(ppuState, tableIndex + patternIndex + i + 8);

        for (uint32_t f = 0; f < 8; ++f)
        {
            uint16_t pixel = 0x0003 & ((((tileLow << f) & 0x80) >> 7) | (((tileHigh << f) & 0x80) >> 6));
            uint16_t paletteIndex = (0x3F00 + (4 * (palette % 8))) | pixel;
            uint32_t rgb = RgbLookupTable[Peek(ppuState, paletteIndex)];
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

StateSave::Ptr PPU::SaveState()
{
    StateSave::Ptr state = StateSave::New();

    // state->StoreValue(Clock);
    // state->StoreValue(Dot);
    // state->StoreValue(Line);
    // state->StoreValue(BaseSpriteTableAddress);
    // state->StoreValue(BaseBackgroundTableAddress);
    // state->StoreValue(LowerBits);
    // state->StoreValue(OamAddress);
    // state->StoreValue(PpuAddress);
    // state->StoreValue(PpuTempAddress);
    // state->StoreValue(FineXScroll);
    // state->StoreValue(DataBuffer);
    // state->StoreValue(Oam);
    // state->StoreValue(NameTable0);
    // state->StoreValue(NameTable1);
    // state->StoreValue(PaletteTable);
    // state->StoreValue(NameTableByte);
    // state->StoreValue(AttributeByte);
    // state->StoreValue(TileBitmapLow);
    // state->StoreValue(TileBitmapHigh);
    // state->StoreValue(BackgroundShift0);
    // state->StoreValue(BackgroundShift1);
    // state->StoreValue(BackgroundAttributeShift0);
    // state->StoreValue(BackgroundAttributeShift1);
    // state->StoreValue(BackgroundAttribute);
    // state->StoreValue(SpriteCount);
    // state->StoreValue(SpriteShift0);
    // state->StoreValue(SpriteShift1);
    // state->StoreValue(SpriteAttribute);
    // state->StoreValue(SpriteCounter);
    // state->StoreValue(FrameBufferIndex);

    // state->StorePackedValues(
    //     Even, 
    //     SuppressNmi,
    //     InterruptActive,
    //     PpuAddressIncrement,
    //     SpriteSizeSwitch,
    //     NmiEnabled,
    //     GrayScaleEnabled,
    //     ShowBackgroundLeft
    // );

    // state->StorePackedValues(
    //     ShowSpritesLeft,
    //     ShowBackground,
    //     ShowSprites,
    //     IntenseRed,
    //     IntenseGreen,
    //     IntenseBlue,
    //     SpriteOverflowFlag,
    //     SpriteZeroHitFlag
    // );

    // state->StorePackedValues(
    //     NmiOccuredFlag,
    //     SpriteZeroSecondaryOamFlag,
    //     AddressLatch,
    //     RenderingEnabled,
    //     RenderStateDelaySlot
    // );

    return state;
}

void PPU::LoadState(const StateSave::Ptr& state)
{
    // state->ExtractValue(Clock);
    // state->ExtractValue(Dot);
    // state->ExtractValue(Line);
    // state->ExtractValue(BaseSpriteTableAddress);
    // state->ExtractValue(BaseBackgroundTableAddress);
    // state->ExtractValue(LowerBits);
    // state->ExtractValue(OamAddress);
    // state->ExtractValue(PpuAddress);
    // state->ExtractValue(PpuTempAddress);
    // state->ExtractValue(FineXScroll);
    // state->ExtractValue(DataBuffer);
    // state->ExtractValue(Oam);
    // state->ExtractValue(NameTable0);
    // state->ExtractValue(NameTable1);
    // state->ExtractValue(PaletteTable);
    // state->ExtractValue(NameTableByte);
    // state->ExtractValue(AttributeByte);
    // state->ExtractValue(TileBitmapLow);
    // state->ExtractValue(TileBitmapHigh);
    // state->ExtractValue(BackgroundShift0);
    // state->ExtractValue(BackgroundShift1);
    // state->ExtractValue(BackgroundAttributeShift0);
    // state->ExtractValue(BackgroundAttributeShift1);
    // state->ExtractValue(BackgroundAttribute);
    // state->ExtractValue(SpriteCount);
    // state->ExtractValue(SpriteShift0);
    // state->ExtractValue(SpriteShift1);
    // state->ExtractValue(SpriteAttribute);
    // state->ExtractValue(SpriteCounter);
    // state->ExtractValue(FrameBufferIndex);

    // state->ExtractPackedValues(
    //     Even, 
    //     SuppressNmi,
    //     InterruptActive,
    //     PpuAddressIncrement,
    //     SpriteSizeSwitch,
    //     NmiEnabled,
    //     GrayScaleEnabled,
    //     ShowBackgroundLeft
    // );

    // state->ExtractPackedValues(
    //     ShowSpritesLeft,
    //     ShowBackground,
    //     ShowSprites,
    //     IntenseRed,
    //     IntenseGreen,
    //     IntenseBlue,
    //     SpriteOverflowFlag,
    //     SpriteZeroHitFlag
    // );

    // state->ExtractPackedValues(
    //     NmiOccuredFlag,
    //     SpriteZeroSecondaryOamFlag,
    //     AddressLatch,
    //     RenderingEnabled,
    //     RenderStateDelaySlot
    // );
}

uint8_t PPU::ReadNameTable0(uint16_t address)
{
    return ppuState->NameTable0[address];
}

uint8_t PPU::ReadNameTable1(uint16_t address)
{
    return ppuState->NameTable1[address];
}

void PPU::WriteNameTable0(uint8_t M, uint16_t address)
{
    ppuState->NameTable0[address] = M;
}

void PPU::WriteNameTable1(uint8_t M, uint16_t address)
{
    ppuState->NameTable1[address] = M;
}
