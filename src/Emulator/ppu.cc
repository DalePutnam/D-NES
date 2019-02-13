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

PPU::PPU(VideoBackend* vout, NESCallback* callback)
    : Cpu(nullptr)
    , Cartridge(nullptr)
    , VideoOut(vout)
    , Callback(callback)
    , FpsCounter(0)
    , CurrentFps(0)
    , FrameCountStart(std::chrono::steady_clock::now())
    , RequestTurboMode(false)
    , TurboModeEnabled(false)
    , TurboFrameSkip(0)
    , Clock(0)
    , Dot(1)
    , Line(241)
    , Even(true)
    , SuppressNmi(false)
    , InterruptActive(false)
    , NmiOccuredCycle(0)
    , PpuAddressIncrement(false)
    , BaseSpriteTableAddress(0)
    , BaseBackgroundTableAddress(0)
    , SpriteSizeSwitch(false)
    , NmiEnabled(false)
    , GrayScaleEnabled(false)
    , ShowBackgroundLeft(0)
    , ShowSpritesLeft(0)
    , ShowBackground(0)
    , ShowSprites(0)
    , IntenseRed(0)
    , IntenseGreen(0)
    , IntenseBlue(0)
    , LowerBits(0x06)
    , SpriteOverflowFlag(false)
    , SpriteZeroHitFlag(false)
    , NmiOccuredFlag(false)
    , SpriteZeroSecondaryOamFlag(false)
    , OamAddress(0)
    , PpuAddress(0)
    , PpuTempAddress(0)
    , FineXScroll(0)
    , AddressLatch(false)
    , DataBuffer(0)
    , NameTableByte(0)
    , AttributeByte(0)
    , TileBitmapLow(0)
    , TileBitmapHigh(0)
    , BackgroundShift0(0)
    , BackgroundShift1(0)
    , BackgroundAttributeShift0(0)
    , BackgroundAttributeShift1(0)
    , BackgroundAttribute(0)
    , SpriteCount(0)
	, FrameBufferIndex(0)
    , NtscMode(false)
{
    memset(NameTable0, 0, sizeof(uint8_t) * 0x400);
    memset(NameTable1, 0, sizeof(uint8_t) * 0x400);
    memset(PrimaryOam, 0, sizeof(uint8_t) * 0x100);
    memset(SecondaryOam, 0, sizeof(uint8_t) * 0x20);
    memset(PaletteTable, 0, sizeof(uint8_t) * 0x20);
    memset(SpriteShift0, 0, sizeof(uint8_t) * 8);
    memset(SpriteShift1, 0, sizeof(uint8_t) * 8);
    memset(SpriteAttribute, 0, sizeof(uint8_t) * 8);
    memset(SpriteCounter, 0, sizeof(uint8_t) * 8);
}

PPU::~PPU() {}

void PPU::AttachCPU(CPU* cpu)
{
    Cpu = cpu;
}

void PPU::AttachAPU(APU* apu)
{
    Apu = apu;
}

void PPU::AttachCart(Cart* cart)
{
    Cartridge = cart;
}

uint16_t PPU::GetCurrentDot()
{
    if (Dot == 0)
    {
        return 340;
    }
    else
    {
        return Dot - 1;
    }
}

uint16_t PPU::GetCurrentScanline()
{
    if (Dot == 0)
    {
        if (Line == 0)
        {
            return 261;
        }
        else
        {
            return Line - 1;
        }
    }
    else
    {
        return Line;
    }
}

