#include <cstring>

#include "ppu.h"
#include "cpu.h"
#include "apu.h"
#include "video/video_backend_base.h"

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
    uint64_t Clock{0};
    uint32_t Dot{1};
    uint32_t Line{241};
    bool Even{false};
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
    uint16_t PpuBusAddress{0};

    uint8_t SpriteEvaluationCopyCycles{0};
    bool SpriteEvaluationRunning{true};
    bool SpriteEvaluationSpriteZero{true};

    void SetBusAddress(Cart& cart, uint16_t address);
    uint8_t ReadPalette(uint16_t address);
    void WritePalette(uint8_t value, uint16_t address);
    uint8_t Read(Cart& cart);
    void Write(Cart& cart, uint8_t value);
    uint8_t Peek(Cart& cart, uint16_t address);
    void StepSpriteEvaluation();
    uint32_t DecodePixel(uint16_t colour);
    uint32_t RenderPixel();
    uint32_t RenderPixelIdle();
    void IncrementXScroll();
    void IncrementYScroll();
    void LoadBackgroundShiftRegisters();
    void SetNameTableAddress(Cart& cart);
    void DoNameTableFetch(Cart& cart);
    void SetBackgroundAttributeAddress(Cart& cart);
    void DoBackgroundAttributeFetch(Cart& cart);
    void SetBackgroundLowByteAddress(Cart& cart);
    void DoBackgroundLowByteFetch(Cart& cart);
    void SetBackgroundHighByteAddress(Cart& cart);
    void DoBackgroundHighByteFetch(Cart& cart);
    void DoSpriteAttributeFetch();
    void DoSpriteXCoordinateFetch();
    void SetSpriteLowByteAddress(Cart& cart);
    void DoSpriteLowByteFetch(Cart& cart);
    void SetSpriteHighByteAddress(Cart& cart);
    void DoSpriteHighByteFetch(Cart& cart);
    void Step(uint32_t* frameBuffer, Cart& cart);
    uint8_t ReadPPUStatus();
    uint8_t ReadOAMData();
    uint8_t ReadPPUData(Cart& cart);
    void WritePPUCTRL(uint8_t M);
    void WritePPUMASK(uint8_t M);
    void WriteOAMADDR(uint8_t M);
    void WriteOAMDATA(uint8_t M);
    void WritePPUSCROLL(uint8_t M);
    void WritePPUADDR(Cart& cart, uint8_t M);
    void WritePPUDATA(Cart& cart, uint8_t M);
};

void PPUState::SetBusAddress(Cart& cart, uint16_t address)
{
    PpuBusAddress = address & 0x3FFF;

    if (PpuBusAddress < 0x3F00)
    {
        cart.SetPpuAddress(PpuBusAddress);
    }
}

uint8_t PPUState::ReadPalette(uint16_t address)
{
    // Ignore second nibble if addr is a multiple of 4
    if (address % 4 == 0)
    {
        address &= 0xFF0F;
    }

    return PaletteTable[address % 0x20];
}

void PPUState::WritePalette(uint8_t value, uint16_t address)
{
    // Ignore second nibble if addr is a multiple of 4
    if (address % 4 == 0)
    {
        address &= 0xFF0F;
    }

    PaletteTable[address % 0x20] = value & 0x3F; // Ignore bits 6 and 7
}

uint8_t PPUState::Read(Cart& cart)
{
    if (PpuBusAddress < 0x3F00)
    {
        return cart.PpuRead();
    }
    else
    {
        return ReadPalette(PpuBusAddress);
    }
}

void PPUState::Write(Cart& cart, uint8_t value)
{
    if (PpuBusAddress < 0x3F00)
    {
        cart.PpuWrite(value);
    }
    else
    {
        WritePalette(value, PpuBusAddress);
    }
}

uint8_t PPUState::Peek(Cart& cart, uint16_t address)
{
    if (address < 0x3F00)
    {
        return cart.PpuPeek(address);
    }
    else
    {
        return ReadPalette(address);
    }
}


