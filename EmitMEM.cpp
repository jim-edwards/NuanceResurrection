#include "basetypes.h"
#include "byteswap.h"
#include "EmitMisc.h"
#include "EmitMEM.h"
#include "ExecuteMEM.h"
#include "InstructionCache.h"
#include "mpe.h"
#include "NativeCodeCache.h"
#include "NuonEnvironment.h"
#include "X86EmitTypes.h"

extern bool SSSE3_supported;
extern NuonEnvironment nuonEnv;

static const __m128i bswap_lut = _mm_set_epi8(12,13,14,15, 8,9,10,11, 4,5,6,7, 0,1,2,3); //_mm_setr_epi8(3,2,1,0, 7,6,5,4, 11,10,9,8, 15,14,13,12);

void EmitControlRegisterLoad(EmitterVariables * const vars, const uint32 address, const x86Reg destReg);
void EmitControlRegisterStore(EmitterVariables * const vars, const uint32 address, const x86Reg destReg);
void EmitControlRegisterStoreImmediate(EmitterVariables * const vars, const uint32 address, const uint32 imm);

void Emit_Mirror(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  if(vars->scalarRegOutDep)
  {
    vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
    vars->mpe->nativeCodeCache.X86Emit_MOVIR(16, x86Reg::x86Reg_ecx);
    vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
    vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xAAAAAAAA, x86Reg::x86Reg_eax);
    vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x55555555, x86Reg::x86Reg_ebx);
    vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_eax, 1);
    vars->mpe->nativeCodeCache.X86Emit_SHLIR(x86Reg::x86Reg_ebx, 1);
    vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);

    vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
    vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xCCCCCCCC, x86Reg::x86Reg_eax);
    vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x33333333, x86Reg::x86Reg_ebx);
    vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_eax, 2);
    vars->mpe->nativeCodeCache.X86Emit_SHLIR(x86Reg::x86Reg_ebx, 2);
    vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);

    vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
    vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xF0F0F0F0, x86Reg::x86Reg_eax);
    vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x0F0F0F0F, x86Reg::x86Reg_ebx);
    vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_eax, 4);
    vars->mpe->nativeCodeCache.X86Emit_SHLIR(x86Reg::x86Reg_ebx, 4);
    vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);

    vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
    vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFF00FF00, x86Reg::x86Reg_eax);
    vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x00FF00FF, x86Reg::x86Reg_ebx);
    vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_eax, 8);
    vars->mpe->nativeCodeCache.X86Emit_SHLIR(x86Reg::x86Reg_ebx, 8);
    vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);

    vars->mpe->nativeCodeCache.X86Emit_ROLRR(x86Reg::x86Reg_eax);
    vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  }
}

void Emit_MV_SImmediate(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 imm = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  if(vars->scalarRegOutDep)
  {
    vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm, x86MemPtr::x86MemPtr_dword, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  }
}

void Emit_MV_SScalar(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  if(vars->scalarRegOutDep)
  {
    vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
    vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  }
}

void Emit_MV_V(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 src1RegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg src1RegReadBaseReg_0 = GetScalarRegReadBaseReg(vars,src1RegIndex);
  const x86BaseReg src1RegReadBaseReg_1 = GetScalarRegReadBaseReg(vars,src1RegIndex+1);
  const x86BaseReg src1RegReadBaseReg_2 = GetScalarRegReadBaseReg(vars,src1RegIndex+2);
  const x86BaseReg src1RegReadBaseReg_3 = GetScalarRegReadBaseReg(vars,src1RegIndex+3);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 src1RegDisp = GetScalarRegEmitDisp(vars,src1RegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  if((src1RegReadBaseReg_0 == src1RegReadBaseReg_1) && (src1RegReadBaseReg_2 == src1RegReadBaseReg_3) && (src1RegReadBaseReg_0 == src1RegReadBaseReg_2))
  {
    vars->mpe->nativeCodeCache.X86Emit_MOVDQUMR(x86Reg::x86Reg_xmm0, src1RegReadBaseReg_0, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, src1RegDisp);
    vars->mpe->nativeCodeCache.X86Emit_MOVDQURM(x86Reg::x86Reg_xmm0, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  }
  else
  {
    vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, src1RegReadBaseReg_0, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, src1RegDisp);
    vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, src1RegReadBaseReg_1, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, src1RegDisp+4);
    vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, src1RegReadBaseReg_2, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, src1RegDisp+8);
    vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, src1RegReadBaseReg_3, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, src1RegDisp+12);

    vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
    vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+4);
    vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+8);
    vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+12);
  }
}

void Emit_LoadByteAbsolute(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcAddress = nuance.fields[FIELD_MEM_POINTER];
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  if(vars->scalarRegOutDep)
  {
    vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcAddress);
    vars->mpe->nativeCodeCache.X86Emit_SHLIR(x86Reg::x86Reg_eax, 24);
    vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  }
}

void Emit_LoadByteLinear(EmitterVariables * const vars, const Nuance &nuance)
{
  constexpr uint32 l_not_control_reg = 0;

  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.patchMgr.Reset();

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFF00000, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(MPE_CTRL_BASE, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_JCC_Label(X86_CC_NZ, l_not_control_reg);
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(1, x86MemPtr::x86MemPtr_dword, (uint32)&(vars->mpe->interpretNextPacket));
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(vars->pInstructionEntry->packet->pcexec, x86MemPtr::x86MemPtr_dword, (uint32)&(vars->mpe->pcexec));
  Emit_ExitBlock(vars);
  vars->mpe->nativeCodeCache.SetLabelPointer(l_not_control_reg);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_SHLIR(x86Reg::x86Reg_ebp, 24);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);

  vars->mpe->nativeCodeCache.patchMgr.ApplyPatches();
}

void Emit_LoadByteBilinearXY(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg rxRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RX);
  const x86BaseReg ryRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RY);
  const x86BaseReg xyctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_XYCTL);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 rxRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
  const int32 ryRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
  const int32 xyctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, rxRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rxRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, ryRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ryRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, xyctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, xyctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB 
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->xybase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_SHLIR(x86Reg::x86Reg_ebp, 24);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
}

void Emit_LoadByteBilinearUV(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg ruRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RU);
  const x86BaseReg rvRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RV);
  const x86BaseReg uvctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_UVCTL);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 ruRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
  const int32 rvRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
  const int32 uvctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, ruRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ruRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, rvRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rvRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, uvctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, uvctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->uvbase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_SHLIR(x86Reg::x86Reg_ebp, 24);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
}

void Emit_LoadWordAbsolute(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcAddress = nuance.fields[FIELD_MEM_POINTER];
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  if(vars->scalarRegOutDep)
  {
    vars->mpe->nativeCodeCache.X86Emit_MOVZXMR(x86Reg::x86Reg_eax, x86MemPtr::x86MemPtr_word, srcAddress);
    vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
    vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  }
}

void Emit_LoadWordLinear(EmitterVariables * const vars, const Nuance &nuance)
{
  constexpr uint32 l_not_control_reg = 0;

  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.patchMgr.Reset();

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFF00000, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(MPE_CTRL_BASE, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_JCC_Label(X86_CC_NZ, l_not_control_reg);
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(1, x86MemPtr::x86MemPtr_dword, (uint32)&(vars->mpe->interpretNextPacket));
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(vars->pInstructionEntry->packet->pcexec, x86MemPtr::x86MemPtr_dword, (uint32)&(vars->mpe->pcexec));
  Emit_ExitBlock(vars);
  vars->mpe->nativeCodeCache.SetLabelPointer(l_not_control_reg);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);

  vars->mpe->nativeCodeCache.patchMgr.ApplyPatches();
}