void PPU::Step()
{
    // Pre-render line
    if (Line == 261)
    {
        if (Dot == 1)
        {
            NmiOccuredFlag = false;
            InterruptActive = false;
            SpriteZeroHitFlag = false;
        }

        if (ShowSprites || ShowBackground)
        {
            if ((Dot >= 2 && Dot <= 257) || (Dot >= 322 && Dot <= 337))
            {
                BackgroundShift0 <<= 1;
                BackgroundShift1 <<= 1;
                BackgroundAttributeShift0 <<= 1;
                BackgroundAttributeShift1 <<= 1;

                uint8_t cycle = (Dot - 1) % 8;
                switch (cycle)
                {
                case 0:
                    LoadBackgroundShiftRegisters();
                    break;
                case 1:
                    NameTableFetch();
                    break;
                case 3:
                    BackgroundAttributeFetch();
                    break;
                case 5:
                    BackgroundLowByteFetch();
                    break;
                case 7:
                    BackgroundHighByteFetch();
                    IncrementXScroll();
                    break;
                default:
                    break;
                }

                if (Dot == 256)
                {
                    IncrementYScroll();
                }

                if (Dot == 257)
                {
                    PpuAddress = (PpuAddress & 0x7BE0) | (PpuTempAddress & 0x041F);
                }
            }
            else if (Dot >= 258 && Dot <= 320)
            {
                uint8_t cycle = (Dot - 1) % 8;
                switch (cycle)
                {
                case 2:
                    SpriteAttributeFetch();
                    break;
                case 3:
                    SpriteXCoordinateFetch();
                    break;
                case 5:
                    SpriteLowByteFetch();
                    break;
                case 7:
                    SpriteHighByteFetch();
                    break;
                }
            }
            else if (Dot == 338 || Dot == 340)
            {
                NameTableFetch();
            }

            // From dot 280 to 304 of the pre-render line copy all vertical position bits to ppuAddress from ppuTempAddress
            if (Dot >= 280 && Dot <= 304)
            {
                PpuAddress = (PpuAddress & 0x041F) | (PpuTempAddress & 0x7BE0);
            }

            if (Dot == 339 && !Even)
            {
                Dot = Line = 0;
            }
            else if (Dot == 340)
            {
                Dot = Line = 0;
            }
            else
            {
                ++Dot;
            }
        }
        else
        {
            if (Dot == 340)
            {
                Dot = Line = 0;
            }
            else
            {
                ++Dot;
            }
        }
    }
    // Visible Lines
    else if (Line >= 0 && Line <= 239)
    {
        if (ShowSprites || ShowBackground)
        {
            if ((Dot >= 2 && Dot <= 257) || (Dot >= 322 && Dot <= 337))
            {
                BackgroundShift0 <<= 1;
                BackgroundShift1 <<= 1;
                BackgroundAttributeShift0 <<= 1;
                BackgroundAttributeShift1 <<= 1;

                uint8_t cycle = (Dot - 1) % 8;
                switch (cycle)
                {
                case 0:
                    LoadBackgroundShiftRegisters();
                    break;
                case 1:
                    NameTableFetch();
                    break;
                case 3:
                    BackgroundAttributeFetch();
                    break;
                case 5:
                    BackgroundLowByteFetch();
                    break;
                case 7:
                    BackgroundHighByteFetch();
                    IncrementXScroll();
                    break;
                default:
                    break;
                }

                if (Dot == 256)
                {
                    IncrementYScroll();
                }

                if (Dot == 257)
                {
                    SpriteEvaluation();
                    PpuAddress = (PpuAddress & 0x7BE0) | (PpuTempAddress & 0x041F);
                }
            }
            else if (Dot >= 258 && Dot <= 320)
            {
                uint8_t cycle = (Dot - 1) % 8;
                switch (cycle)
                {
                case 2:
                    SpriteAttributeFetch();
                    break;
                case 3:
                    SpriteXCoordinateFetch();
                    break;
                case 5:
                    SpriteLowByteFetch();
                    break;
                case 7:
                    SpriteHighByteFetch();
                    break;
                }
            }
            else if (Dot == 338 || Dot == 340)
            {
                NameTableFetch();
            }

            if (Dot >= 1 && Dot <= 256)
            {
                //if (TurboFrameSkip == 0)
                {
                    RenderPixel();
                }
            }
        }
        else
        {
            if (Dot >= 1 && Dot <= 256)
            {
                //if (TurboFrameSkip == 0)
                {
                    RenderPixelIdle();
                }
            }
        }

        // End of Visible Frame
        if (Dot == 256 && Line == 239)
        {
            // Toggle even flag
            Even = !Even;

            UpdateFrameSkipCounters();

            // Update frame rate counter
            UpdateFrameRate();

            // Check if a change in rendering mode or turbo mode has been requested
            MaybeChangeModes();

            if (TurboFrameSkip == 0) {
                VideoOut->SubmitFrame(reinterpret_cast<uint8_t*>(FrameBuffer));
                FrameBufferIndex = 0;

                if (Callback != nullptr) {
                    Callback->OnFrameComplete();
                }
            }
        }

        if (Dot == 340)
        {
            Dot = 0;
            ++Line;
        }
        else
        {
            ++Dot;
        }
    }
    // Post-render line
    else if (Line == 240 || (Line == 241 && Dot == 0))
    {
        if (Dot == 340)
        {
            Dot = 0;
            ++Line;
        }
        else
        {
            ++Dot;
        }
    }
    // VBlank Lines
    else if (Line >= 241 && Line <= 260)
    {
        if (Dot == 1 && Line == 241 && Clock > ResetDelay)
        {
            if (Dot == 1 && Line == 241 && !SuppressNmi)
            {
                NmiOccuredFlag = true;

                if (NmiOccuredFlag && NmiEnabled)
                {
                    NmiOccuredCycle = Clock;
                    InterruptActive = true;
                }
            }

            SuppressNmi = false;
        }
        else
        {
            if (!NmiOccuredFlag || !NmiEnabled)
            {
                InterruptActive = false;
            }

            if (NmiOccuredFlag && NmiEnabled)
            {
                if (!InterruptActive)
                {
                    NmiOccuredCycle = Clock;
                    InterruptActive = true;
                }
            }
        }

        if (Dot == 340)
        {
            Dot = 0;
            ++Line;
        }
        else
        {
            ++Dot;
        }
    }

    ++Clock;
}

void PPU::Run()
{
    while (Cpu->GetClock() - Clock > 0)
    {
        Step();
    }
}

bool PPU::CheckNMI(uint64_t& occurredCycle)
{
    occurredCycle = NmiOccuredCycle;
    return InterruptActive;
}

void PPU::SetTurboModeEnabled(bool enabled)
{
    RequestTurboMode = enabled;
}

void PPU::SetNtscDecodingEnabled(bool enabled)
{
    RequestNtscMode = enabled;
}

uint8_t PPU::ReadPPUStatus()
{
    uint8_t vB = static_cast<uint8_t>(NmiOccuredFlag);
    uint8_t sp0 = static_cast<uint8_t>(SpriteZeroHitFlag);
    uint8_t spOv = static_cast<uint8_t>(SpriteOverflowFlag);

    if (Line == 241 && (Dot - 1) == 0)
    {
        SuppressNmi = true; // Suppress interrupt
    }

    if (Line == 241 && (Dot - 1) >= 1 && (Dot - 1) <= 2)
    {
        InterruptActive = false;
    }

    NmiOccuredFlag = false;
    AddressLatch = false;

    return  (vB << 7) | (sp0 << 6) | (spOv << 5) | LowerBits;
}

