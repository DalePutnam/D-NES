#pragma once

#include "mapper_base.h"

class CPU;

class SXROM : public MapperBase
{
public:
    SXROM(iNesFile& file);

    void SaveState(StateSave::Ptr& state) override;
    void LoadState(const StateSave::Ptr& state) override;

    uint8_t CpuRead(uint16_t address) override;
    void CpuWrite(uint8_t M, uint16_t address) override;

protected:
    uint8_t NameTableRead(uint16_t address) override;
    void NameTableWrite(uint8_t M, uint16_t address) override;
    uint8_t ChrRead(uint16_t address) override;
    void ChrWrite(uint8_t M, uint16_t address) override;

private:
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

	void UpdatePageOffsets();
};