void PPUState::StepSpriteEvaluation()
{
    if (Dot <= 64)
    {
        if ((Dot % 2) != 0)
        {
            OamData = 0xFF;
        }
        else
        {
            Oam[SecondaryOamOffset + SecondaryOamIndex] = OamData;
            SecondaryOamIndex = (SecondaryOamIndex + 1) % 32;
        }
    }
    else
    {
        if ((Dot % 2) != 0)
        {
            OamData = Oam[OamAddress];
        }
        else
        {
            if (SpriteEvaluationRunning)
            {
                uint8_t size = SpriteSizeSwitch ? 16 : 8;
                if (SpriteEvaluationCopyCycles == 0 && OamData <= Line && OamData + size > Line)
                {
                    if (SpriteCount == 8)
                    {
                        SpriteOverflowFlag = true;
                    }

                    if (SpriteEvaluationSpriteZero)
                    {
                        SpriteZeroOnNextLine = true;
                    }

                    SpriteEvaluationCopyCycles = 4;
                }
                
                SpriteEvaluationSpriteZero = false;

                if (SpriteCount < 8)
                {
                    Oam[SecondaryOamOffset + SecondaryOamIndex] = OamData;
                }
                else
                {
                    OamData = Oam[SecondaryOamOffset + SecondaryOamIndex];
                }

                if (SpriteEvaluationCopyCycles == 0)
                {
                    OamAddress += 4;
                    if (SpriteCount == 8)
                    {
                        OamAddress = (OamAddress & ~3) | ((OamAddress + 1) & 3);
                    }
                }
                else
                {
                    SpriteEvaluationCopyCycles--;

                    OamAddress++;

                    if (SpriteCount < 8)
                    {
                        SecondaryOamIndex = (SecondaryOamIndex + 1) % 32;
                        if (SpriteEvaluationCopyCycles == 0)
                        {
                            SpriteCount++;
                        }
                    }
                    else if (SpriteEvaluationCopyCycles == 0)
                    {
                        SpriteEvaluationRunning = false;
                    }

                    if (OamAddress >= 0x100)
                    {
                        SpriteEvaluationCopyCycles = 0;
                    }
                }

                if (SpriteEvaluationCopyCycles == 0 && !SpriteEvaluationRunning)
                {
                    OamAddress = 0;
                }
            }
            else
            {
                OamAddress = (OamAddress + 4) % 0x100;
                OamData = Oam[SecondaryOamOffset + SecondaryOamIndex];
            }
        }
    }
}

uint32_t PPUState::DecodePixel(uint16_t colour)
{
   //FrameBuffer[FrameBufferIndex++] = RgbLookupTable[colour];
    return RgbLookupTable[colour];
}

