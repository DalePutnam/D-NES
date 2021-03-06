#pragma once

#include "mapper_base.h"

class CPU;

class MMC1 : public MapperBase
{
public:
    MMC1(iNesFile& file);

    void SaveState(StateSave::Ptr& state) override;
    void LoadState(const StateSave::Ptr& state) override;

    uint8_t CpuRead(uint16_t address) override;
    void CpuWrite(uint8_t M, uint16_t address) override;

    uint8_t PpuRead() override;
    void PpuWrite(uint8_t M) override;

    uint8_t PpuPeek(uint16_t address) override;

private:
    uint8_t NameTableRead(uint16_t address);
    void NameTableWrite(uint8_t M, uint16_t address);
    uint8_t ChrRead(uint16_t address);
    void ChrWrite(uint8_t M, uint16_t address);

    void UpdatePageOffsets();

    uint64_t _lastWriteCycle;
    uint8_t _cycleCounter;
    uint8_t _tempRegister;

    uint8_t* _chr;
    uint32_t _chrSize;
    uint8_t _mirroring;

	const uint8_t* _prgPage0Pointer;
	const uint8_t* _prgPage1Pointer;
	uint8_t* _chrPage0Pointer;
	uint8_t* _chrPage1Pointer;

	uint8_t _prgPage;
	uint8_t _chrPage0;
	uint8_t _chrPage1;
	bool _wramDisable;
	bool _prgLastPageFixed;
	bool _prgPageSize16K;
	bool _chrPageSize4K;
};