void Emit_LoadWordBilinearXY(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg rxRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RX);
  const x86BaseReg ryRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RY);
  const x86BaseReg xyctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_XYCTL);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 rxRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
  const int32 ryRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
  const int32 xyctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, rxRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rxRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, ryRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ryRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, xyctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, xyctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->xybase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
}

void Emit_LoadWordBilinearUV(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg ruRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RU);
  const x86BaseReg rvRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RV);
  const x86BaseReg uvctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_UVCTL);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 ruRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
  const int32 rvRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
  const int32 uvctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, ruRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ruRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, rvRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rvRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, uvctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, uvctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->uvbase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
}

void Emit_LoadScalarAbsolute(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcAddress = nuance.fields[FIELD_MEM_POINTER];
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  if(vars->scalarRegOutDep)
  {
    vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcAddress);
    vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
    vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  }
}

void Emit_LoadScalarControlRegisterAbsolute(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcAddress = nuance.fields[FIELD_MEM_FROM];

  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  if(vars->scalarRegOutDep)
  {
    EmitControlRegisterLoad(vars, srcAddress, x86Reg::x86Reg_eax);
    vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  }
}

void Emit_LoadScalarLinear(EmitterVariables * const vars, const Nuance &nuance)
{
  constexpr uint32 l_not_control_reg = 0;

  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.patchMgr.Reset();

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFF00000, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(MPE_CTRL_BASE, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_JCC_Label(X86_CC_NZ, l_not_control_reg);
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(1, x86MemPtr::x86MemPtr_dword, (uint32)&(vars->mpe->interpretNextPacket));
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(vars->pInstructionEntry->packet->pcexec, x86MemPtr::x86MemPtr_dword, (uint32)&(vars->mpe->pcexec));
  Emit_ExitBlock(vars);
  vars->mpe->nativeCodeCache.SetLabelPointer(l_not_control_reg);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFC, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);

  vars->mpe->nativeCodeCache.patchMgr.ApplyPatches();
}

void Emit_LoadScalarBilinearXY(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg rxRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RX);
  const x86BaseReg ryRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RY);
  const x86BaseReg xyctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_XYCTL);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 rxRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
  const int32 ryRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
  const int32 xyctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, rxRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rxRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, ryRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ryRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, xyctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, xyctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->xybase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
}

void Emit_LoadScalarBilinearUV(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg ruRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RU);
  const x86BaseReg rvRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RV);
  const x86BaseReg uvctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_UVCTL);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 ruRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
  const int32 rvRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
  const int32 uvctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, ruRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ruRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, rvRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rvRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, uvctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, uvctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->uvbase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
}

void Emit_LoadShortVectorAbsolute(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcAddress = nuance.fields[FIELD_MEM_POINTER];
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcAddress);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, srcAddress+2);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcAddress+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, srcAddress+6);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR(0xFFFF0000, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ANDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_ANDRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_ANDRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_ANDRR(x86Reg::x86Reg_edx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+12);
}

void Emit_LoadShortVectorLinear(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFF8, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 2);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 6);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000, x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+12);
}

void Emit_LoadShortVectorBilinearXY(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg rxRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RX);
  const x86BaseReg ryRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RY);
  const x86BaseReg xyctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_XYCTL);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 rxRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
  const int32 ryRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
  const int32 xyctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, rxRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rxRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, ryRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ryRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, xyctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, xyctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->xybase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 2);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 6);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000, x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+12);
}

void Emit_LoadShortVectorBilinearUV(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg ruRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RU);
  const x86BaseReg rvRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RV);
  const x86BaseReg uvctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_UVCTL);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 ruRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
  const int32 rvRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
  const int32 uvctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, ruRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ruRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, rvRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rvRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, uvctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, uvctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->uvbase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 2);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 6);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000, x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+12);
}

void Emit_LoadVectorAbsolute(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcAddress = nuance.fields[FIELD_MEM_POINTER];
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  if(SSSE3_supported)
  {
  if((srcAddress & 15) == 0)
    vars->mpe->nativeCodeCache.X86Emit_MOVDQAMR(x86Reg::x86Reg_xmm0, srcAddress);
  else
    vars->mpe->nativeCodeCache.X86Emit_MOVDQUMR(x86Reg::x86Reg_xmm0, srcAddress);
  vars->mpe->nativeCodeCache.X86Emit_MOVDQAMR(x86Reg::x86Reg_xmm1, (uint32)&bswap_lut);
  vars->mpe->nativeCodeCache.X86Emit_PSHUFBRR(x86Reg::x86Reg_xmm0, x86Reg::x86Reg_xmm1);
  vars->mpe->nativeCodeCache.X86Emit_MOVDQURM(x86Reg::x86Reg_xmm0, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  }
  else
  {
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcAddress);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, srcAddress+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcAddress+8);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, srcAddress+12);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+12);
  }
}

void Emit_LoadVectorLinear(EmitterVariables * const vars, const Nuance &nuance)
{
  constexpr uint32 l_not_control_reg = 0;

  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.patchMgr.Reset();

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFF00000, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(MPE_CTRL_BASE, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_JCC_Label(X86_CC_NZ, l_not_control_reg);
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(1, x86MemPtr::x86MemPtr_dword, (uint32)&(vars->mpe->interpretNextPacket));
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(vars->pInstructionEntry->packet->pcexec, x86MemPtr::x86MemPtr_dword, (uint32)&(vars->mpe->pcexec));
  Emit_ExitBlock(vars);
  vars->mpe->nativeCodeCache.SetLabelPointer(l_not_control_reg);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);

  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFF0, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 12);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+12);

  vars->mpe->nativeCodeCache.patchMgr.ApplyPatches();
}

void Emit_LoadVectorBilinearXY(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg rxRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RX);
  const x86BaseReg ryRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RY);
  const x86BaseReg xyctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_XYCTL);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 rxRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
  const int32 ryRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
  const int32 xyctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, rxRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rxRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, ryRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ryRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, xyctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, xyctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->xybase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 12);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+12);
}

void Emit_LoadVectorBilinearUV(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg ruRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RU);
  const x86BaseReg rvRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RV);
  const x86BaseReg uvctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_UVCTL);
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 ruRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
  const int32 rvRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
  const int32 uvctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, ruRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ruRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, rvRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rvRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, uvctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, uvctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->uvbase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 12);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+12);
}

void Emit_LoadPixelAbsolute(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  //const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  //const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, (uint32)&vars->mpe->linpixctl);
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(destRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, (uint32)&vars->mpe->ba_control);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR(nuance.fields[FIELD_MEM_POINTER], x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_LoadPixelAbsolute,0);
}

void Emit_LoadPixelZAbsolute(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  //const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  //const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, (uint32)&vars->mpe->linpixctl);
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(destRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, (uint32)&vars->mpe->ba_control);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR(nuance.fields[FIELD_MEM_POINTER], x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_LoadPixelZAbsolute,0);
}

void Emit_LoadPixelLinear(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  //const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  //const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe->linpixctl);
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(destRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, (uint32)&vars->mpe->ba_control);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx); //!! in case of needing this again: the eax in here was replaced for all the other code with edx (first emit line above)
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);

  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_edx, x86Reg::x86Reg_ebx); // edx passed in below

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_LoadPixelAbsolute,0);
}