uint8_t PPU::ReadOAMData()
{
    return PrimaryOam[OamAddress];
}

uint8_t PPU::ReadPPUData()
{
    uint16_t addr = PpuAddress % 0x4000;
    PpuAddressIncrement ? PpuAddress = (PpuAddress + 32) & 0x7FFF : PpuAddress = (PpuAddress + 1) & 0x7FFF;
    uint8_t value = DataBuffer;

    if (addr < 0x2000)
    {
        DataBuffer = Cartridge->ChrRead(addr);
    }
    else if (addr >= 0x2000 && addr < 0x4000)
    {
        DataBuffer = ReadNameTable(addr);
    }

    if (addr >= 0x3F00)
    {
        if (addr % 4 == 0) addr &= 0xFF0F; // Ignore second nibble if addr is a multiple of 4
        value = PaletteTable[addr % 0x20];
    }

    return value;
}

void PPU::WritePPUCTRL(uint8_t M)
{
    if (Cpu->GetClock() > ResetDelay)
    {
        PpuTempAddress = (PpuTempAddress & 0x73FF) | ((0x3 & static_cast<uint16_t>(M)) << 10); // High bits of NameTable address
        PpuAddressIncrement = ((0x4 & M) >> 2) != 0;
        BaseSpriteTableAddress = 0x1000 * ((0x8 & M) >> 3);
        BaseBackgroundTableAddress = 0x1000 * ((0x10 & M) >> 4);
        SpriteSizeSwitch = ((0x20 & M) >> 5) != 0;
        NmiEnabled = ((0x80 & M) >> 7) != 0;
        LowerBits = (0x1F & M);

        if (NmiEnabled == false && Line == 241 && (Dot - 1) == 1)
        {
            InterruptActive = false;
        }
    }
}

void PPU::WritePPUMASK(uint8_t M)
{
    if (Cpu->GetClock() > ResetDelay)
    {
        GrayScaleEnabled = (0x1 & M);
        ShowBackgroundLeft = ((0x2 & M) >> 1) != 0;
        ShowSpritesLeft = ((0x4 & M) >> 2) != 0;
        ShowBackground = ((0x8 & M) >> 3) != 0;
        ShowSprites = ((0x10 & M) >> 4) != 0;
        IntenseRed = ((0x20 & M) >> 5) != 0;
        IntenseGreen = ((0x40 & M) >> 6) != 0;
        IntenseBlue = ((0x80 & M) >> 7) != 0;
        LowerBits = (0x1F & M);
    }
}

void PPU::WriteOAMADDR(uint8_t M)
{
    OamAddress = M;
    LowerBits = (0x1F & OamAddress);
}

void PPU::WriteOAMDATA(uint8_t M)
{
    PrimaryOam[OamAddress++] = M;
    LowerBits = (0x1F & M);
}

void PPU::WritePPUSCROLL(uint8_t M)
{
    if (Cpu->GetClock() > ResetDelay)
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
        LowerBits = static_cast<uint8_t>(0x1F & M);
    }
}

void PPU::WritePPUADDR(uint8_t M)
{
    if (Cpu->GetClock() > ResetDelay)
    {
        AddressLatch ? PpuTempAddress = (PpuTempAddress & 0x7F00) | M : PpuTempAddress = (PpuTempAddress & 0x00FF) | ((0x3F & M) << 8);
        if (AddressLatch) PpuAddress = PpuTempAddress;
        AddressLatch = !AddressLatch;
        LowerBits = static_cast<uint8_t>(0x1F & M);
    }
}

void PPU::WritePPUDATA(uint8_t M)
{
    Write(PpuAddress, M);
    PpuAddressIncrement ? PpuAddress = (PpuAddress + 32) & 0x7FFF : PpuAddress = (PpuAddress + 1) & 0x7FFF;
    LowerBits = (0x1F & M);
}

