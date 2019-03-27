#pragma once

#include "mapper_base.h"

class CPU;

class SXROM : public MapperBase
{
public:
    SXROM(const std::string& fileName, const std::string& saveDir);
    virtual ~SXROM();

    void SaveState(StateSave::Ptr& state) override;
    void LoadState(const StateSave::Ptr& state) override;

    //Cart::MirrorMode GetMirrorMode() override;

    uint8_t PrgRead(uint16_t address) override;
    void PrgWrite(uint8_t M, uint16_t address) override;

    uint8_t ChrRead(uint16_t address) override;
    void ChrWrite(uint8_t M, uint16_t address) override;

private:
    uint64_t LastWriteCycle;
    uint8_t Counter;
    uint8_t TempRegister;

	const uint8_t* PrgPage0Pointer;
	const uint8_t* PrgPage1Pointer;
	uint8_t* ChrPage0Pointer;
	uint8_t* ChrPage1Pointer;

	uint8_t PrgPage;
	uint8_t ChrPage0;
	uint8_t ChrPage1;
	bool WramDisable;
	bool PrgLastPageFixed;
	bool PrgPageSize16K;
	bool ChrPageSize4K;

	void UpdatePageOffsets();
};