void Emit_LoadPixelZLinear(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  //const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  //const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe->linpixctl);
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(destRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, (uint32)&vars->mpe->ba_control);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx); //!! in case of needing this again: the eax in here was replaced for all the other code with edx (first emit line above)
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);

  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_edx, x86Reg::x86Reg_ebx); // edx passed in below

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_LoadPixelZAbsolute,0);
}

void Emit_LoadPixelBilinearUV(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg ruRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RU);
  const x86BaseReg rvRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RV);
  const x86BaseReg uvctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_UVCTL);
  //const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  //const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 ruRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
  const int32 rvRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
  const int32 uvctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, ruRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ruRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, rvRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rvRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, uvctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, uvctlRegDisp); // passed directly into GetBilinearAddress
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, (uint32)&vars->mpe->ba_control); // needed by _LoadPixelAbsolute
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(destRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);

  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, (uint32)&vars->mpe->ba_mipped_xoffset); // store the 4bit pixel addressing bit
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->uvbase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_edx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_LoadPixelAbsolute,0);
}

void Emit_LoadPixelZBilinearUV(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg ruRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RU);
  const x86BaseReg rvRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RV);
  const x86BaseReg uvctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_UVCTL);
  //const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  //const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 ruRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
  const int32 rvRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
  const int32 uvctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, ruRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ruRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, rvRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rvRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, uvctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, uvctlRegDisp); // passed directly into GetBilinearAddress
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, (uint32)&vars->mpe->ba_control); // needed by _LoadPixelZAbsolute
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(destRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);

  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->uvbase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_edx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_LoadPixelZAbsolute,0);
}

void Emit_LoadPixelBilinearXY(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg rxRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RX);
  const x86BaseReg ryRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RY);
  const x86BaseReg xyctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_XYCTL);
  //const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  //const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 rxRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
  const int32 ryRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
  const int32 xyctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, rxRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rxRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, ryRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ryRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, xyctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, xyctlRegDisp); // passed directly into GetBilinearAddress
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, (uint32)&vars->mpe->ba_control); // needed by _LoadPixelAbsolute
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(destRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);

  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, (uint32)&vars->mpe->ba_mipped_xoffset); // store the 4bit pixel addressing bit
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->xybase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_edx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_LoadPixelAbsolute,0);
}

void Emit_LoadPixelZBilinearXY(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg rxRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RX);
  const x86BaseReg ryRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RY);
  const x86BaseReg xyctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_XYCTL);
  //const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  //const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 rxRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
  const int32 ryRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
  const int32 xyctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, rxRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rxRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, ryRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ryRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, xyctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, xyctlRegDisp); // passed directly into GetBilinearAddress
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, (uint32)&vars->mpe->ba_control); // needed by _LoadPixelZAbsolute
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(destRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);

  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->xybase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_edx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_LoadPixelZAbsolute,0);
}

void Emit_StoreScalarImmediate(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destAddress = nuance.fields[FIELD_MEM_POINTER];
  const uint32 imm = SwapBytes((uint32)nuance.fields[FIELD_MEM_FROM]);

  vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm, x86MemPtr::x86MemPtr_dword, destAddress);
}

void Emit_StoreScalarControlRegisterImmediate(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destAddress = nuance.fields[FIELD_MEM_TO];
  const uint32 imm = nuance.fields[FIELD_MEM_FROM];

  EmitControlRegisterStoreImmediate(vars, destAddress, imm);
}

void Emit_StoreScalarControlRegisterAbsolute(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destAddress = nuance.fields[FIELD_MEM_TO];
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];

  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  EmitControlRegisterStore(vars, destAddress, x86Reg::x86Reg_eax);
}

void Emit_StoreScalarAbsolute(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destAddress = nuance.fields[FIELD_MEM_POINTER];
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destAddress);
}

void Emit_StoreScalarLinear(EmitterVariables * const vars, const Nuance &nuance)
{
  constexpr uint32 l_not_control_reg = 0;

  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg destRegReadBaseReg = GetScalarRegReadBaseReg(vars,destRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.patchMgr.Reset();

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, destRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFF00000, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(MPE_CTRL_BASE, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_JCC_Label(X86_CC_NZ, l_not_control_reg);
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(1, x86MemPtr::x86MemPtr_dword, (uint32)&(vars->mpe->interpretNextPacket));
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(vars->pInstructionEntry->packet->pcexec, x86MemPtr::x86MemPtr_dword, (uint32)&(vars->mpe->pcexec));
  Emit_ExitBlock(vars);
  vars->mpe->nativeCodeCache.SetLabelPointer(l_not_control_reg);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFC, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx);

  vars->mpe->nativeCodeCache.patchMgr.ApplyPatches();
}

void Emit_StoreScalarBilinearXY(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg rxRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RX);
  const x86BaseReg ryRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RY);
  const x86BaseReg xyctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_XYCTL);
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 rxRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
  const int32 ryRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
  const int32 xyctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, rxRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rxRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, ryRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ryRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, xyctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, xyctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->xybase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx,x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx);
}

void Emit_StoreScalarBilinearUV(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg ruRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RU);
  const x86BaseReg rvRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RV);
  const x86BaseReg uvctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_UVCTL);
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 ruRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
  const int32 rvRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
  const int32 uvctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, ruRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ruRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, rvRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rvRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, uvctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, uvctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->uvbase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx,x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx);
}

void Emit_StoreShortVectorAbsolute(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const uint32 destAddress = nuance.fields[FIELD_MEM_POINTER];
  const x86BaseReg srcRegReadBaseReg_0 = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg srcRegReadBaseReg_1 = GetScalarRegReadBaseReg(vars,srcRegIndex+1);
  const x86BaseReg srcRegReadBaseReg_2 = GetScalarRegReadBaseReg(vars,srcRegIndex+2);
  const x86BaseReg srcRegReadBaseReg_3 = GetScalarRegReadBaseReg(vars,srcRegIndex+3);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg_0, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, srcRegReadBaseReg_1, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_2, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, srcRegReadBaseReg_3, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+12);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ax, destAddress);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_bx, destAddress+2);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_cx, destAddress+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_dx, destAddress+6);
}

void Emit_StoreShortVectorLinear(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg_0 = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg srcRegReadBaseReg_1 = GetScalarRegReadBaseReg(vars,srcRegIndex+1);
  const x86BaseReg srcRegReadBaseReg_2 = GetScalarRegReadBaseReg(vars,srcRegIndex+2);
  const x86BaseReg srcRegReadBaseReg_3 = GetScalarRegReadBaseReg(vars,srcRegIndex+3);
  const x86BaseReg destRegReadBaseReg = GetScalarRegReadBaseReg(vars,destRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, destRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx,x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFF8, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, srcRegReadBaseReg_0, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_1, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, srcRegReadBaseReg_2, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, srcRegReadBaseReg_3, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+12);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_bx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_cx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 2);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_dx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_bp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 6);
}