int PPU::GetFrameRate()
{
    return CurrentFps;
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
            uint8_t ntByte = ReadNameTable(0x2000 | address);
            uint8_t atShift = (((address & 0x0002) >> 1) | ((address & 0x0040) >> 5)) * 2;
            uint8_t atByte = ReadNameTable(0x23C0 | (address & 0x0C00) | ((address >> 4) & 0x38) | ((address >> 2) & 0x07));
            atByte = (atByte >> atShift) & 0x3;
            uint16_t patternAddress = static_cast<uint16_t>(ntByte) * 16;

            for (uint32_t h = 0; h < 8; ++h)
            {
                uint8_t tileLow = Cartridge->ChrRead(BaseBackgroundTableAddress + patternAddress + h);
                uint8_t tileHigh = Cartridge->ChrRead(BaseBackgroundTableAddress + patternAddress + h + 8);

                for (uint32_t g = 0; g < 8; ++g)
                {
                    uint16_t pixel = 0x0003 & ((((tileLow << g) & 0x80) >> 7) | (((tileHigh << g) & 0x80) >> 6));
                    uint16_t paletteIndex = 0x3F00 | (atByte << 2) | pixel;
                    if ((paletteIndex & 0x3) == 0) paletteIndex = 0x3F00;
                    uint32_t rgb = RgbLookupTable[Read(paletteIndex)];
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
                uint8_t tileLow = Cartridge->ChrRead(tableIndex + patternIndex + g);
                uint8_t tileHigh = Cartridge->ChrRead(tableIndex + patternIndex + g + 8);

                for (uint32_t h = 0; h < 8; ++h)
                {
                    uint16_t pixel = 0x0003 & ((((tileLow << h) & 0x80) >> 7) | (((tileHigh << h) & 0x80) >> 6));
                    uint16_t paletteIndex = (0x3F00 + (4 * (palette % 8))) | pixel;
                    uint32_t rgb = RgbLookupTable[Read(paletteIndex)];
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
                uint32_t rgb = RgbLookupTable[Read(paletteAddress)];
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
    uint8_t byteOne = PrimaryOam[(0x4 * sprite) + 1];
    uint8_t byteTwo = PrimaryOam[(0x4 * sprite) + 2];

    uint16_t tableIndex = BaseSpriteTableAddress;
    uint16_t patternIndex = byteOne * 16;
    uint8_t palette = (byteTwo & 0x03) + 4;

    for (uint32_t i = 0; i < 8; ++i)
    {
        uint8_t tileLow = Cartridge->ChrRead(tableIndex + patternIndex + i);
        uint8_t tileHigh = Cartridge->ChrRead(tableIndex + patternIndex + i + 8);

        for (uint32_t f = 0; f < 8; ++f)
        {
            uint16_t pixel = 0x0003 & ((((tileLow << f) & 0x80) >> 7) | (((tileHigh << f) & 0x80) >> 6));
            uint16_t paletteIndex = (0x3F00 + (4 * (palette % 8))) | pixel;
            uint32_t rgb = RgbLookupTable[Read(paletteIndex)];
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

int PPU::STATE_SIZE = (sizeof(uint64_t)*2)+(sizeof(uint16_t)*8)+(sizeof(uint8_t)*0x96C)+(sizeof(char)*3);

void PPU::SaveState(char* state)
{
    memcpy(state, &Clock, sizeof(uint64_t));
    state += sizeof(uint64_t);

    memcpy(state, &Dot, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(state, &Line, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(state, &NmiOccuredCycle, sizeof(uint64_t));
    state += sizeof(uint64_t);

    memcpy(state, &BaseSpriteTableAddress, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(state, &BaseBackgroundTableAddress, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(state, &LowerBits, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &OamAddress, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &PpuAddress, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(state, &PpuTempAddress, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(state, &FineXScroll, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &DataBuffer, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &PrimaryOam, sizeof(uint8_t) * 0x100);
    state += sizeof(uint8_t) * 0x100;

    memcpy(state, &SecondaryOam, sizeof(uint8_t) * 0x20);
    state += sizeof(uint8_t) * 0x20;

    memcpy(state, &NameTable0, sizeof(uint8_t) * 0x400);
    state += sizeof(uint8_t) * 0x400;

    memcpy(state, &NameTable1, sizeof(uint8_t) * 0x400);
    state += sizeof(uint8_t) * 0x400;

    memcpy(state, &PaletteTable, sizeof(uint8_t) * 0x20);
    state += sizeof(uint8_t) * 0x20;

    memcpy(state, &NameTableByte, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &AttributeByte, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &TileBitmapLow, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &TileBitmapHigh, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &BackgroundShift0, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(state, &BackgroundShift1, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(state, &BackgroundAttributeShift0, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &BackgroundAttributeShift1, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &BackgroundAttribute, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &SpriteCount, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(state, &SpriteShift0, sizeof(uint8_t) * 8);
    state += sizeof(uint8_t) * 8;

    memcpy(state, &SpriteShift1, sizeof(uint8_t) * 8);
    state += sizeof(uint8_t) * 8;

    memcpy(state, &SpriteAttribute, sizeof(uint8_t) * 8);
    state += sizeof(uint8_t) * 8;

    memcpy(state, &SpriteCounter, sizeof(uint8_t) * 8);
    state += sizeof(uint8_t) * 8;

    char packedBool = 0;
    packedBool |= Even << 7;
    packedBool |= SuppressNmi << 6;
    packedBool |= InterruptActive << 5;
    packedBool |= PpuAddressIncrement << 4;
    packedBool |= SpriteSizeSwitch << 3;
    packedBool |= NmiEnabled << 2;
    packedBool |= GrayScaleEnabled << 1;
    packedBool |= ShowBackgroundLeft << 0;

    memcpy(state, &packedBool, sizeof(char));
    state += sizeof(char);

    packedBool = 0;
    packedBool |= ShowSpritesLeft << 7;
    packedBool |= ShowBackground << 6;
    packedBool |= ShowSprites << 5;
    packedBool |= IntenseRed << 4;
    packedBool |= IntenseGreen << 3;
    packedBool |= IntenseBlue << 2;
    packedBool |= SpriteOverflowFlag << 1;
    packedBool |= SpriteZeroHitFlag << 0;

    memcpy(state, &packedBool, sizeof(char));
    state += sizeof(char);

    packedBool = 0;
    packedBool |= NmiOccuredFlag << 6;
    packedBool |= SpriteZeroSecondaryOamFlag << 5;
    packedBool |= AddressLatch << 4;

    memcpy(state, &packedBool, sizeof(char));
}

void PPU::LoadState(const char* state)
{
    memcpy(&Clock, state, sizeof(uint64_t));
    state += sizeof(uint64_t);

    memcpy(&Dot, state, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(&Line, state, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(&NmiOccuredCycle, state, sizeof(uint64_t));
    state += sizeof(uint64_t);

    memcpy(&BaseSpriteTableAddress, state, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(&BaseBackgroundTableAddress, state, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(&LowerBits, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&OamAddress, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&PpuAddress, state, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(&PpuTempAddress, state, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(&FineXScroll, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&DataBuffer, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&PrimaryOam, state, sizeof(uint8_t) * 0x100);
    state += sizeof(uint8_t) * 0x100;

    memcpy(&SecondaryOam, state, sizeof(uint8_t) * 0x20);
    state += sizeof(uint8_t) * 0x20;

    memcpy(&NameTable0, state, sizeof(uint8_t) * 0x400);
    state += sizeof(uint8_t) * 0x400;

    memcpy(&NameTable1, state, sizeof(uint8_t) * 0x400);
    state += sizeof(uint8_t) * 0x400;

    memcpy(&PaletteTable, state, sizeof(uint8_t) * 0x20);
    state += sizeof(uint8_t) * 0x20;

    memcpy(&NameTableByte, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&AttributeByte, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&TileBitmapLow, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&TileBitmapHigh, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&BackgroundShift0, state, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(&BackgroundShift1, state, sizeof(uint16_t));
    state += sizeof(uint16_t);

    memcpy(&BackgroundAttributeShift0, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&BackgroundAttributeShift1, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&BackgroundAttribute, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&SpriteCount, state, sizeof(uint8_t));
    state += sizeof(uint8_t);

    memcpy(&SpriteShift0, state, sizeof(uint8_t) * 8);
    state += sizeof(uint8_t) * 8;

    memcpy(&SpriteShift1, state, sizeof(uint8_t) * 8);
    state += sizeof(uint8_t) * 8;

    memcpy(&SpriteAttribute, state, sizeof(uint8_t) * 8);
    state += sizeof(uint8_t) * 8;

    memcpy(&SpriteCounter, state, sizeof(uint8_t) * 8);
    state += sizeof(uint8_t) * 8;

    char packedBool;
    memcpy(&packedBool, state, sizeof(char));
    state += sizeof(char);

    Even = !!(packedBool & 0x80);
    SuppressNmi = !!(packedBool & 0x40);
    InterruptActive = !!(packedBool & 0x20);
    PpuAddressIncrement = !!(packedBool & 0x10);
    SpriteSizeSwitch = !!(packedBool & 0x8);
    NmiEnabled = !!(packedBool & 0x4);
    GrayScaleEnabled = !!(packedBool & 0x2);
    ShowBackgroundLeft = !!(packedBool & 0x1);

    memcpy(&packedBool, state, sizeof(char));
    state += sizeof(char);

    ShowSpritesLeft = !!(packedBool & 0x80);
    ShowBackground = !!(packedBool & 0x40);
    ShowSprites = !!(packedBool & 0x20);
    IntenseRed = !!(packedBool & 0x10);
    IntenseGreen = !!(packedBool & 0x8);
    IntenseBlue = !!(packedBool & 0x4);
    SpriteOverflowFlag = !!(packedBool & 0x2);
    SpriteZeroHitFlag = !!(packedBool & 0x1);

    memcpy(&packedBool, state, sizeof(char));

    NmiOccuredFlag = !!(packedBool & 0x40);
    SpriteZeroSecondaryOamFlag = !!(packedBool & 0x20);
    AddressLatch = !!(packedBool & 0x10);
}

void PPU::RenderNtscPixel(int pixel)
{
    static constexpr float levels[16] =
    {
        // Normal Levels
          -0.116f / 12.f, 0.000f / 12.f, 0.307f / 12.f, 0.714f / 12.f,
           0.399f / 12.f, 0.684f / 12.f, 1.000f / 12.f, 1.000f / 12.f,
           // Attenuated Levels
          -0.087f / 12.f, 0.000f / 12.f, 0.229f / 12.f, 0.532f / 12.f,
           0.298f / 12.f, 0.510f / 12.f, 0.746f / 12.f, 0.746f / 12.f
    };

    auto SignalLevel = [=](int phase)
    {
        // Decode the NES color.
        int color = (pixel & 0x0F);    // 0..15 "cccc"
        int level = (pixel >> 4) & 3;  // 0..3  "ll"
        int emphasis = (pixel >> 6);   // 0..7  "eee"
        if (color > 13) { level = 1; } // For colors 14..15, level 1 is forced.

                                       // The square wave for this color alternates between these two voltages:
        int low = level;
        int high = level + 4;

        if (color == 0) { low = high; } // For color 0, only high level is emitted
        if (color > 12) { high = low; } // For colors 13..15, only low level is emitted

         // Generate the square wave
        auto InColorPhase = [=](int color) { return (color + phase) % 12 < 6; }; // Inline function
        int index = InColorPhase(color) ? high : low;

        // When de-emphasis bits are set, some parts of the signal are attenuated:
        if (((emphasis & 1) && InColorPhase(0)) || ((emphasis & 2) && InColorPhase(4)) || ((emphasis & 4) && InColorPhase(8)))
        {
            return levels[index + 8];
        }
        else
        {
            return levels[index];
        }
    };

    int phase = (Clock << 3) % 12;
    for (int p = 0; p < 8; ++p) // Each pixel produces distinct 8 samples of NTSC signal.
    {
        int index = ((Dot - 1) << 3) + p;
        SignalLevels[index] = SignalLevel((phase + p) % 12);
    }
}

void PPU::RenderNtscLine()
{
    static constexpr int width = 256;
    static constexpr float sineTable[12] =
    {
        0.89101f,  0.54464f,  0.05234f, -0.45399f, -0.83867f, -0.99863f,
       -0.89101f, -0.54464f, -0.05234f,  0.45399f,  0.83867f,  0.99863f
    };

    int phase = ((Clock - width + 1) << 3) % 12;

    for (unsigned int x = 0; x < width; ++x)
    {
        // Determine the region of scanline signal to sample. Take 12 samples.
        int center = ((x * width * 8) / width) + 4;
        int begin = center - 6; if (begin < 0) begin = 0;
        int end = center + 6; if (end > (width << 3)) end = (width << 3);
        float y = 0.f, i = 0.f, q = 0.f; // Calculate the color in YIQ.
        for (int p = begin; p < end; ++p) // Collect and accumulate samples
        {
            float level = SignalLevels[p];
            y = y + level;
            i = i + level * sineTable[(phase + p + 3) % 12];
            q = q + level * sineTable[(phase + p) % 12];
        }

        auto clamp = [](float v) { return (v > 255.0f) ? 255.0f : ((v < 0.0f) ? 0.0f : v); };

        int red = static_cast<int>(clamp(255.95f * (y + 0.946882f*i + 0.623557f*q)));
        int green = static_cast<int>(clamp(255.95f * (y + -0.274788f*i + -0.635691f*q)));
        int blue = static_cast<int>(clamp(255.95f * (y + -1.108545f*i + 1.709007f*q)));

        if (VideoOut != nullptr)
        {
            uint32_t pixel = (red << 16) | (green << 8) | blue | 0xFF000000;
			FrameBuffer[FrameBufferIndex++] = pixel;
        }
        
    }
}

// This is sort of a bastardized version of the PPU sprite evaluation
// It is not cycle accurate unfortunately, but I don't suspect that will
// cause any issues (but what do I know really?)
void PPU::SpriteEvaluation()
{
    uint8_t glitchCount = 0;
    SpriteCount = 0;
    SpriteZeroSecondaryOamFlag = false;

    for (int i = 0; i < 8; ++i)
    {
        uint8_t index = i * 4;
        SecondaryOam[index] = 0xFF;
        SecondaryOam[index + 1] = 0xFF;
        SecondaryOam[index + 2] = 0xFF;
        SecondaryOam[index + 3] = 0xFF;
    }

    for (int i = 0; i < 64; ++i)
    {
        uint8_t size = SpriteSizeSwitch ? 16 : 8;
        uint8_t index = i * 4; // Index of one of 64 sprites in Primary OAM
        uint8_t spriteY = PrimaryOam[index + glitchCount]; // The sprites Y coordinate, glitchCount is used to replicate a hardware glitch

        if (SpriteCount < 8) // If fewer than 8 sprites have been found
        {
            // If a sprite is in range, copy it to the next spot in secondary OAM
            if (spriteY <= Line && spriteY + size > Line && Line != 239)
            {
                if (index == 0)
                {
                    SpriteZeroSecondaryOamFlag = true;
                }

                SecondaryOam[SpriteCount * 4] = PrimaryOam[index];
                SecondaryOam[(SpriteCount * 4) + 1] = PrimaryOam[index + 1];
                SecondaryOam[(SpriteCount * 4) + 2] = PrimaryOam[index + 2];
                SecondaryOam[(SpriteCount * 4) + 3] = PrimaryOam[index + 3];
                SpriteCount++;
            }
        }
        else // 8 sprites have already been found
        {
            // If the sprite is in range, set the sprite overflow, due to glitch Count this may actually be incorrect
            if (spriteY <= Line && spriteY + size > Line && !SpriteOverflowFlag)
            {
                SpriteOverflowFlag = true;
            }
            else if (!SpriteOverflowFlag) // If no sprite in range the increment glitchCount
            {
                glitchCount++;
            }
        }
    }
}

void PPU::RenderPixel()
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
                    SpriteShift0[i] = spShift0 >> 1;
                    SpriteShift1[i] = spShift1 >> 1;
                }
                else
                {
                    pixel = ((spShift0 & 0x80) >> 7) | ((spShift1 & 0x80) >> 6);
                    SpriteShift0[i] = spShift0 << 1;
                    SpriteShift1[i] = spShift1 << 1;
                }

                if (pixel != 0 && !spriteFound)
                {
                    spPixel = pixel;
                    spPriority = !!(spAttribute & 0x20);
                    spPaletteIndex |= ((spAttribute & 0x03) << 2) | spPixel;
                    spriteFound = true;

                    if (i == 0 && SpriteZeroSecondaryOamFlag && !SpriteZeroHitFlag)
                    {
                        SpriteZeroHitFlag = Dot > 1 && Dot != 256 && spPixel != 0 && bgPixel != 0;
                    }
                }
            }
        }
    }

    for (int i = 0; i < 8; ++i)
    {
        --SpriteCounter[i];
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

    colour = Read(paletteIndex);

    DecodePixel(colour);
}

void PPU::RenderPixelIdle()
{
    if (TurboFrameSkip == 0)
    {
        uint16_t colour;

        if (PpuAddress >= 0x3F00 && PpuAddress <= 0x3FFF)
        {
            colour = Read(PpuAddress);
        }
        else
        {
            colour = Read(0x3F00);
        }

        DecodePixel(colour);
    }
}

void PPU::DecodePixel(uint16_t colour)
{
    if (TurboFrameSkip == 0)
    {
        // NTSC rendering is not supported in turbo mode
        if (!TurboModeEnabled && NtscMode)
        {
            uint16_t pixel = colour & 0x3F;
            pixel = pixel | (IntenseRed << 6);
            pixel = pixel | (IntenseGreen << 7);
            pixel = pixel | (IntenseBlue << 8);

            RenderNtscPixel(pixel);
            if (Dot == 256) RenderNtscLine();
        }
        else
        {
            if (VideoOut != nullptr)
            {
                FrameBuffer[FrameBufferIndex++] = RgbLookupTable[colour];
            }
        }
    }
}

void PPU::MaybeChangeModes()
{
    if (TurboModeEnabled != RequestTurboMode)
    {
        TurboModeEnabled = RequestTurboMode;
        if (!TurboModeEnabled)
        {
            TurboFrameSkip = 0;
        }
    }
    else if (NtscMode != RequestNtscMode)
    {
        NtscMode = RequestNtscMode;
    }
}

void PPU::IncrementXScroll()
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

void PPU::IncrementYScroll()
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

void PPU::IncrementClock()
{
    if (Line == 261 && !Even && (ShowSprites || ShowBackground) && Dot == 339)
    {
        Dot = Line = 0;
    }
    else if (Line == 261 && Dot == 340)
    {
        Dot = Line = 0;
    }
    else if (Dot == 340)
    {
        Dot = 0;
        ++Line;
    }
    else
    {
        ++Dot;
    }

    ++Clock;
}

void PPU::LoadBackgroundShiftRegisters()
{
    BackgroundShift0 = BackgroundShift0 | TileBitmapLow;
    BackgroundShift1 = BackgroundShift1 | TileBitmapHigh;
    BackgroundAttributeShift0 = BackgroundAttributeShift0 | ((AttributeByte & 0x1) ? 0xFF : 0x00);
    BackgroundAttributeShift1 = BackgroundAttributeShift1 | ((AttributeByte & 0x2) ? 0xFF : 0x00);
}

void PPU::NameTableFetch()
{
    NameTableByte = ReadNameTable(0x2000 | (PpuAddress & 0x0FFF));
}

void PPU::BackgroundAttributeFetch()
{
    // Get the attribute byte, determines the palette to use when rendering
    AttributeByte = ReadNameTable(0x23C0 | (PpuAddress & 0x0C00) | ((PpuAddress >> 4) & 0x38) | ((PpuAddress >> 2) & 0x07));
    uint8_t attributeShift = (((PpuAddress & 0x0002) >> 1) | ((PpuAddress & 0x0040) >> 5)) << 1;
    AttributeByte = (AttributeByte >> attributeShift) & 0x3;
}

void PPU::BackgroundLowByteFetch()
{
    uint8_t fineY = PpuAddress >> 12; // Get fine y scroll bits from address
    uint16_t patternAddress = static_cast<uint16_t>(NameTableByte) << 4; // Get pattern address, independent of the table
    TileBitmapLow = Cartridge->ChrRead(BaseBackgroundTableAddress + patternAddress + fineY); // Read pattern byte
}

void PPU::BackgroundHighByteFetch()
{
    uint8_t fineY = PpuAddress >> 12; // Get fine y scroll
    uint16_t patternAddress = static_cast<uint16_t>(NameTableByte) << 4; // Get pattern address, independent of the table
    TileBitmapHigh = Cartridge->ChrRead(BaseBackgroundTableAddress + patternAddress + fineY + 8); // Read pattern byte
}

void PPU::SpriteAttributeFetch()
{
    uint8_t sprite = (Dot - 257) / 8;
    SpriteAttribute[sprite] = SecondaryOam[(sprite * 4) + 2];
}

void PPU::SpriteXCoordinateFetch()
{
    uint8_t sprite = (Dot - 257) / 8;
    SpriteCounter[sprite] = SecondaryOam[(sprite * 4) + 3];
}

void PPU::SpriteLowByteFetch()
{
    uint8_t sprite = (Dot - 257) / 8;
    if (sprite < SpriteCount)
    {
        bool flipVertical = !!(0x80 & SpriteAttribute[sprite]);
        uint8_t spriteY = SecondaryOam[sprite * 4];

        if (SpriteSizeSwitch) // if spriteSize is 8x16
        {
            uint16_t base = (0x1 & SecondaryOam[(sprite * 4) + 1]) ? 0x1000 : 0; // Get base pattern table from bit 0 of pattern address
            uint16_t patternIndex = base + ((SecondaryOam[(sprite * 4) + 1] >> 1) << 5); // Index of the beginning of the pattern
            uint16_t offset = flipVertical ? 15 - (Line - spriteY) : (Line - spriteY); // Offset from base index
            if (offset >= 8) offset += 8;

            SpriteShift0[sprite] = Cartridge->ChrRead(patternIndex + offset);
        }
        else
        {
            uint16_t patternIndex = BaseSpriteTableAddress + (SecondaryOam[(sprite * 4) + 1] << 4); // Index of the beginning of the pattern
            uint16_t offset = flipVertical ? 7 - (Line - spriteY) : (Line - spriteY); // Offset from base index

            SpriteShift0[sprite] = Cartridge->ChrRead(patternIndex + offset);
        }
    }
    else
    {
        SpriteShift0[sprite] = 0x00;
    }
}

void PPU::SpriteHighByteFetch()
{
    uint8_t sprite = (Dot - 257) / 8;
    if (sprite < SpriteCount)
    {
        bool flipVertical = !!(0x80 & SpriteAttribute[sprite]);
        uint8_t spriteY = SecondaryOam[sprite * 4];

        if (SpriteSizeSwitch) // if spriteSize is 8x16
        {
            uint16_t base = (0x1 & SecondaryOam[(sprite * 4) + 1]) ? 0x1000 : 0; // Get base pattern table from bit 0 of pattern address
            uint16_t patternIndex = base + ((SecondaryOam[(sprite * 4) + 1] >> 1) << 5); // Index of the beginning of the pattern
            uint16_t offset = flipVertical ? 15 - (Line - spriteY) : (Line - spriteY); // Offset from base index
            if (offset >= 8) offset += 8;

            SpriteShift1[sprite] = Cartridge->ChrRead(patternIndex + offset + 8);
        }
        else
        {
            uint16_t patternIndex = BaseSpriteTableAddress + (SecondaryOam[(sprite * 4) + 1] << 4); // Index of the beginning of the pattern
            uint16_t offset = flipVertical ? 7 - (Line - spriteY) : (Line - spriteY); // Offset from base index

            SpriteShift1[sprite] = Cartridge->ChrRead(patternIndex + offset + 8);
        }
    }
    else
    {
        SpriteShift1[sprite] = 0x00;
    }
}

uint8_t PPU::Read(uint16_t address)
{
    uint16_t addr = address % 0x4000;

    if (addr < 0x2000)
    {
        return Cartridge->ChrRead(addr);
    }
    else if (addr >= 0x2000 && addr < 0x3F00)
    {
        return ReadNameTable(addr);
    }
    else
    {
        if (addr % 4 == 0) addr &= 0xFF0F;  // Ignore second nibble if addr is a multiple of 4
        return PaletteTable[addr % 0x20];
    }
}

void PPU::Write(uint16_t address, uint8_t value)
{
    uint16_t addr = address % 0x4000;

    if (addr < 0x2000)
    {
        Cartridge->ChrWrite(value, addr);
    }
    else if (addr >= 0x2000 && addr < 0x3F00)
    {
        WriteNameTable(addr, value);
    }
    else
    {
        if (addr % 4 == 0) addr &= 0xFF0F;  // Ignore second nibble if addr is a multiple of 4
        PaletteTable[addr % 0x20] = value & 0x3F; // Ignore bits 6 and 7
    }
}

uint8_t PPU::ReadNameTable(uint16_t address)
{
    uint16_t nametableaddr = address % 0x2000;

    Cart::MirrorMode mode = Cartridge->GetMirrorMode();

    switch (mode)
    {
    case Cart::MirrorMode::SINGLE_SCREEN_A:

        return NameTable0[nametableaddr % 0x400];

    case Cart::MirrorMode::SINGLE_SCREEN_B:

        return NameTable1[nametableaddr % 0x400];

    case Cart::MirrorMode::HORIZONTAL:

        if (nametableaddr < 0x800)
        {
            return NameTable0[nametableaddr % 0x400];
        }
        else
        {
            return NameTable1[nametableaddr % 0x400];
        }

    case Cart::MirrorMode::VERTICAL:

        if (nametableaddr < 0x400 || (nametableaddr >= 0x800 && nametableaddr < 0xC00))
        {
            return NameTable0[nametableaddr % 0x400];
        }
        else
        {
            return NameTable1[nametableaddr % 0x400];
        }
    }

    return 0;
}

void PPU::WriteNameTable(uint16_t address, uint8_t value)
{
    uint16_t nametableaddr = address % 0x2000;

    Cart::MirrorMode mode = Cartridge->GetMirrorMode();

    switch (mode)
    {
    case Cart::MirrorMode::SINGLE_SCREEN_A:

        NameTable0[nametableaddr % 0x400] = 0;
        break;

    case Cart::MirrorMode::SINGLE_SCREEN_B:

        NameTable1[nametableaddr % 0x400] = 0;
        break;

    case Cart::MirrorMode::HORIZONTAL:

        if (nametableaddr < 0x800)
        {
            NameTable0[nametableaddr % 0x400] = value;
        }
        else
        {
            NameTable1[nametableaddr % 0x400] = value;
        }
        break;

    case Cart::MirrorMode::VERTICAL:

        if (nametableaddr < 0x400 || (nametableaddr >= 0x800 && nametableaddr < 0xC00))
        {
            NameTable0[nametableaddr % 0x400] = value;
        }
        else
        {
            NameTable1[nametableaddr % 0x400] = value;
        }
        break;
    }
}

void PPU::UpdateFrameRate()
{
    using namespace std::chrono;

    steady_clock::time_point now = steady_clock::now();
    microseconds time_span = duration_cast<microseconds>(now - FrameCountStart);
    if (time_span.count() >= 1000000)
    {
        CurrentFps = FpsCounter;
        FpsCounter = 0;
        FrameCountStart = steady_clock::now();

        if (VideoOut != nullptr)
        {
            VideoOut->SetFps(CurrentFps);
        }
    }
    else
    {
        FpsCounter++;
    }
}

void PPU::UpdateFrameSkipCounters()
{
    if (TurboModeEnabled)
    {
        if (TurboFrameSkip == 0)
        {
            TurboFrameSkip = 20;
        }
        else
        {
            TurboFrameSkip--;
        }
    }
}
