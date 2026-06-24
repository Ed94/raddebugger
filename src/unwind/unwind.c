// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Generated Code

#include "unwind/generated/unwind.meta.c"

////////////////////////////////
//~ rjf: Moduleless Unwinder

internal UWND_StepResult
uwnd_step_moduleless(Arch arch, MemoryMap *memory_map, void *regs)
{
  UWND_StepResult result = {UWND_StepStatus_Error};
  {
    B32 done = 0;
    ARCH_Info *arch_info = arch_info_from_arch(arch);
    U64 addr_size = byte_size_from_arch(arch);
    
    // rjf: read new ip from stack pointer
    U64 sp = arch_sp_from_reg_block(arch_info, regs);
    U64 new_ip = 0;
    Rng1U64 new_ip_vaddr_range = r1u64(sp, sp+addr_size);
    if(memory_map_read(memory_map, new_ip_vaddr_range, &new_ip) != addr_size)
    {
      done = 1;
      result.status = UWND_StepStatus_FailedMemoryRead;
      result.missed_read_vaddr_range = new_ip_vaddr_range;
    }
    
    // rjf: commit
    if(!done)
    {
      U64 new_sp = sp+addr_size;
      arch_reg_block_write_ip(arch_info, regs, new_ip);
      arch_reg_block_write_sp(arch_info, regs, new_sp);
      result.status = UWND_StepStatus_Good;
    }
  }
  return result;
}

////////////////////////////////
//~ rjf: Abstracted Unwind Step Functions

internal UWND_StepResult
uwnd_step(UWND_Unwinder unwinder, Arch arch, MemoryMap *memory_map, UWND_ModuleInfo *module_info, U64 tls_vaddr, void *regs, U64 *cfa_out)
{
  UWND_StepResult result = {UWND_StepStatus_Error};
  switch(unwinder)
  {
    default:{result = uwnd_step_moduleless(arch, memory_map, regs);}break;
#define X(name, namespace_lower) case UWND_Unwinder_##name:{result = namespace_lower##_uwnd_step(arch, memory_map, module_info, tls_vaddr, regs, cfa_out);}break;
    UWND_Unwinder_XList
#undef X
  }
  return result;
}