void Emit_StoreShortVectorBilinearXY(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg rxRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RX);
  const x86BaseReg ryRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RY);
  const x86BaseReg xyctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_XYCTL);
  const x86BaseReg srcRegReadBaseReg_0 = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg srcRegReadBaseReg_1 = GetScalarRegReadBaseReg(vars,srcRegIndex+1);
  const x86BaseReg srcRegReadBaseReg_2 = GetScalarRegReadBaseReg(vars,srcRegIndex+2);
  const x86BaseReg srcRegReadBaseReg_3 = GetScalarRegReadBaseReg(vars,srcRegIndex+3);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 rxRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
  const int32 ryRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
  const int32 xyctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, rxRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rxRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, ryRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ryRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, xyctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, xyctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->xybase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_0, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, srcRegReadBaseReg_1, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, srcRegReadBaseReg_2, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_cx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_3, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+12);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_dx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 2);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_bp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_cx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 6);
}

void Emit_StoreShortVectorBilinearUV(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg ruRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RU);
  const x86BaseReg rvRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RV);
  const x86BaseReg uvctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_UVCTL);
  const x86BaseReg srcRegReadBaseReg_0 = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg srcRegReadBaseReg_1 = GetScalarRegReadBaseReg(vars,srcRegIndex+1);
  const x86BaseReg srcRegReadBaseReg_2 = GetScalarRegReadBaseReg(vars,srcRegIndex+2);
  const x86BaseReg srcRegReadBaseReg_3 = GetScalarRegReadBaseReg(vars,srcRegIndex+3);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 ruRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
  const int32 rvRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
  const int32 uvctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, ruRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ruRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, rvRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rvRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, uvctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, uvctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->uvbase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx,x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFC, x86Reg::x86Reg_eax);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_0, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, srcRegReadBaseReg_1, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, srcRegReadBaseReg_2, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_cx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_3, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+12);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_dx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 2);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_bp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_cx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 6);
}

void Emit_StoreVectorAbsolute(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destAddress = nuance.fields[FIELD_MEM_POINTER];
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg_0 = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg srcRegReadBaseReg_1 = GetScalarRegReadBaseReg(vars,srcRegIndex+1);
  const x86BaseReg srcRegReadBaseReg_2 = GetScalarRegReadBaseReg(vars,srcRegIndex+2);
  const x86BaseReg srcRegReadBaseReg_3 = GetScalarRegReadBaseReg(vars,srcRegIndex+3);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);

  if (SSSE3_supported && (srcRegReadBaseReg_0 == srcRegReadBaseReg_1) && (srcRegReadBaseReg_2 == srcRegReadBaseReg_3) && (srcRegReadBaseReg_0 == srcRegReadBaseReg_2))
  {
  vars->mpe->nativeCodeCache.X86Emit_MOVDQUMR(x86Reg::x86Reg_xmm0, srcRegReadBaseReg_0, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVDQAMR(x86Reg::x86Reg_xmm1, (uint32)&bswap_lut);
  vars->mpe->nativeCodeCache.X86Emit_PSHUFBRR(x86Reg::x86Reg_xmm0, x86Reg::x86Reg_xmm1);
  if((destAddress & 15) == 0)
    vars->mpe->nativeCodeCache.X86Emit_MOVDQARM(x86Reg::x86Reg_xmm0, destAddress);
  else
    vars->mpe->nativeCodeCache.X86Emit_MOVDQURM(x86Reg::x86Reg_xmm0, destAddress);
  }
  else
  {
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg_0, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, srcRegReadBaseReg_1, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_2, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, srcRegReadBaseReg_3, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+12);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destAddress);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, destAddress+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destAddress+8);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, destAddress+12);
  }
}

void Emit_StoreVectorLinear(EmitterVariables * const vars, const Nuance &nuance)
{
  constexpr uint32 l_not_control_reg = 0;

  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg_0 = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg srcRegReadBaseReg_1 = GetScalarRegReadBaseReg(vars,srcRegIndex+1);
  const x86BaseReg srcRegReadBaseReg_2 = GetScalarRegReadBaseReg(vars,srcRegIndex+2);
  const x86BaseReg srcRegReadBaseReg_3 = GetScalarRegReadBaseReg(vars,srcRegIndex+3);
  const x86BaseReg destRegReadBaseReg = GetScalarRegReadBaseReg(vars,destRegIndex);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.patchMgr.Reset();

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, destRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFF00000, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(MPE_CTRL_BASE, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_JCC_Label(X86_CC_NZ, l_not_control_reg);
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(1, x86MemPtr::x86MemPtr_dword, (uint32)&(vars->mpe->interpretNextPacket));
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(vars->pInstructionEntry->packet->pcexec, x86MemPtr::x86MemPtr_dword, (uint32)&(vars->mpe->pcexec));
  Emit_ExitBlock(vars);
  vars->mpe->nativeCodeCache.SetLabelPointer(l_not_control_reg);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFF0, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_0, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, srcRegReadBaseReg_1, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, srcRegReadBaseReg_2, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_3, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+12);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 12);
  vars->mpe->nativeCodeCache.patchMgr.ApplyPatches();
}

void Emit_StorePixelAbsolute(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  //const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  //const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  //const x86BaseReg destRegReadBaseReg = GetScalarRegReadBaseReg(vars,destRegIndex);
  //const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  //const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe->linpixctl);
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(srcRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, (uint32)&vars->mpe->ba_control);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR(nuance.fields[FIELD_MEM_POINTER], x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_StorePixelAbsolute,0);
}

void Emit_StoreVectorBilinearXY(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg rxRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RX);
  const x86BaseReg ryRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RY);
  const x86BaseReg xyctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_XYCTL);
  const x86BaseReg srcRegReadBaseReg_0 = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg srcRegReadBaseReg_1 = GetScalarRegReadBaseReg(vars,srcRegIndex+1);
  const x86BaseReg srcRegReadBaseReg_2 = GetScalarRegReadBaseReg(vars,srcRegIndex+2);
  const x86BaseReg srcRegReadBaseReg_3 = GetScalarRegReadBaseReg(vars,srcRegIndex+3);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 rxRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
  const int32 ryRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
  const int32 xyctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, rxRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rxRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, ryRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ryRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, xyctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, xyctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->xybase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx,x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_0, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, srcRegReadBaseReg_1, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, srcRegReadBaseReg_2, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_3, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+12);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 12);
}

void Emit_StoreVectorBilinearUV(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg ruRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RU);
  const x86BaseReg rvRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RV);
  const x86BaseReg uvctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_UVCTL);
  const x86BaseReg srcRegReadBaseReg_0 = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg srcRegReadBaseReg_1 = GetScalarRegReadBaseReg(vars,srcRegIndex+1);
  const x86BaseReg srcRegReadBaseReg_2 = GetScalarRegReadBaseReg(vars,srcRegIndex+2);
  const x86BaseReg srcRegReadBaseReg_3 = GetScalarRegReadBaseReg(vars,srcRegIndex+3);
  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 ruRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
  const int32 rvRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
  const int32 uvctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, ruRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ruRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, rvRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rvRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, uvctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, uvctlRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->uvbase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx,x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_0, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, srcRegReadBaseReg_1, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebp, srcRegReadBaseReg_2, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_3, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+12);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_eax, x86IndexReg::x86IndexReg_ebx, x86ScaleVal::x86Scale_1, 12);
}

void Emit_StorePixelZAbsolute(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  //const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  //const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  //const x86BaseReg destRegReadBaseReg = GetScalarRegReadBaseReg(vars,destRegIndex);
  //const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  //const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe->linpixctl);
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(srcRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, (uint32)&vars->mpe->ba_control);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR(nuance.fields[FIELD_MEM_POINTER], x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_StorePixelZAbsolute,0);
}