uint32_t PPUState::RenderPixel()
{
    uint16_t bgPixel = 0;
    uint16_t bgPaletteIndex = 0x3F00;

    if (ShowBackground && (ShowBackgroundLeft || Dot > 8))
    {
        uint16_t bgAttribute = (((BackgroundAttributeShift0 << FineXScroll) & 0x8000) >> 13) | (((BackgroundAttributeShift1 << FineXScroll) & 0x8000) >> 12);
        bgPixel = (((BackgroundShift0 << FineXScroll) & 0x8000) >> 15) | (((BackgroundShift1 << FineXScroll) & 0x8000) >> 14);
        bgPaletteIndex |= bgAttribute | bgPixel;
    }

    uint16_t spPixel = 0;
    uint16_t spPaletteIndex = 0x3F10;
    bool spPriority = true;
    bool spriteFound = false;


    if (ShowSprites && (ShowSpritesLeft || Dot > 8))
    {
        for (int i = 0; i < 8; ++i)
        {
            if (SpriteCounter[i] <= 0 && SpriteCounter[i] >= -7)
            {
                uint8_t spShift0 = SpriteShift0[i];
                uint8_t spShift1 = SpriteShift1[i];
                uint8_t spAttribute = SpriteAttribute[i];

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

                    if (SpriteZeroOnCurrentLine && !SpriteZeroHitFlag && i == 0)
                    {
                        SpriteZeroHitFlag = (Dot > 1) & (Dot != 256) & (spPixel != 0) & (bgPixel != 0);
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

    colour = ReadPalette(paletteIndex);

    return DecodePixel(colour);
}

uint32_t  PPUState::RenderPixelIdle()
{
    uint16_t colour;

    if (PpuAddress >= 0x3F00 && PpuAddress <= 0x3FFF)
    {
        colour = ReadPalette(PpuAddress);
    }
    else
    {
        colour = ReadPalette(0x3F00);
    }

    return DecodePixel(colour);
}


void PPUState::IncrementXScroll()
{
    if ((PpuAddress & 0x001F) == 31) // Reach end of Name Table
    {
        PpuAddress &= 0xFFE0; // Set Coarse X to 0
        PpuAddress ^= 0x400; // Switch horizontal Name Table
    }
    else
    {
        PpuAddress++; // Increment Coarse X
    }
}

void PPUState::IncrementYScroll()
{
    if ((PpuAddress & 0x7000) != 0x7000) // if the fine Y < 7
    {
        PpuAddress += 0x1000; // increment fine Y
    }
    else
    {
        PpuAddress &= 0x8FFF; // Set fine Y to 0
        uint16_t coarseY = (PpuAddress & 0x03E0) >> 5; // get coarse Y

        if (coarseY == 29) // End of name table
        {
            coarseY = 0; // set coarse Y to 0
            PpuAddress ^= 0x0800; // Switch Name Table
        }
        else if (coarseY == 31) // End of attribute table
        {
            coarseY = 0; // set coarse Y to 0
        }
        else
        {
            coarseY++; // Increment coarse Y
        }

        PpuAddress = (PpuAddress & 0xFC1F) | (coarseY << 5); // Combine values into new address
    }
}

void PPUState::LoadBackgroundShiftRegisters()
{
    BackgroundShift0 = BackgroundShift0 | TileBitmapLow;
    BackgroundShift1 = BackgroundShift1 | TileBitmapHigh;
    BackgroundAttributeShift0 = BackgroundAttributeShift0 | ((AttributeByte & 0x1) ? 0xFF : 0x00);
    BackgroundAttributeShift1 = BackgroundAttributeShift1 | ((AttributeByte & 0x2) ? 0xFF : 0x00);
}

void PPUState::SetNameTableAddress(Cart& cart)
{
    SetBusAddress(cart, 0x2000 | (PpuAddress & 0x0FFF));
}

void PPUState::DoNameTableFetch(Cart& cart)
{
    NameTableByte = Read(cart);
}

void PPUState::SetBackgroundAttributeAddress(Cart& cart)
{
    SetBusAddress(cart, 0x23C0 | (PpuAddress & 0x0C00) | ((PpuAddress >> 4) & 0x38) | ((PpuAddress >> 2) & 0x07));
}

void PPUState::DoBackgroundAttributeFetch(Cart& cart)
{
    // Get the attribute byte, determines the palette to use when rendering
    AttributeByte = Read(cart);
    uint8_t attributeShift = (((PpuAddress & 0x0002) >> 1) | ((PpuAddress & 0x0040) >> 5)) << 1;
    AttributeByte = (AttributeByte >> attributeShift) & 0x3;
}

void PPUState::SetBackgroundLowByteAddress(Cart& cart)
{
    uint8_t fineY = PpuAddress >> 12; // Get fine y scroll bits from address
    uint16_t patternAddress = static_cast<uint16_t>(NameTableByte) << 4; // Get pattern address, independent of the table

    SetBusAddress(cart, BaseBackgroundTableAddress + patternAddress + fineY);
}

void PPUState::DoBackgroundLowByteFetch(Cart& cart)
{
    TileBitmapLow = Read(cart);
}

void PPUState::SetBackgroundHighByteAddress(Cart& cart)
{
    uint8_t fineY = PpuAddress >> 12; // Get fine y scroll
    uint16_t patternAddress = static_cast<uint16_t>(NameTableByte) << 4; // Get pattern address, independent of the table

    SetBusAddress(cart, BaseBackgroundTableAddress + patternAddress + fineY + 8);
}

void PPUState::DoBackgroundHighByteFetch(Cart& cart)
{
    TileBitmapHigh = Read(cart);
}

void PPUState::DoSpriteAttributeFetch()
{
    uint8_t sprite = (Dot - 257) / 8;
    SpriteAttribute[sprite] = Oam[SecondaryOamOffset + (sprite * 4) + 2];
}

void PPUState::DoSpriteXCoordinateFetch()
{
    uint8_t sprite = (Dot - 257) / 8;
    SpriteCounter[sprite] = Oam[SecondaryOamOffset + (sprite * 4) + 3];
}

void PPUState::SetSpriteLowByteAddress(Cart& cart)
{
    uint8_t sprite = (Dot - 257) / 8;
    if (sprite < SpriteCount)
    {
        bool flipVertical = !!(0x80 & SpriteAttribute[sprite]);
        uint8_t spriteY = Oam[SecondaryOamOffset + (sprite * 4)];

        if (SpriteSizeSwitch) // if spriteSize is 8x16
        {
            uint16_t base = (0x1 & Oam[SecondaryOamOffset + (sprite * 4) + 1]) ? 0x1000 : 0; // Get base pattern table from bit 0 of pattern address
            uint16_t patternIndex = base + ((Oam[SecondaryOamOffset + (sprite * 4) + 1] >> 1) << 5); // Index of the beginning of the pattern
            uint16_t offset = flipVertical ? 15 - (Line - spriteY) : (Line - spriteY); // Offset from base index
            if (offset >= 8) offset += 8;

            SetBusAddress(cart, patternIndex + offset);
        }
        else
        {
            uint16_t patternIndex = BaseSpriteTableAddress + (Oam[SecondaryOamOffset + (sprite * 4) + 1] << 4); // Index of the beginning of the pattern
            uint16_t offset = flipVertical ? 7 - (Line - spriteY) : (Line - spriteY); // Offset from base index

            SetBusAddress(cart, patternIndex + offset);
        }
    }
    else
    {
        if (SpriteSizeSwitch)
        {
            SetBusAddress(cart, 0x1FF0);
        }
        else
        {
            SetBusAddress(cart, BaseSpriteTableAddress + 0xFF0);
        }
    }
}

void PPUState::DoSpriteLowByteFetch(Cart& cart)
{
    uint8_t sprite = (Dot - 257) / 8;
    uint8_t bitmap = Read(cart);

    SpriteShift0[sprite] = sprite < SpriteCount ? bitmap : 0x00; 
}

void PPUState::SetSpriteHighByteAddress(Cart& cart)
{
    uint8_t sprite = (Dot - 257) / 8;
    if (sprite < SpriteCount)
    {
        bool flipVertical = !!(0x80 & SpriteAttribute[sprite]);
        uint8_t spriteY = Oam[SecondaryOamOffset + (sprite * 4)];

        if (SpriteSizeSwitch) // if spriteSize is 8x16
        {
            uint16_t base = (0x1 & Oam[SecondaryOamOffset + (sprite * 4) + 1]) ? 0x1000 : 0; // Get base pattern table from bit 0 of pattern address
            uint16_t patternIndex = base + ((Oam[SecondaryOamOffset + (sprite * 4) + 1] >> 1) << 5); // Index of the beginning of the pattern
            uint16_t offset = flipVertical ? 15 - (Line - spriteY) : (Line - spriteY); // Offset from base index
            if (offset >= 8) offset += 8;

            SetBusAddress(cart, patternIndex + offset + 8);
        }
        else
        {
            uint16_t patternIndex = BaseSpriteTableAddress + (Oam[SecondaryOamOffset + (sprite * 4) + 1] << 4); // Index of the beginning of the pattern
            uint16_t offset = flipVertical ? 7 - (Line - spriteY) : (Line - spriteY); // Offset from base index

            SetBusAddress(cart, patternIndex + offset + 8);
        }
    }
    else
    {
        if (SpriteSizeSwitch)
        {
            SetBusAddress(cart, 0x1FF0);
        }
        else
        {
            SetBusAddress(cart, BaseSpriteTableAddress + 0xFF0);
        }
    }
}

void PPUState::DoSpriteHighByteFetch(Cart& cart)
{
    uint8_t sprite = (Dot - 257) / 8;
    uint8_t bitmap = Read(cart);

    SpriteShift1[sprite] = sprite < SpriteCount ? bitmap : 0x00; 
}

void PPUState::Step(uint32_t* frameBuffer, Cart& cart)
{
    // Visible Lines and Pre-Render Line
    if ((Line >= 0 && Line <= 239) || Line == 261)
    {
        if (Dot == 1)
        {
            if (Line == 261)
            {
                NmiOccuredFlag = false;
                InterruptActive = false;
                SpriteZeroHitFlag = false;
                SpriteOverflowFlag = false;
                Even = !Even;
            }

            SpriteCount = 0;
            SpriteZeroOnCurrentLine = SpriteZeroOnNextLine;
            SpriteZeroOnNextLine = false;
            SpriteEvaluationRunning = true;
            SpriteEvaluationSpriteZero = true;
            SecondaryOamIndex = 0;
        }

        if (RenderingEnabled)
        {
            if ((Dot >= 2 && Dot <= 257) || (Dot >= 322 && Dot <= 337))
            {
                BackgroundShift0 <<= 1;
                BackgroundShift1 <<= 1;
                BackgroundAttributeShift0 <<= 1;
                BackgroundAttributeShift1 <<= 1;

                if ((Dot - 1) % 8 == 0)
                {
                    LoadBackgroundShiftRegisters();
                }

                if (Dot <= 257)
                {
                    for (int i = 0; i < 8; ++i)
                    {
                        if (SpriteCounter[i] <= 0 && SpriteCounter[i] >= -7)
                        {
                            if (SpriteAttribute[i] & 0x40)
                            {
                                SpriteShift0[i] >>= 1;
                                SpriteShift1[i] >>= 1;
                            }
                            else
                            {
                                SpriteShift0[i] <<= 1;
                                SpriteShift1[i] <<= 1;
                            }
                        }

                        --SpriteCounter[i];
                    }
                }
            }

            if ((Dot >= 1 && Dot <= 256) || (Dot >= 321 && Dot <= 336))
            {
                uint8_t cycle = (Dot - 1) % 8;
                switch (cycle)
                {
                case 0:
                    SetNameTableAddress(cart);
                    break;
                case 1:
                    DoNameTableFetch(cart);
                    break;
                case 2:
                    SetBackgroundAttributeAddress(cart);
                    break;
                case 3:
                    DoBackgroundAttributeFetch(cart);
                    break;
                case 4:
                    SetBackgroundLowByteAddress(cart);
                    break;
                case 5:
                    DoBackgroundLowByteFetch(cart);
                    break;
                case 6:
                    SetBackgroundHighByteAddress(cart);
                    break;
                case 7:
                    DoBackgroundHighByteFetch(cart);
                    IncrementXScroll();
                    break;
                }

                if (Dot == 256)
                {
                    IncrementYScroll();
                }

                if (Line != 261 && Dot <= 256)
                {
                    frameBuffer[(Line * 256) + (Dot - 1)] = RenderPixel();
                }
            }
            else if (Dot >= 257 && Dot <= 320)
            {
                if (Dot == 257)
                {
                    PpuAddress = (PpuAddress & 0x7BE0) | (PpuTempAddress & 0x041F);
                }

                OamAddress = 0;

                uint8_t cycle = (Dot - 1) % 8;
                switch (cycle)
                {
                case 0:
                    SetNameTableAddress(cart);
                    break;
                case 1:
                    DoNameTableFetch(cart);
                    break;
                case 2:
                    SetBackgroundAttributeAddress(cart);
                    DoSpriteAttributeFetch();
                    break;
                case 3:
                    DoBackgroundAttributeFetch(cart);
                    DoSpriteXCoordinateFetch();
                    break;
                case 4:
                    SetSpriteLowByteAddress(cart);
                    break;
                case 5:
                    DoSpriteLowByteFetch(cart);
                    break;
                case 6:
                    SetSpriteHighByteAddress(cart);
                    break;
                case 7:
                    DoSpriteHighByteFetch(cart);
                    break;
                }
            }
            else if (Dot >= 337 && Dot <= 340)
            {
                uint8_t cycle = (Dot - 1) % 4;
                switch (cycle)
                {
                    case 0: case 2:
                        SetNameTableAddress(cart);
                        break;
                    case 1: case 3:
                        DoNameTableFetch(cart);
                        break;
                }
            }

            // From dot 280 to 304 of the pre-render line copy all vertical position bits to ppuAddress from ppuTempAddress
            if (Line == 261 && Dot >= 280 && Dot <= 304)
            {
                PpuAddress = (PpuAddress & 0x041F) | (PpuTempAddress & 0x7BE0);
            }

            // Skip the last cycle of the pre-render line on odd frames
            if (!Even && Line == 261 && Dot == 339)
            {
                ++Dot;
            }
        }
        else
        {
            if (Line != 261 && Dot >= 1 && Dot <= 256)
            {
                frameBuffer[(Line * 256) + (Dot - 1)] = RenderPixelIdle();
            }
        }
    }
    // VBlank Lines
    else if (Line >= 241 && Line <= 260)
    {
        if (Line == 241 && Dot == 1 && Clock > ResetDelay)
        {
            if (!SuppressNmi)
            {
                NmiOccuredFlag = true;
            }

            SuppressNmi = false;
        }

        if (Line != 241 || Dot != 0)
        {
            InterruptActive = NmiOccuredFlag && NmiEnabled;
        }
    }

    if (RenderingEnabled && Line < 240 && Dot >= 1 && Dot <= 256)
    {
        StepSpriteEvaluation();
    }

    RenderingEnabled = RenderStateDelaySlot;
    RenderStateDelaySlot = ShowBackground || ShowSprites;

    Dot = (Dot + 1) % 341;
    if (Dot == 0)
    {
       Line = (Line + 1) % 262;
    }

    ++Clock;
}

uint8_t PPUState::ReadPPUStatus()
{
    uint8_t vB = static_cast<uint8_t>(NmiOccuredFlag);
    uint8_t sp0 = static_cast<uint8_t>(SpriteZeroHitFlag);
    uint8_t spOv = static_cast<uint8_t>(SpriteOverflowFlag);

    if (Line == 241 && Dot == 1)
    {
        SuppressNmi = true; // Suppress interrupt
    }

    if (Line == 241 && Dot >= 2 && Dot <= 3)
    {
        InterruptActive = false;
    }

    NmiOccuredFlag = false;
    AddressLatch = false;

    return  (vB << 7) | (sp0 << 6) | (spOv << 5) | LowerBits;
}

uint8_t PPUState::ReadOAMData()
{
    return OamData;
}

uint8_t PPUState::ReadPPUData(Cart& cart)
{
    uint8_t value = DataBuffer;
    DataBuffer = Read(cart);
    PpuAddressIncrement ? PpuAddress = (PpuAddress + 32) & 0x7FFF : PpuAddress = (PpuAddress + 1) & 0x7FFF;
    SetBusAddress(cart, PpuAddress);

    return value;
}

void PPUState::WritePPUCTRL(uint8_t M)
{
    if (Clock > ResetDelay)
    {
        PpuTempAddress = (PpuTempAddress & 0x73FF) | ((0x3 & static_cast<uint16_t>(M)) << 10); // High bits of NameTable address
        PpuAddressIncrement = ((0x4 & M) >> 2) != 0;
        BaseSpriteTableAddress = 0x1000 * ((0x8 & M) >> 3);
        BaseBackgroundTableAddress = 0x1000 * ((0x10 & M) >> 4);
        SpriteSizeSwitch = ((0x20 & M) >> 5) != 0;
        NmiEnabled = ((0x80 & M) >> 7) != 0;

        if (NmiEnabled == false && Line == 241 && (Dot - 1) == 1)
        {
            InterruptActive = false;
        }
    }

    LowerBits = (0x1F & M);
}

void PPUState::WritePPUMASK(uint8_t M)
{
    if (Clock > ResetDelay)
    {
        GrayScaleEnabled = (0x1 & M);
        ShowBackgroundLeft = ((0x2 & M) >> 1) != 0;
        ShowSpritesLeft = ((0x4 & M) >> 2) != 0;
        ShowBackground = ((0x8 & M) >> 3) != 0;
        ShowSprites = ((0x10 & M) >> 4) != 0;
        IntenseRed = ((0x20 & M) >> 5) != 0;
        IntenseGreen = ((0x40 & M) >> 6) != 0;
        IntenseBlue = ((0x80 & M) >> 7) != 0;
    }

    LowerBits = (0x1F & M);
}

void PPUState::WriteOAMADDR(uint8_t M)
{
    OamAddress = M;
    OamData = Oam[OamAddress];
    LowerBits = (0x1F & OamAddress);
}

void PPUState::WriteOAMDATA(uint8_t M)
{
    if (RenderingEnabled && (Line < 240 || Line == 261))
    {
        return;
    }

    OamData = M;
    Oam[OamAddress] = M;
    OamAddress = (OamAddress + 1) % 0x100;

    LowerBits = (0x1F & M);
}

void PPUState::WritePPUSCROLL(uint8_t M)
{
    if (Clock > ResetDelay)
    {
        if (AddressLatch)
        {
            PpuTempAddress = (PpuTempAddress & 0x0FFF) | ((0x7 & M) << 12);
            PpuTempAddress = (PpuTempAddress & 0x7C1F) | ((0xF8 & M) << 2);
        }
        else
        {
            FineXScroll = static_cast<uint8_t>(0x7 & M);
            PpuTempAddress = (PpuTempAddress & 0x7FE0) | ((0xF8 & M) >> 3);
        }

        AddressLatch = !AddressLatch;
    }

    LowerBits = static_cast<uint8_t>(0x1F & M);
}

void PPUState::WritePPUADDR(Cart& cart, uint8_t M)
{
    if (Clock > ResetDelay)
    {      
        if (AddressLatch)
        {
            PpuTempAddress = (PpuTempAddress & 0x7F00) | M;
            PpuAddress = PpuTempAddress;
            SetBusAddress(cart, PpuAddress);
        }
        else
        {
            PpuTempAddress = (PpuTempAddress & 0x00FF) | ((0x3F & M) << 8);
        }
        
        AddressLatch = !AddressLatch;
    }

    LowerBits = static_cast<uint8_t>(0x1F & M);
}

void PPUState::WritePPUDATA(Cart& cart, uint8_t M)
{
    Write(cart, M);
    PpuAddressIncrement ? PpuAddress = (PpuAddress + 32) & 0x7FFF : PpuAddress = (PpuAddress + 1) & 0x7FFF;
    SetBusAddress(cart, PpuAddress);

    LowerBits = (0x1F & M);
}

PPU::PPU(NES& nes)
    : _nes(nes)
    , _ppuState(new PPUState())
    , _frameBuffer(new uint32_t[256 * 240])
{
}

PPU::~PPU() = default;

int32_t PPU::GetCurrentDot()
{
    // Adjust the value of Dot to match Nintedulator's logs
    if (_ppuState->Dot == 0)
    {
        return 340;
    }
    else
    {
        return _ppuState->Dot - 1;
    }
}

int32_t PPU::GetCurrentScanline()
{
    // Adjust the value of Line to match Nintedulator's logs
    if (_ppuState->Dot == 0)
    {
        if (_ppuState->Line == 0)
        {
            return -1;
        }
        else
        {
            return _ppuState->Line - 1;
        }
    }
    else
    {
        if (_ppuState->Line == 261)
        {
            return -1;
        }
        else 
        {
            return _ppuState->Line - 1;
        }
    }
}

uint64_t PPU::GetClock()
{
    return _ppuState->Clock;
}

void PPU::Step()
{
    _ppuState->Step(_frameBuffer.get(), _nes.GetCart());

    if (_ppuState->Line == 240 && _ppuState->Dot == 0)
    {
        _frameEnd = true;

        _nes.GetVideoBackend().SubmitFrame(reinterpret_cast<uint8_t*>(_frameBuffer.get()));
    }
}


bool PPU::GetNMIActive()
{
    return _ppuState->InterruptActive;
}

uint8_t PPU::ReadPPUStatus()
{
    return _ppuState->ReadPPUStatus();
}

uint8_t PPU::ReadOAMData()
{
    return _ppuState->ReadOAMData();
}

uint8_t PPU::ReadPPUData()
{
    return _ppuState->ReadPPUData(_nes.GetCart());
}

void PPU::WritePPUCTRL(uint8_t M)
{
    _ppuState->WritePPUCTRL(M);
}

void PPU::WritePPUMASK(uint8_t M)
{
    _ppuState->WritePPUMASK(M);
}

void PPU::WriteOAMADDR(uint8_t M)
{
    _ppuState->WriteOAMADDR(M);
}

void PPU::WriteOAMDATA(uint8_t M)
{
    _ppuState->WriteOAMDATA(M);
}

void PPU::WritePPUSCROLL(uint8_t M)
{
    _ppuState->WritePPUSCROLL(M);
}

void PPU::WritePPUADDR(uint8_t M)
{
    _ppuState->WritePPUADDR(_nes.GetCart(), M);
}

void PPU::WritePPUDATA(uint8_t M)
{
    _ppuState->WritePPUDATA(_nes.GetCart(), M);
}

int PPU::GetFrameRate()
{
    return 0;
}

void PPU::GetNameTable(uint32_t tableIndex, uint8_t* imageBuffer)
{
    uint16_t tableAddress;

    // Select nametable address
    switch (tableIndex)
    {
    case 0:
        tableAddress = 0x000;
        break;
    case 1:
        tableAddress = 0x400;
        break;
    case 2:
        tableAddress = 0x800;
        break;
    case 3:
        tableAddress = 0xC00;
        break;
    default:
        SPDLOG_LOGGER_ERROR(_nes.GetLogger(), "The specified name table index {} is out of range. Valid values are 0-3", tableIndex);
        throw NesException(ERROR_INVALID_NAME_TABLE_INDEX);
    };

    SPDLOG_LOGGER_INFO(_nes.GetLogger(), "Rendering name table {}", tableIndex);

    // Render specified nametable to the supplied buffer
    for (uint32_t i = 0; i < 30; ++i)
    {
        for (uint32_t f = 0; f < 32; ++f)
        {
            uint16_t address = (tableAddress | (i << 5) | f);
            uint8_t ntByte = _ppuState->Peek(_nes.GetCart(), 0x2000 | address);
            uint8_t atShift = (((address & 0x0002) >> 1) | ((address & 0x0040) >> 5)) * 2;
            uint8_t atByte = _ppuState->Peek(_nes.GetCart(), 0x23C0 | (address & 0x0C00) | ((address >> 4) & 0x38) | ((address >> 2) & 0x07));
            atByte = (atByte >> atShift) & 0x3;
            uint16_t patternAddress = static_cast<uint16_t>(ntByte) * 16;

            for (uint32_t h = 0; h < 8; ++h)
            {
                uint8_t tileLow = _ppuState->Peek(_nes.GetCart(), _ppuState->BaseBackgroundTableAddress + patternAddress + h);
                uint8_t tileHigh = _ppuState->Peek(_nes.GetCart(), _ppuState->BaseBackgroundTableAddress + patternAddress + h + 8);

                for (uint32_t g = 0; g < 8; ++g)
                {
                    uint16_t pixel = 0x0003 & ((((tileLow << g) & 0x80) >> 7) | (((tileHigh << g) & 0x80) >> 6));
                    uint16_t paletteIndex = 0x3F00 | (atByte << 2) | pixel;
                    if ((paletteIndex & 0x3) == 0) paletteIndex = 0x3F00;
                    uint32_t rgb = RgbLookupTable[_ppuState->Peek(_nes.GetCart(), paletteIndex)];
                    uint8_t red = (rgb & 0xFF0000) >> 16;
                    uint8_t green = (rgb & 0x00FF00) >> 8;
                    uint8_t blue = (rgb & 0x0000FF);

                    uint32_t index = ((24 * f) + (g * 3)) + ((6144 * i) + (h * 768));

                    imageBuffer[index] = red;
                    imageBuffer[index + 1] = green;
                    imageBuffer[index + 2] = blue;
                }
            }
        }
    }
}

void PPU::GetPatternTable(uint32_t tableIndex, uint32_t paletteIndex, uint8_t* imageBuffer)
{
    uint16_t tableAddress;

    // Select pattern table address
    switch (tableIndex)
    {
    case 0:
        tableAddress = 0x0000;
        break;
    case 1:
        tableAddress = 0x1000;
        break;
    default:
        SPDLOG_LOGGER_ERROR(_nes.GetLogger(), "The specified pattern table index {} is out of range. Valid values are 0-1", tableIndex);
        throw NesException(ERROR_INVALID_PATTERN_TABLE_INDEX);
    };

    if (paletteIndex > 7)
    {
        SPDLOG_LOGGER_ERROR(_nes.GetLogger(), "The specified palette index {} is out of range. Valid values are 0-7", paletteIndex);
        throw NesException(ERROR_INVALID_PALETTE_INDEX);
    }

    SPDLOG_LOGGER_INFO(_nes.GetLogger(), "Rendering pattern table {} using palette {}", tableIndex, paletteIndex);

    for (uint32_t i = 0; i < 16; ++i)
    {
        for (uint32_t f = 0; f < 16; ++f)
        {
            uint32_t patternIndex = (16 * f) + (256 * i);

            for (uint32_t g = 0; g < 8; ++g)
            {
                uint8_t tileLow = _ppuState->Peek(_nes.GetCart(), tableAddress + patternIndex + g);
                uint8_t tileHigh = _ppuState->Peek(_nes.GetCart(), tableAddress + patternIndex + g + 8);

                for (uint32_t h = 0; h < 8; ++h)
                {
                    uint16_t pixel = 0x0003 & ((((tileLow << h) & 0x80) >> 7) | (((tileHigh << h) & 0x80) >> 6));
                    uint16_t colorIndex = (0x3F00 + (4 * (paletteIndex % 8))) | pixel;
                    uint32_t rgb = RgbLookupTable[_ppuState->Peek(_nes.GetCart(), colorIndex)];
                    uint8_t red = (rgb & 0xFF0000) >> 16;
                    uint8_t green = (rgb & 0x00FF00) >> 8;
                    uint8_t blue = (rgb & 0x0000FF);

                    uint32_t index = ((24 * f) + (h * 3)) + ((3072 * i) + (g * 384));

                    imageBuffer[index] = red;
                    imageBuffer[index + 1] = green;
                    imageBuffer[index + 2] = blue;
                }
            }
        }
    }
}

void PPU::GetPalette(uint32_t paletteIndex, uint8_t* imageBuffer)
{
    uint16_t baseAddress;
    switch (paletteIndex)
    {
    case 0:
        baseAddress = 0x3F00;
        break;
    case 1:
        baseAddress = 0x3F04;
        break;
    case 2:
        baseAddress = 0x3F08;
        break;
    case 3:
        baseAddress = 0x3F0C;
        break;
    case 4:
        baseAddress = 0x3F10;
        break;
    case 5:
        baseAddress = 0x3F14;
        break;
    case 6:
        baseAddress = 0x3F18;
        break;
    case 7:
        baseAddress = 0x3F1C;
        break;
    default:
        SPDLOG_LOGGER_ERROR(_nes.GetLogger(), "The specified palette index {} is out of range. Valid values are 0-7", paletteIndex);
        throw NesException(ERROR_INVALID_PALETTE_INDEX);
    }

    SPDLOG_LOGGER_INFO(_nes.GetLogger(), "Rendering palette {}", paletteIndex);

    for (uint32_t i = 0; i < 4; ++i)
    {
        uint16_t paletteAddress = baseAddress + i;

        for (uint32_t g = 0; g < 16; ++g)
        {
            for (uint32_t h = 0; h < 16; ++h)
            {
                uint32_t rgb = RgbLookupTable[_ppuState->Peek(_nes.GetCart(), paletteAddress)];
                uint8_t red = (rgb & 0xFF0000) >> 16;
                uint8_t green = (rgb & 0x00FF00) >> 8;
                uint8_t blue = (rgb & 0x0000FF);

                uint32_t index = ((48 * i) + (h * 3)) + (192 * g);

                imageBuffer[index] = red;
                imageBuffer[index + 1] = green;
                imageBuffer[index + 2] = blue;
            }
        }
    }
}

void PPU::GetSprite(uint32_t spriteIndex, uint8_t* imageBuffer)
{
    if (spriteIndex > 63)
    {
        SPDLOG_LOGGER_ERROR(_nes.GetLogger(), "The specified sprite index {} is out of range. Valid values are 0-63", spriteIndex);
        throw NesException(ERROR_INVALID_PALETTE_INDEX);
    }

    SPDLOG_LOGGER_INFO(_nes.GetLogger(), "Rendering sprite {}", spriteIndex);

    uint8_t byteOne = _ppuState->Oam[(0x4 * spriteIndex) + 1];
    uint8_t byteTwo = _ppuState->Oam[(0x4 * spriteIndex) + 2];

    uint16_t tableIndex = _ppuState->BaseSpriteTableAddress;
    uint16_t patternIndex = byteOne * 16;
    uint8_t palette = (byteTwo & 0x03) + 4;

    for (uint32_t i = 0; i < 8; ++i)
    {
        uint8_t tileLow = _ppuState->Peek(_nes.GetCart(), tableIndex + patternIndex + i);
        uint8_t tileHigh = _ppuState->Peek(_nes.GetCart(), tableIndex + patternIndex + i + 8);

        for (uint32_t f = 0; f < 8; ++f)
        {
            uint16_t pixel = 0x0003 & ((((tileLow << f) & 0x80) >> 7) | (((tileHigh << f) & 0x80) >> 6));
            uint16_t paletteIndex = (0x3F00 + (4 * (palette % 8))) | pixel;
            uint32_t rgb = RgbLookupTable[_ppuState->Peek(_nes.GetCart(), paletteIndex)];
            uint8_t red = (rgb & 0xFF0000) >> 16;
            uint8_t green = (rgb & 0x00FF00) >> 8;
            uint8_t blue = (rgb & 0x0000FF);

            uint32_t index = (24 * i) + (f * 3);

            imageBuffer[index] = red;
            imageBuffer[index + 1] = green;
            imageBuffer[index + 2] = blue;
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

bool PPU::EndOfFrame()
{
    bool frameEnd = _frameEnd;
    _frameEnd = false;

    return frameEnd;
}