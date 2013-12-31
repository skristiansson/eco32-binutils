#include "sysdep.h"
#include <stdio.h>
#define STATIC_TABLE
#define DEFINE_TABLE

#include "opcode/eco32.h"
#include "dis-asm.h"

#define Word unsigned int
#define SEXT26(x)       (((Word) (x)) & SIGN(26) ? \
                         (((Word) (x)) | ~MASK(26)) : \
                          (((Word) (x)) & MASK(26)))
#define SIGN(n)         (((Word) 1) << (n - 1))
#define MASK(n)         ((((Word) 1) << n) - 1)

static fprintf_ftype fpr;
static void *stream;

extern const eco32_opc_info_t eco32_opc_info[ECO32_INSN_COUNT];

eco32_opc_info_t eco32_opc_lookup(unsigned int);

eco32_opc_info_t eco32_opc_lookup(unsigned int opc)
{
	
	int cnt=0;
	eco32_opc_info_t ret = eco32_opc_info[0] ; //initialize OP_BAD
	for(cnt = 0; cnt < ECO32_INSN_COUNT; cnt++)
	{
		if(eco32_opc_info[cnt].opcode == opc)
		{
			ret=eco32_opc_info[cnt];
			break;
		}
	}
	return ret;
}

/* Disassemble one instruction at the given target address.
 *   Return number of octets processed.  */
int
print_insn_eco32 (bfd_vma addr, struct disassemble_info *info)
{
 int status;
 stream = info->stream;
 bfd_byte buffer[ECO32_MAX_INSN_SIZE];
 fpr = info->fprintf_func;
 eco32_opc_info_t eco32_instr;
 unsigned int iword = 0;
 unsigned int opc = 0;
 unsigned int dest = 0;
 unsigned int src1 = 0;
 unsigned int src2 = 0;
 unsigned int immediate = 0;
 
 if ((status = info->read_memory_func (addr, buffer, eco32_insn_byte, info)))
 goto fail;
 
 iword |= bfd_get_bits (buffer, ECO32_MAX_INSN_SIZE, TRUE);
 opc |= iword>>opc_offset;
 
 eco32_instr = eco32_opc_lookup(opc);
 iword ^= (opc<<opc_offset);
 
 if(eco32_instr.opcode == OP_BAD)
 {
	printf("unknown instruction");
	return -1;
 }
 else
 {
	switch(eco32_instr.itype)
	{
		case ECO32_N:
			fpr (stream, "%s", eco32_instr.name);
		break;
		case ECO32_J:
		{
			immediate = iword;
			fpr (stream, "%s 0x%08x", eco32_instr.name,((bfd_vma) (addr+4)+(SEXT26(immediate)<<2)));
			break;
		}
		case ECO32_R:
			src1 = iword>>R_reg;
			fpr (stream, "%s $%d", eco32_instr.name,(short)src1);
		break;
		case ECO32_RRR:
			src1=iword>>RRR_src1_reg;
			iword ^= (src1<<RRR_src1_reg);
			
			src2=iword>>RRR_src2_reg;
			iword ^= (src2<<RRR_src2_reg);
			
			dest=iword>>RRR_dest_reg;
			fpr (stream, "%s $%d,$%d,$%d", eco32_instr.name,dest,src1,src2);
		break;
		case ECO32_RRB:
			src1=iword>>RRB_src1;
			iword ^= (src1<<RRB_src1);
			
			src2=iword>>RRB_src2;
			iword ^= (dest<<RRB_src2);
			/* 
			 * rest of iword is now immediate
			 */
			immediate = (short)iword;
			fpr (stream, "%s $%d,$%d,0x%08x", eco32_instr.name,src1,src2,((bfd_vma) (addr+4)+(SEXT26(immediate)<<2)));
			break;
		case ECO32_RRH:
		case ECO32_RRS:
		case ECO32_RRS_:
			src1=iword>>RRS_src_reg;
			iword ^= (src1<<RRS_src_reg);
			
			dest=iword>>RRS_dest_reg;
			iword ^= (dest<<RRS_dest_reg);
			/* 
			 * rest of iword is now immediate
			 */
			immediate = iword;
			fpr (stream, "%s $%d,$%d,0x%04x", eco32_instr.name,dest,src1,immediate);
		break;
		case ECO32_RH:
			dest=iword>>RHh_dest_reg;
			iword ^= (dest<<RHh_dest_reg);
			
			fpr (stream, "%s $%d,0x%08x", eco32_instr.name,dest,iword);
		break;
		case ECO32_RHh:
			dest=iword>>RHh_dest_reg;
			iword ^= (dest<<RHh_dest_reg);
			
			fpr (stream, "%s $%d,0x%08x", eco32_instr.name,dest,iword<<16);
		break;
		default:
		fpr (stream, "%s ???", eco32_instr.name);
	break;
	}
	return eco32_insn_byte;
 }

 fail:
 info->memory_error_func (status, addr, info);
 return -1;
} 