void Emit_StorePixelLinear(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  //const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg destRegReadBaseReg = GetScalarRegReadBaseReg(vars,destRegIndex);
  //const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, destRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe->linpixctl);
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(srcRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, (uint32)&vars->mpe->ba_control);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx); //!! in case of needing this again: the eax in here was replaced for all the other code with edx (first emit line above)
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);

  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_edx, x86Reg::x86Reg_ebx); // edx passed in below

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_StorePixelAbsolute,0);
}

void Emit_StorePixelZLinear(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  //const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg destRegReadBaseReg = GetScalarRegReadBaseReg(vars,destRegIndex);
  //const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, destRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe->linpixctl);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, (uint32)&vars->mpe->ba_control);
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(srcRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);

  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_edx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_StorePixelZAbsolute,0);
}

void Emit_StorePixelBilinearUV(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg ruRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RU);
  const x86BaseReg rvRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RV);
  const x86BaseReg uvctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_UVCTL);
  //const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  //const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 ruRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
  const int32 rvRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
  const int32 uvctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, ruRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ruRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, rvRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rvRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, uvctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, uvctlRegDisp); // passed directly into GetBilinearAddress
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, (uint32)&vars->mpe->ba_control); // needed by _StorePixelAbsolute
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(srcRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);

  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->uvbase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx,x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_edx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_StorePixelAbsolute,0);
}

void Emit_StorePixelZBilinearUV(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg ruRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RU);
  const x86BaseReg rvRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RV);
  const x86BaseReg uvctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_UVCTL);
  //const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  //const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 ruRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
  const int32 rvRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
  const int32 uvctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, ruRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ruRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, rvRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rvRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, uvctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, uvctlRegDisp); // passed directly into GetBilinearAddress
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, (uint32)&vars->mpe->ba_control); // needed by _StorePixelZAbsolute
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(srcRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);

  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->uvbase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx,x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_edx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_StorePixelZAbsolute,0);
}

void Emit_StorePixelBilinearXY(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg rxRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RX);
  const x86BaseReg ryRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RY);
  const x86BaseReg xyctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_XYCTL);
  //const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  //const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 rxRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
  const int32 ryRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
  const int32 xyctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, rxRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rxRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, ryRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ryRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, xyctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, xyctlRegDisp); // passed directly into GetBilinearAddress
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, (uint32)&vars->mpe->ba_control); // needed by _StorePixelAbsolute
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(srcRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);

  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->xybase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx,x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);

  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_edx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_StorePixelAbsolute,0);
}

void Emit_StorePixelZBilinearXY(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg rxRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RX);
  const x86BaseReg ryRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RY);
  const x86BaseReg xyctlRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_XYCTL);
  //const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  //const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 rxRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
  const int32 ryRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
  const int32 xyctlRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, rxRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rxRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, ryRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ryRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, xyctlRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, xyctlRegDisp); // passed directly into GetBilinearAddress
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF0000u, x86Reg::x86Reg_ecx); // pack upper 16bits into ECX
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,16);
  vars->mpe->nativeCodeCache.X86Emit_ORRR(x86Reg::x86Reg_ecx, x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, (uint32)&vars->mpe->ba_control); // needed by _StorePixelZAbsolute
  vars->mpe->nativeCodeCache.X86Emit_MOVIM(srcRegIndex, x86MemPtr::x86MemPtr_dword, (uint32)&vars->mpe->ba_reg_offset);

  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)GetBilinearAddress,0);
  // returns result in eax
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFFFFFFu, x86Reg::x86Reg_eax); // clear the passed back 4bit pixel addressing bit in MSB
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&vars->mpe->xybase);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCu, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_edx);

  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(vars->mpe->dtrom), x86Reg::x86Reg_ebx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.mainBusDRAM), x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.systemBusDRAM), x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_MOVIR((uint32)(nuonEnv.flashEEPROM.GetBasePointer()), x86Reg::x86Reg_ebp);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(MAIN_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ecx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(SYSTEM_BUS_BASE, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_edx);
  //vars->mpe->nativeCodeCache.X86Emit_CMPIR(0xF0000000, x86Reg::x86Reg_eax);
  //vars->mpe->nativeCodeCache.X86Emit_CMOVNBRR(x86Reg::x86Reg_ebx, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebx,x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_SHRIR(x86Reg::x86Reg_ebx,26);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x3C,x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, (int32)vars->mpe->bankPtrTable);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x007FFFFF, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_ADDRR(x86Reg::x86Reg_eax, x86Reg::x86Reg_ebx);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, (uint32)&vars->mpe);
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_edx, x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_CALLI((uint32)_StorePixelZAbsolute,0);
}

void Emit_PushVector(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg_0 = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg srcRegReadBaseReg_1 = GetScalarRegReadBaseReg(vars,srcRegIndex+1);
  const x86BaseReg srcRegReadBaseReg_2 = GetScalarRegReadBaseReg(vars,srcRegIndex+2);
  const x86BaseReg srcRegReadBaseReg_3 = GetScalarRegReadBaseReg(vars,srcRegIndex+3);

  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&(vars->mpe->sp));
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg_0, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, srcRegReadBaseReg_1, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_2, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_SUBIR(16, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, (uint32)&(vars->mpe->sp));
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(MPE_VALID_MEMORY_MASK, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDIR((uint32)vars->mpe->dtrom, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg_3, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+12);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 12);
}

void Emit_PushVectorRz(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg_0 = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg srcRegReadBaseReg_1 = GetScalarRegReadBaseReg(vars,srcRegIndex+1);
  const x86BaseReg srcRegReadBaseReg_2 = GetScalarRegReadBaseReg(vars,srcRegIndex+2);

  const x86BaseReg rzRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RZ);

  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 rzRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RZ);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&(vars->mpe->sp));
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg_0, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, srcRegReadBaseReg_1, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, srcRegReadBaseReg_2, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_SUBIR(16, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, (uint32)&(vars->mpe->sp));
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(MPE_VALID_MEMORY_MASK, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDIR((uint32)vars->mpe->dtrom, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, rzRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rzRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 12);
}

void Emit_PushScalarRzi1(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg ccRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_CC);
  const x86BaseReg rzi1RegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RZI1);
  const x86BaseReg rzRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RZ);

  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 rzi1RegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RZI1);
  const int32 rzRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RZ);
  const int32 ccRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_CC);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&(vars->mpe->sp));
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, ccRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ccRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, rzi1RegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rzi1RegDisp);
  vars->mpe->nativeCodeCache.X86Emit_SUBIR(16, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, (uint32)&(vars->mpe->sp));
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(MPE_VALID_MEMORY_MASK, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDIR((uint32)vars->mpe->dtrom, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, rzRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rzRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 12);
}

void Emit_PushScalarRzi2(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 srcRegIndex = nuance.fields[FIELD_MEM_FROM];
  const x86BaseReg srcRegReadBaseReg = GetScalarRegReadBaseReg(vars,srcRegIndex);
  const x86BaseReg ccRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_CC);
  const x86BaseReg rzi2RegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RZI2);
  const x86BaseReg rzRegReadBaseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RZ);

  const int32 srcRegDisp = GetScalarRegEmitDisp(vars,srcRegIndex);
  const int32 rzi2RegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RZI2);
  const int32 rzRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RZ);
  const int32 ccRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_CC);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&(vars->mpe->sp));
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, srcRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, srcRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, ccRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ccRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, rzi2RegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rzi2RegDisp);
  vars->mpe->nativeCodeCache.X86Emit_SUBIR(16, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, (uint32)&(vars->mpe->sp));
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(MPE_VALID_MEMORY_MASK, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDIR((uint32)vars->mpe->dtrom, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, rzRegReadBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rzRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 12);
}

