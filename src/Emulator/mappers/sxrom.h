#pragma once

#include "mapper_base.h"

class CPU;

class SXROM : public MapperBase
{
public:
    SXROM(iNesFile& file);
    virtual ~SXROM();

    void SaveState(StateSave::Ptr& state) override;
    void LoadState(const StateSave::Ptr& state) override;

    //Cart::MirrorMode GetMirrorMode() override;

    uint8_t PrgRead(uint16_t address) override;
    void PrgWrite(uint8_t M, uint16_t address) override;

    uint8_t ChrRead(uint16_t address) override;
    void ChrWrite(uint8_t M, uint16_t address) override;

private:
    uint64_t _lastWriteCycle;
    uint8_t _cycleCounter;
    uint8_t _tempRegister;

    uint8_t* _chr;
    uint32_t _chrSize;

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