void Emit_PopVector(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&(vars->mpe->sp));
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebp, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDIR(16, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, (uint32)&(vars->mpe->sp));
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(MPE_VALID_MEMORY_MASK, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_ADDIR((uint32)vars->mpe->dtrom, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_ebp, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_ebp, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_ebp, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 8);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, x86BaseReg::x86BaseReg_ebp, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 12);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_edx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+12);
}

void Emit_PopVectorRz(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const x86BaseReg rzRegWriteBaseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RZ);

  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 rzRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RZ);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&(vars->mpe->sp));
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebp, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(MPE_VALID_MEMORY_MASK, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDIR((uint32)vars->mpe->dtrom, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 12);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+4);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp+8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, rzRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rzRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ADDIR(16, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, (uint32)&(vars->mpe->sp));
}

void Emit_PopScalarRzi1(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const x86BaseReg rzRegWriteBaseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RZ);
  const x86BaseReg rzi1RegWriteBaseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RZI1);
  const x86BaseReg ccRegWriteBaseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_CC);

  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 rzRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RZ);
  const int32 rzi1RegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RZI1);
  const int32 ccRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_CC);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&(vars->mpe->sp));
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebp, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(MPE_VALID_MEMORY_MASK, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDIR((uint32)vars->mpe->dtrom, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 12);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, ccRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ccRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, rzi1RegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rzi1RegDisp);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, rzRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rzRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ADDIR(16, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, (uint32)&(vars->mpe->sp));
}

void Emit_PopScalarRzi2(EmitterVariables * const vars, const Nuance &nuance)
{
  const uint32 destRegIndex = nuance.fields[FIELD_MEM_TO];
  const x86BaseReg destRegWriteBaseReg = GetScalarRegWriteBaseReg(vars,destRegIndex);
  const x86BaseReg rzRegWriteBaseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RZ);
  const x86BaseReg rzi2RegWriteBaseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RZI2);
  const x86BaseReg ccRegWriteBaseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_CC);

  const int32 destRegDisp = GetScalarRegEmitDisp(vars,destRegIndex);
  const int32 rzRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RZ);
  const int32 rzi2RegDisp = GetMiscRegEmitDisp(vars,REGINDEX_RZI2);
  const int32 ccRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_CC);

  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_edx, (uint32)&(vars->mpe->sp));
  vars->mpe->nativeCodeCache.X86Emit_MOVRR(x86Reg::x86Reg_ebp, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ANDIR(MPE_VALID_MEMORY_MASK, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_ADDIR((uint32)vars->mpe->dtrom, x86Reg::x86Reg_edx);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 0);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ebx, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 4);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_ecx, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 8);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ebx);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_ecx);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, destRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, destRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVMR(x86Reg::x86Reg_eax, x86BaseReg::x86BaseReg_edx, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, 12);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebx, ccRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ccRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ecx, rzi2RegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rzi2RegDisp);
  vars->mpe->nativeCodeCache.X86Emit_BSWAP(x86Reg::x86Reg_eax);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_eax, rzRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, rzRegDisp);
  vars->mpe->nativeCodeCache.X86Emit_ADDIR(16, x86Reg::x86Reg_ebp);
  vars->mpe->nativeCodeCache.X86Emit_MOVRM(x86Reg::x86Reg_ebp, (uint32)&(vars->mpe->sp));
}

void EmitControlRegisterLoad(EmitterVariables * const vars, const uint32 address, const x86Reg destReg)
{
  switch((address & 0x1FF0) >> 4)
  {
    case (0x00 >> 4):
      //mpectl
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, ((uint32)&(vars->mpe->mpectl)));
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(~(0xF0FF5555UL), destReg);
      break;
    case (0x10 >> 4):
      //excepsrc
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, ((uint32)&(vars->mpe->excepsrc)));
      break;
    case (0x20 >> 4):
      //excepclr: always reads as zero
      vars->mpe->nativeCodeCache.X86Emit_XORRR(destReg, destReg);
      break;
    case (0x30 >> 4):
      //excephalten
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, ((uint32)&(vars->mpe->excephalten)));
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x1FFFUL, destReg);
      break;
    case (0x40 >> 4):
      //cc
      {
      const x86BaseReg baseReg = GetMiscRegReadBaseReg(vars,REGINDEX_CC);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_CC);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFUL, destReg);
      }
      break;
    case (0x80 >> 4):
      //rz
      {
      const x86BaseReg baseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RZ);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RZ);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x90 >> 4):
      //rzi1
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RZI1);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RZI1);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0xA0 >> 4):
      //rzi2
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RZI2);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RZI2);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0xB0 >> 4):
      //intvec1
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, ((uint32)&(vars->mpe->intvec1)));
      break;
    case (0xC0 >> 4):
      //intvec2
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, ((uint32)&(vars->mpe->intvec2)));
      break;
    case (0xD0 >> 4):
      //intsrc
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, ((uint32)&(vars->mpe->intsrc)));
      break;
    case (0xE0 >> 4):
      //intclr: always reads as zero
      vars->mpe->nativeCodeCache.X86Emit_XORRR(destReg, destReg);
      break;
    case (0xF0 >> 4):
      //intctl
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, ((uint32)&(vars->mpe->intctl)));
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xAA, destReg);
      break;
    case (0x100 >> 4):
      //inten1
    case (0x110 >> 4):
      //inten1set: always reads the same as inten1
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, ((uint32)&(vars->mpe->inten1)));
      break;
    case (0x120 >> 4):
      //inten1clr: always reads as zero
      vars->mpe->nativeCodeCache.X86Emit_XORRR(destReg, destReg);
      break;
    case (0x130 >> 4):
      //inten2sel
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, ((uint32)&(vars->mpe->inten2sel)));
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x1F, destReg);
      break;
    case (0x1E0 >> 4):
      //rc0
      {
      const x86BaseReg baseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RC0);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RC0);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF, destReg);
      }
      break;
    case (0x1F0 >> 4):
      //rc1
      {
      const x86BaseReg baseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RC1);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RC1);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF, destReg);
      }
      break;
    case (0x200 >> 4):
      //rx
      {
      const x86BaseReg baseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RX);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x210 >> 4):
      //ry
      {
      const x86BaseReg baseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RY);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x220 >> 4):
      //xyrange
      {
      const x86BaseReg baseReg = GetMiscRegReadBaseReg(vars,REGINDEX_XYRANGE);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_XYRANGE);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(((0x3FFUL << 16) | 0x3FFUL), destReg);
      }
      break;
    case (0x230 >> 4):
      //xybase
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, ((uint32)&(vars->mpe->xybase)));
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCUL, destReg);
      break;
    case (0x240 >> 4):
      //xyctl
      {
      const x86BaseReg baseReg = GetMiscRegReadBaseReg(vars,REGINDEX_XYCTL);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(~((1UL << 11) | (1UL << 27) | (1UL << 31)), destReg);
      }
      break;
    case (0x250 >> 4):
      //ru
      {
      const x86BaseReg baseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RU);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x260 >> 4):
      //rv
      {
      const x86BaseReg baseReg = GetMiscRegReadBaseReg(vars,REGINDEX_RV);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x270 >> 4):
      //uvrange
      {
      const x86BaseReg baseReg = GetMiscRegReadBaseReg(vars,REGINDEX_UVRANGE);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_UVRANGE);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(((0x3FFUL << 16) | 0x3FFUL), destReg);
      }
      break;
    case (0x280 >> 4):
      //uvbase
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, ((uint32)&(vars->mpe->uvbase)));
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCUL, destReg);
      break;
    case (0x290 >> 4):
      //uvctl
      {
      const x86BaseReg baseReg = GetMiscRegReadBaseReg(vars,REGINDEX_UVCTL);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(~((1UL << 11) | (1UL << 27) | (1UL << 31)), destReg);
      }
      break;
    case (0x2A0 >> 4):
      //linpixctl
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, ((uint32)&(vars->mpe->linpixctl)));
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(((1UL << 28) | (15UL << 20)), destReg);
      break;
    case (0x2B0 >> 4):
      //clutbase
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, ((uint32)&(vars->mpe->clutbase)));
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFC0UL, destReg);
      break;
    case (0x2C0 >> 4):
      //svshift
      {
      const x86BaseReg baseReg = GetMiscRegReadBaseReg(vars,REGINDEX_SVSHIFT);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_SVSHIFT);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x03UL, destReg);
      }
      break;
    case (0x2D0 >> 4):
      //acshift
      {
      const x86BaseReg baseReg = GetMiscRegReadBaseReg(vars,REGINDEX_ACSHIFT);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_ACSHIFT);
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_SHLIR(destReg, 25);
      vars->mpe->nativeCodeCache.X86Emit_SARIR(destReg, 25);
      }
      break;
    case (0x2E0 >> 4):
      //sp
      vars->mpe->nativeCodeCache.X86Emit_MOVMR(destReg, ((uint32)&(vars->mpe->sp)));
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFF0UL, destReg);
      break;
    case (0x500 >> 4):
      //odmactl
    case (0x510 >> 4):
      //odmacptr
    case (0x600 >> 4):
      //mdmactl
    case (0x610 >> 4):
      //mdmacptr
    case (0x7E0 >> 4):
      //comminfo
    case (0x7F0 >> 4):
      //commctl
    case (0x800 >> 4):
      //commxmit0 to commxmit3
    case (0x810 >> 4):
      //commrecv0 to commrecv3
    default:
      assert(!"unhandled control register load");
      break;
  }
}

void EmitControlRegisterStore(EmitterVariables * const vars, const uint32 address, const x86Reg srcReg)
{
  const x86BaseReg ccRegWriteBaseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_CC);
  const int32 ccRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_CC);

  switch((address & 0x1FF0) >> 4)
  {
    //case (0x00 >> 4):
      //mpectl
    //  break;
    //case (0x10 >> 4):
      //excepsrc
    //  break;
    case (0x20 >> 4):
      //excepclr: clear corresponding bits in excepsrc
      vars->mpe->nativeCodeCache.X86Emit_NOTR(srcReg);
      vars->mpe->nativeCodeCache.X86Emit_ANDRM(srcReg,((uint32)&(vars->mpe->excepsrc)));
      break;
    //case (0x30 >> 4):
      //excephalten
    //  break;
    case (0x40 >> 4):
      //cc
      {
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x7FFUL, srcReg);
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_CC);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_CC);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x80 >> 4):
      //rz
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RZ);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RZ);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x90 >> 4):
      //rzi1
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RZI1);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RZI1);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0xA0 >> 4):
      //rzi2
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RZI2);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RZI2);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0xB0 >> 4):
      //intvec1
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFEUL, srcReg);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, ((uint32)&(vars->mpe->intvec1)));
      break;
    case (0xC0 >> 4):
      //intvec2
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFEUL, srcReg);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, ((uint32)&(vars->mpe->intvec2)));
      break;
    //case (0xD0 >> 4):
      //intsrc
    //  break;
    case (0xE0 >> 4):
      //intclr: clears corresponding bit in intsrc
      vars->mpe->nativeCodeCache.X86Emit_NOTR(srcReg);
      vars->mpe->nativeCodeCache.X86Emit_ANDRM(srcReg, ((uint32)&(vars->mpe->intsrc)));
      break;
    //case (0xF0 >> 4):
      //intctl
    //  break;
    //case (0x100 >> 4):
      //inten1
    //  break;
    //case (0x110 >> 4):
      //inten1set
    //  break;
    //case (0x130 >> 4):
      //inten2sel
    //  break;
    case (0x1E0 >> 4):
      //rc0: lower 16 bits only
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RC0);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RC0);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_ANDIM(~CC_COUNTER0_ZERO, x86MemPtr::x86MemPtr_dword, ccRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ccRegDisp);
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF, srcReg);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_MOVIR(0, srcReg);
      vars->mpe->nativeCodeCache.X86Emit_MOVIR(CC_COUNTER0_ZERO, x86Reg::x86Reg_ebp);
      vars->mpe->nativeCodeCache.X86Emit_CMOVZRR(srcReg, x86Reg::x86Reg_ebp);
      vars->mpe->nativeCodeCache.X86Emit_ORRM(srcReg, ccRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ccRegDisp);
      }
      break;
    case (0x1F0 >> 4):
      //rc1: lower 16 bits only
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RC1);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RC1);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_ANDIM(~CC_COUNTER1_ZERO, x86MemPtr::x86MemPtr_dword, ccRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ccRegDisp);
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFF, srcReg);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_MOVIR(0, srcReg);
      vars->mpe->nativeCodeCache.X86Emit_MOVIR(CC_COUNTER1_ZERO, x86Reg::x86Reg_ebp);
      vars->mpe->nativeCodeCache.X86Emit_CMOVZRR(srcReg, x86Reg::x86Reg_ebp);
      vars->mpe->nativeCodeCache.X86Emit_ORRM(srcReg, ccRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ccRegDisp);
      }
      break;
    case (0x200 >> 4):
      //rx
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RX);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x210 >> 4):
      //ry
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RY);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x220 >> 4):
      //xyrange
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_XYRANGE);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_XYRANGE);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x230 >> 4):
      //xybase: always on scalar boundary
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFCUL, srcReg);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, ((uint32)&(vars->mpe->xybase)));
      break;
    case (0x240 >> 4):
      //xyctl
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_XYCTL);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(~((1UL << 31) | (1UL << 27) | (1UL << 11)), srcReg);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x250 >> 4):
      //ru
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RU);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x260 >> 4):
      //rv
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RV);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x270 >> 4):
      //uvrange
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_UVRANGE);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_UVRANGE);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x280 >> 4):
      //uvbase
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFFC, srcReg);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, ((uint32)&(vars->mpe->uvbase)));
      break;
    case (0x290 >> 4):
      //uvctl
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_UVCTL);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(~((1UL << 31) | (1UL << 27) | (1UL << 11)), srcReg);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x2A0 >> 4):
      //linpixctl
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(((0x01UL << 28) | (0xFUL << 20)), srcReg);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, ((uint32)&(vars->mpe->linpixctl)));
      break;
    case (0x2B0 >> 4):
      //clutbase
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFC0, srcReg);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, ((uint32)&(vars->mpe->clutbase)));
      break;
    case (0x2C0 >> 4):
      // svshift
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_SVSHIFT);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_SVSHIFT);
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0x03, srcReg);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x2D0 >> 4):
      //acshift
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_ACSHIFT);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_ACSHIFT);
      vars->mpe->nativeCodeCache.X86Emit_SHLIR(srcReg, 25);
      vars->mpe->nativeCodeCache.X86Emit_SARIR(srcReg, 25);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x2E0 >> 4):
      //sp
      vars->mpe->nativeCodeCache.X86Emit_ANDIR(0xFFFFFFF0, srcReg);
      vars->mpe->nativeCodeCache.X86Emit_MOVRM(srcReg, ((uint32)&(vars->mpe->sp)));
      break;
    case (0x500 >> 4):
      //odmactl
    case (0x510 >> 4):
      //odmacptr
    case (0x600 >> 4):
      //mdmactl
    case (0x610 >> 4):
      //mdmacptr
    case (0x7E0 >> 4):
      //comminfo
    case (0x7F0 >> 4):
      //commctl
    case (0x800 >> 4):
      //commxmit0 to commxmit3
    case (0x810 >> 4):
      //commrecv0 to commrecv3
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(1, x86MemPtr::x86MemPtr_dword, (uint32)&(vars->mpe->interpretNextPacket));
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(vars->pInstructionEntry->packet->pcexec, x86MemPtr::x86MemPtr_dword, (uint32)&(vars->mpe->pcexec));
      Emit_ExitBlock(vars);
      break;
    default:
      assert(!"unhandled control register store");
      break;
  }
}

void EmitControlRegisterStoreImmediate(EmitterVariables * const vars, const uint32 address, const uint32 imm)
{
  const x86BaseReg ccRegWriteBaseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_CC);
  const int32 ccRegDisp = GetMiscRegEmitDisp(vars,REGINDEX_CC);

  switch((address & 0x1FF0) >> 4)
  {
    //case (0x00 >> 4):
      //mpectl
    //  break;
    //case (0x10 >> 4):
      //excepsrc
    //  break;
    case (0x20 >> 4):
      vars->mpe->nativeCodeCache.X86Emit_ANDIM(~imm, x86MemPtr::x86MemPtr_dword, ((uint32)&(vars->mpe->excepsrc)));
      break;
    //case (0x30 >> 4):
      //excephalten
    //  break;
    case (0x40 >> 4):
      //cc
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_CC);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_CC);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm & 0x7FFUL, x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x80 >> 4):
      //rz
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RZ);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RZ);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm, x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x90 >> 4):
      //rzi1
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RZI1);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RZI1);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm, x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0xA0 >> 4):
      //rzi2
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RZI2);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RZI2);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm, x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0xB0 >> 4):
      //intvec1
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm & 0xFFFFFFFEUL, x86MemPtr::x86MemPtr_dword, ((uint32)&(vars->mpe->intvec1)));
      break;
    case (0xC0 >> 4):
      //intvec2
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm & 0xFFFFFFFEUL, x86MemPtr::x86MemPtr_dword, ((uint32)&(vars->mpe->intvec2)));
      break;
    //case (0xD0 >> 4):
      //intsrc
    //  break;
    case (0xE0 >> 4):
      //intclr
      vars->mpe->nativeCodeCache.X86Emit_ANDIM(~imm, x86MemPtr::x86MemPtr_dword, ((uint32)&(vars->mpe->intsrc)));
      break;
    //case (0xF0 >> 4):
      //intctl
    //  break;
    //case (0x100 >> 4):
      //inten1
    //  break;
    //case (0x110 >> 4):
      //inten1set
    //  break;
    //case (0x130 >> 4):
      //inten2sel
    //  break;
    case (0x1E0 >> 4):
      //rc0
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RC0);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RC0);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm & 0xFFFF, x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_ORIM(CC_COUNTER0_ZERO, x86MemPtr::x86MemPtr_dword, ccRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ccRegDisp);
      if(imm & 0xFFFF)
        vars->mpe->nativeCodeCache.X86Emit_ANDIM(~CC_COUNTER0_ZERO, x86MemPtr::x86MemPtr_dword, ccRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ccRegDisp);
      }
      break;
    case (0x1F0 >> 4):
      //rc1
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RC1);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RC1);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm & 0xFFFF, x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      vars->mpe->nativeCodeCache.X86Emit_ORIM(CC_COUNTER1_ZERO, x86MemPtr::x86MemPtr_dword, ccRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ccRegDisp);
      if(imm & 0xFFFF)
        vars->mpe->nativeCodeCache.X86Emit_ANDIM(~CC_COUNTER1_ZERO, x86MemPtr::x86MemPtr_dword, ccRegWriteBaseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, ccRegDisp);
      }
      break;
    case (0x200 >> 4):
      //rx
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RX);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RX);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm, x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x210 >> 4):
      //ry
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RY);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RY);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm, x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x220 >> 4):
      //xyrange
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_XYRANGE);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_XYRANGE);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm, x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x230 >> 4):
      //xybase
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm & 0xFFFFFFFC, x86MemPtr::x86MemPtr_dword, ((uint32)&(vars->mpe->xybase)));
      break;
    case (0x240 >> 4):
      //xyctl
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_XYCTL);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_XYCTL);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm & (~((1UL << 31) | (1UL << 27) | (1UL << 11))), x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x250 >> 4):
      //ru
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RU);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RU);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm, x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x260 >> 4):
      //rv
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_RV);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_RV);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm, x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x270 >> 4):
      //uvrange
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_UVRANGE);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_UVRANGE);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm, x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x280 >> 4):
      //uvbase
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm & 0xFFFFFFFC, x86MemPtr::x86MemPtr_dword, ((uint32)&(vars->mpe->uvbase)));
      break;
    case (0x290 >> 4):
      //uvctl
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_UVCTL);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_UVCTL);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm & (~((1UL << 31) | (1UL << 27) | (1UL << 11))), x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x2A0 >> 4):
      //linpixctl
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm & ((0x01UL << 28) | (0xFUL << 20)), x86MemPtr::x86MemPtr_dword, ((uint32)&(vars->mpe->linpixctl)));
      break;
    case (0x2B0 >> 4):
      //clutbase
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm & 0xFFFFFFC0, x86MemPtr::x86MemPtr_dword, ((uint32)&(vars->mpe->clutbase)));
      break;
    case (0x2C0 >> 4):
      //svshift
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_SVSHIFT);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_SVSHIFT);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm & 0x03, x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x2D0 >> 4):
      //acshift
      {
      const x86BaseReg baseReg = GetMiscRegWriteBaseReg(vars,REGINDEX_ACSHIFT);
      const int32 disp = GetMiscRegEmitDisp(vars,REGINDEX_ACSHIFT);
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(((int32)(imm << 25)) >> 25, x86MemPtr::x86MemPtr_dword, baseReg, x86IndexReg::x86IndexReg_none, x86ScaleVal::x86Scale_1, disp);
      }
      break;
    case (0x2E0 >> 4):
      //sp
      vars->mpe->nativeCodeCache.X86Emit_MOVIM(imm & 0xFFFFFFF0, x86MemPtr::x86MemPtr_dword, ((uint32)&(vars->mpe->sp)));
      break;
    case (0x500 >> 4):
      //odmactl
    case (0x510 >> 4):
      //odmacptr
    case (0x600 >> 4):
      //mdmactl
    case (0x610 >> 4):
      //mdmacptr
    case (0x7E0 >> 4):
      //comminfo
    case (0x7F0 >> 4):
      //commctl
    case (0x800 >> 4):
      //commxmit0 to commxmit3
    case (0x810 >> 4):
      //commrecv0 to commrecv3
    default:
      assert(!"unhandled control register store immediate");
      break;
  }
}
