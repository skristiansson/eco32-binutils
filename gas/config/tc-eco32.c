#include "as.h"
#include "safe-ctype.h"
#include "opcode/eco32.h"

extern const eco32_opc_info_t eco32_opc_info[ECO32_INSN_COUNT];

const char comment_chars[] = "#";
const char line_separator_chars[] = ";";
const char line_comment_chars[] = "#";

static int pending_reloc;
static struct hash_control *opcode_hash_control;

const pseudo_typeS md_pseudo_table[] =
{
	{ "byte",    cons,                1 },
	{ "hword",   cons,                2 },
	{ "word",    cons,                4 },
	{(char *)0 , (void(*)(int))0,     0}
};

const char FLT_CHARS[] = "rRsSfFdDxXpP";
const char EXP_CHARS[] = "eE";

void
md_operand (expressionS *op __attribute__((unused)))
{
 /* Empty for now. */
}

/* This function is called once, at assembler startup time. It sets
 up the hash table with all the opcodes in it, and also initializes
 some aliases for compatibility with other assemblers. */

void
md_begin (void)
{
 const eco32_opc_info_t *opcode;
 opcode_hash_control = hash_new ();

 /* Insert names into hash table. */
 for (opcode = eco32_opc_info; opcode->name; opcode++)
 hash_insert (opcode_hash_control, opcode->name, (char *) opcode);

 bfd_set_arch_mach (stdoutput, TARGET_ARCH, 0);
}

/* Parse an expression and then restore the input line pointer. */

static char *
parse_exp_save_ilp (char *s, expressionS *op)
{
 char *save = input_line_pointer;
 input_line_pointer = s;
 expression (op);
 s = input_line_pointer;
 input_line_pointer = save;
 return s;
}

static int
parse_register_operand(char **op)
{
	char *op_end = *op;
	int regno = 0;
	if(*op_end != '$')
	{
		as_bad(_("expecting register($) got: >%s< "),op_end);
		return -1;
	}
	op_end++; //skip $
	regno = REGNO(op_end[0]);
	op_end++; //skip number
	if(*op_end != 0 && *op_end != ',' && *op_end != '$' )
	{
	regno = (regno*10)+REGNO(op_end[0]);
	op_end++; //skip number
	}
	if(regno < 0 || regno > 31)
	{
		as_bad(_("wrong register number %d\n"),regno);
		return -1;
	}
	*op+=(regno > 9 ? 3 : 2);
	return regno;
}


/* This is the guts of the machine-dependent assembler. STR points to
 a machine dependent instruction. This function is supposed to emit
 the frags/bytes it assembles to. */

void
md_assemble (char *str)
{
 char *op_start;
 char *op_end;

 eco32_opc_info_t *opcode;
 unsigned int iword = 0;
 char *p;
 int regno = 0;
 int immediate = 0;
 char pend;

 int nlen = 0;
 /* Drop leading whitespace. */
 while (*str == ' ')
 str++;

 /* Find the op code end. */
 op_start = str;
 
 for (op_end = str;
 *op_end && !is_end_of_line[*op_end & 0xff] && *op_end != ' ';
 op_end++)
 nlen++;

 pend = *op_end;
 *op_end = 0;

 if (nlen == 0)
 as_bad (_("can't find opcode "));

 opcode = (eco32_opc_info_t *) hash_find (opcode_hash_control, op_start);
 *op_end = pend;

 if (opcode == NULL)
 {
 as_bad (_("unknown opcode %s"), op_start);
 return;
 }

 while (ISSPACE (*op_end))
 op_end++;
		 /*
		  * this *could* be done in a much cooler way ~ I love the cool way
		  */
	switch(opcode->itype)
	{
		case ECO32_N: /* no operands */
		{
			p = frag_more(eco32_insn_byte); //number of bytes in insn
			iword |= (opcode->opcode << opc_offset);
			md_number_to_chars(p,iword,eco32_insn_byte);
		}
		break;
		case ECO32_J: /* no registers and a sign-extended 26 bit offset */
		{
			expressionS arg;
			/*char *where;*/
			if(*op_end == 0)
			{
				as_bad(_("expecting an operand in %s"),str);
				return;
			}
			op_end = parse_exp_save_ilp (op_end, &arg);
			
			switch (arg.X_op)
			{
				case O_constant:
				
					immediate=arg.X_add_number;
					p = frag_more(eco32_insn_byte); //number of bytes in insn
					iword |= (opcode->opcode << opc_offset);
					iword |= (((immediate -((unsigned)frag_now->fr_address+4))/4) & 0x03FFFFFF);
					md_number_to_chars(p,iword,eco32_insn_byte);
				break;
				case O_symbol:
					p = frag_more (eco32_insn_byte);
					iword=0x0;
					iword |= (opcode->opcode << opc_offset);


					fix_new_exp (frag_now,		/* Which frag?  */
							 (p-frag_now->fr_literal),			/* Where in that frag?  */
							 (eco32_insn_byte),			/* 1, 2, or 4 usually.  */
							 &arg,		/* Expression.  */
							 FALSE,			/* TRUE if PC-relative relocation.  */
							 BFD_RELOC_ECO32_METHOD_R26		/* Relocation type.  */);
					md_number_to_chars(p,iword,eco32_insn_byte);
				break;
			}
		}
		break;
		case ECO32_R: /* one register operand */
		{
			p = frag_more(eco32_insn_byte); //number of bytes in insn
			iword |= (opcode->opcode << opc_offset);
			if(*op_end == 0)
			{
				as_bad(_("expecting an operand in %s"),str);
				return;
			}
			regno = parse_register_operand(&op_end);
			if(regno == -1)
				return;
			iword |= regno << R_reg;
			md_number_to_chars(p,iword,eco32_insn_byte);
		}
		break;
		case ECO32_RH: /* one register and the lower 16 bits of a word */
		{
			expressionS arg;
			char *where;
			int dest=parse_register_operand(&op_end);
			if(dest == -1)
				return;
			while (ISSPACE (*op_end))
			op_end++;
			if(*op_end != ',')
			{
				as_bad(_("expecting ',' delimitted operand got >%s< : %s"),op_end,str);
				return;
			}
			op_end++; // skip ,
			while (ISSPACE (*op_end))
			op_end++;
			
			if(*op_end == 0)
			{
				as_bad(_("expecting an operand in %s"),str);
				return;
			}			
			op_end = parse_exp_save_ilp (op_end, &arg);
			
			switch (arg.X_op)
			{
				case O_constant:
					immediate=arg.X_add_number;
					iword=0x0;
					iword |= (opcode->opcode << opc_offset);
					iword |= dest<<RHh_dest_reg;
					iword |= immediate&lower_part;
					p=frag_more(eco32_insn_byte);
					md_number_to_chars(p,iword,eco32_insn_byte);
				break;
				case O_symbol:
					where = frag_more (eco32_insn_byte);
					iword=0x0;
					
					iword |= (opcode->opcode << opc_offset);
					iword |= dest<<RH_dest_reg;	 
					fix_new_exp (frag_now,		/* Which frag?  */
							 (where - frag_now->fr_literal),			/* Where in that frag?  */
							 (eco32_insn_byte),			/* 1, 2, or 4 usually.  */
							 &arg,		/* Expression.  */
							 0,			/* TRUE if PC-relative relocation.  */
							 BFD_RELOC_ECO32_METHOD_L16		/* Relocation type.  */);
					md_number_to_chars(where,iword,eco32_insn_byte);
				break;
			}
		}
		break;
		case ECO32_RHh: /* one register and the upper 16 bits of a word */ 
		{
			expressionS arg;
			char *where;
			int dest=parse_register_operand(&op_end);
			if(dest == -1)
				return;
			while (ISSPACE (*op_end))
			op_end++;
			if(*op_end != ',')
			{
				as_bad(_("expecting ',' delimitted operand got >%s< : %s"),op_end,str);
				return;
			}
			op_end++; // skip ,
			while (ISSPACE (*op_end))
			op_end++;
			if(*op_end == 0)
			{
				as_bad(_("expecting an operand in %s"),str);
				return;
			}			
			op_end = parse_exp_save_ilp (op_end, &arg);
			
			switch (arg.X_op)
			{
				case O_constant:
					immediate=arg.X_add_number;
					iword=0x0;
					iword |= (opcode->opcode << opc_offset);
					iword |= dest<<RHh_dest_reg;
					iword |= immediate>>16;
					p=frag_more(eco32_insn_byte);
					md_number_to_chars(p,iword,eco32_insn_byte);
				break;
				case O_symbol:
					where = frag_more (eco32_insn_byte);
					iword=0x0;
					
					iword |= (opcode->opcode << opc_offset);
					iword |= dest<<RHh_dest_reg;	 
					fix_new_exp (frag_now,		/* Which frag?  */
							 (where - frag_now->fr_literal),			/* Where in that frag?  */
							 (eco32_insn_byte),			/* 1, 2, or 4 usually.  */
							 &arg,		/* Expression.  */
							 0,			/* TRUE if PC-relative relocation.  */
							 BFD_RELOC_ECO32_METHOD_H16		/* Relocation type.  */);
					md_number_to_chars(where,iword,eco32_insn_byte);
				break;
			}
		}
		break;
		case ECO32_RRS_:
		case ECO32_RRS: /* two registers and a sign-extended halfword */
		{
			expressionS arg;
			char *where;
			int dest=0;
			int src=0;
			
			dest=parse_register_operand(&op_end);
			if(dest == -1)
				return;
			while (ISSPACE (*op_end))
			op_end++;
			if(*op_end != ',')
			{
				as_bad(_("expecting ',' delimitted operand got >%s< : %s"),op_end,str);
				return;
			}
			op_end++; // skip ,
			while (ISSPACE (*op_end))
			op_end++;						
			src=parse_register_operand(&op_end);
			if(src == -1)
				return;
			while (ISSPACE (*op_end))
			op_end++;
			if(*op_end != ',')
			{
				as_bad(_("expecting ',' delimitted operand got >%s< : %s"),op_end,str);
				return;
			}
			op_end++; // skip ,
			while (ISSPACE (*op_end))
			op_end++;
			if(*op_end == 0)
			{
				as_bad(_("expecting an operand in %s"),str);
				return;
			}			
			op_end = parse_exp_save_ilp (op_end, &arg);
			
			switch (arg.X_op)
			{
				case O_constant:
				
					immediate=arg.X_add_number;
					/* check if ldhi is needed */
					if ((immediate & 0xFFFF8000) == 0x00000000 
					 || (immediate & 0xFFFF8000) == 0xFFFF8000)
					{
						p = frag_more(eco32_insn_byte); //number of bytes in insn
						iword |= (opcode->opcode << opc_offset);
						iword |= dest << RRS_dest_reg;
						iword |= src << RRS_src_reg;
						iword |= (immediate & lower_part);
						md_number_to_chars(p,iword,eco32_insn_byte);
					}
					else
					{
						
						/* code: ldhi $1,imm; or $1,$1,imm; add $1,$1,src; op dst,$1,0 */
						iword=0x0;
						/*ldhi $1,imm*/
						iword |= OP_LDHI<<opc_offset;
						iword |= ECO32_AUX<<16;
						iword |= (immediate&higher_part)>>16;
						where=frag_more(eco32_insn_byte);
						md_number_to_chars(where,iword,eco32_insn_byte);
						
						iword=0x0;
						/*or $1,$1,imm*/
						iword |= OP_ORI<<opc_offset;
						iword |= ECO32_AUX<<RRS_dest_reg;
						iword |= ECO32_AUX<<RRS_src_reg;
						iword |= (immediate&lower_part);
						where=frag_more(eco32_insn_byte);
						md_number_to_chars(where,iword,eco32_insn_byte);
						
						switch(opcode->itype)
						{
							case ECO32_RRS:
								iword=0x0;
								/*opc-1 dest,$src,$1*/
								iword |= ((opcode->opcode)-1)<<opc_offset;
								iword |= dest << RRR_dest_reg;
								iword |= src<<RRR_src1_reg;
								iword |= ECO32_AUX << RRR_src2_reg;
								where=frag_more(eco32_insn_byte);
								md_number_to_chars(where,iword,eco32_insn_byte);
							break;
							case ECO32_RRS_:
								iword=0x0;
								/*add $1,$1,src*/
								iword |= OP_ADD<<opc_offset;
								iword |= ECO32_AUX << RRR_dest_reg;
								iword |= ECO32_AUX << RRR_src1_reg;
								iword |= src<<RRR_src2_reg;
								where=frag_more(eco32_insn_byte);
								md_number_to_chars(where,iword,eco32_insn_byte);
								
								iword=0x0;
								/*op dst,$1,0 */
								iword |= (opcode->opcode)<<opc_offset;
								iword |= ECO32_AUX << RRS_src_reg;
								iword |= dest << RRS_dest_reg;
								where=frag_more(eco32_insn_byte);
								md_number_to_chars(where,iword,eco32_insn_byte);
							break;
						}

					}
				break;
				case O_symbol:
				/* code: ldhi $1,con; or $1,$1,con; add $1,$1,src; op dst,$1,0 */
					
					where = frag_more (eco32_insn_byte);
					iword=0x0;
					/*ldhi $1,imm*/
					iword |= OP_LDHI<<opc_offset;
					iword |= ECO32_AUX<<16;
					fix_new_exp (frag_now,		/* Which frag?  */
							 (where - frag_now->fr_literal),			/* Where in that frag?  */
							 (eco32_insn_byte),			/* 1, 2, or 4 usually.  */
							 &arg,		/* Expression.  */
							 0,			/* TRUE if PC-relative relocation.  */
							 BFD_RELOC_ECO32_METHOD_H16		/* Relocation type.  */);
					md_number_to_chars(where,iword,eco32_insn_byte);
					
					where = frag_more (eco32_insn_byte);
					iword=0x0;
					/*or $1,$1,imm*/
					iword |= OP_ORI<<opc_offset;
					iword |= ECO32_AUX<<RRS_dest_reg;
					iword |= ECO32_AUX<<RRS_src_reg;	 
					fix_new_exp (frag_now,		/* Which frag?  */
							 (where - frag_now->fr_literal),			/* Where in that frag?  */
							 (eco32_insn_byte),			/* 1, 2, or 4 usually.  */
							 &arg,		/* Expression.  */
							 0,			/* TRUE if PC-relative relocation.  */
							 BFD_RELOC_ECO32_METHOD_L16		/* Relocation type.  */);
					md_number_to_chars(where,iword,eco32_insn_byte);
					
					
					switch(opcode->itype)
					{
						case ECO32_RRS:
							iword=0x0;
							/*opc-1 dest,$src,$1*/
							iword |= ((opcode->opcode)-1)<<opc_offset;
							iword |= dest << RRR_dest_reg;
							iword |= src<<RRR_src1_reg;
							iword |= ECO32_AUX << RRR_src2_reg;
							where=frag_more(eco32_insn_byte);
							md_number_to_chars(where,iword,eco32_insn_byte);
						break;
						case ECO32_RRS_:
							iword=0x0;
							/*add $1,$1,src*/
							iword |= OP_ADD<<opc_offset;
							iword |= ECO32_AUX << RRR_dest_reg;
							iword |= ECO32_AUX << RRR_src1_reg;
							iword |= src<<RRR_src2_reg;
							where=frag_more(eco32_insn_byte);
							md_number_to_chars(where,iword,eco32_insn_byte);
							
							iword=0x0;
							/*op dst,$1,0 */
							iword |= (opcode->opcode)<<opc_offset;
							iword |= ECO32_AUX << RRS_src_reg;
							iword |= dest << RRS_dest_reg;
							where=frag_more(eco32_insn_byte);
							md_number_to_chars(where,iword,eco32_insn_byte);
						break;
					}
					
				break;
			}
		}
		break;
		case ECO32_RRH:/* two registers and a zero-extended halfword */
		{
			expressionS arg;
			char *where;
			int dest=0;
			int src=0;
			
			dest=parse_register_operand(&op_end);
			if(dest == -1)
				return;
			while (ISSPACE (*op_end))
			op_end++;
			if(*op_end != ',')
			{
				as_bad(_("expecting ',' delimitted operand got >%s< : %s"),op_end,str);
				return;
			}
			op_end++; // skip ,
			while (ISSPACE (*op_end))
			op_end++;
			
			src=parse_register_operand(&op_end);
			if(src == -1)
				return;
			while (ISSPACE (*op_end))
			op_end++;
			if(*op_end != ',')
			{
				as_bad(_("expecting ',' delimitted operand got >%s< : %s"),op_end,str);
				return;
			}
			op_end++; // skip ,
			while (ISSPACE (*op_end))
			op_end++;
			if(*op_end == 0)
			{
				as_bad(_("expecting an operand in %s"),str);
				return;
			}
			op_end = parse_exp_save_ilp (op_end, &arg);
			
			switch (arg.X_op)
			{
				case O_constant:
				
					immediate=arg.X_add_number;
					/* check if ldhi is needed */
					if ((immediate & higher_part) == 0)
					{
						p = frag_more(eco32_insn_byte); //number of bytes in insn
						iword |= (opcode->opcode << opc_offset);
						iword |= dest << RRS_dest_reg;
						iword |= src << RRS_src_reg;
						iword |= (immediate & lower_part);
						md_number_to_chars(p,iword,eco32_insn_byte);
					}
					else
					{
						/* code: ldhi $1,imm; ori $1,$1,imm; opc-1 dst,src,$1 */
						iword=0x0;
						/*ldhi $1,imm*/
						iword |= OP_LDHI<<opc_offset;
						iword |= ECO32_AUX<<16;
						iword |= (immediate&higher_part)>>16;
						where=frag_more(eco32_insn_byte);
						md_number_to_chars(where,iword,eco32_insn_byte);
						
						iword=0x0;
						/*ori $1,$1,imm*/
						iword |= OP_ORI<<opc_offset;
						iword |= ECO32_AUX<<RRS_dest_reg;
						iword |= ECO32_AUX<<RRS_src_reg;
						iword |= (immediate&lower_part);
						where=frag_more(eco32_insn_byte);
						md_number_to_chars(where,iword,eco32_insn_byte);
						
						iword=0x0;
						/*opc-1 dest,$src,$1*/
						iword |= ((opcode->opcode)-1)<<opc_offset;
						iword |= dest << RRR_dest_reg;
						iword |= src<<RRR_src1_reg;
						iword |= ECO32_AUX << RRR_src2_reg;
						where=frag_more(eco32_insn_byte);
						md_number_to_chars(where,iword,eco32_insn_byte);
					}
				break;
				case O_symbol:
					
					where = frag_more (eco32_insn_byte);
					iword=0x0;
					/*ldhi $1,imm*/
					iword |= OP_LDHI<<opc_offset;
					iword |= ECO32_AUX<<16;
					fix_new_exp (frag_now,		/* Which frag?  */
							 (where - frag_now->fr_literal),			/* Where in that frag?  */
							 (eco32_insn_byte),			/* 1, 2, or 4 usually.  */
							 &arg,		/* Expression.  */
							 0,			/* TRUE if PC-relative relocation.  */
							 BFD_RELOC_ECO32_METHOD_H16		/* Relocation type.  */);
					md_number_to_chars(where,iword,eco32_insn_byte);
					
					where = frag_more (eco32_insn_byte);
					iword=0x0;
					/*or $1,$1,imm*/
					iword |= OP_ORI<<opc_offset;
					iword |= ECO32_AUX<<RRS_dest_reg;
					iword |= ECO32_AUX<<RRS_src_reg;	 
					fix_new_exp (frag_now,		/* Which frag?  */
							 (where - frag_now->fr_literal),			/* Where in that frag?  */
							 (eco32_insn_byte),			/* 1, 2, or 4 usually.  */
							 &arg,		/* Expression.  */
							 0,			/* TRUE if PC-relative relocation.  */
							 BFD_RELOC_ECO32_METHOD_L16		/* Relocation type.  */);
					md_number_to_chars(where,iword,eco32_insn_byte);

					iword=0x0;
					/*opc-1 dest,$src,$1*/
					iword |= ((opcode->opcode)-1)<<opc_offset;
					iword |= dest << RRR_dest_reg;
					iword |= src<<RRR_src1_reg;
					iword |= ECO32_AUX << RRR_src2_reg;
					where=frag_more(eco32_insn_byte);
					md_number_to_chars(where,iword,eco32_insn_byte);
				break;
			}			
		}
		break;
		case ECO32_RRB: /* two registers and a sign-extended 16 bit offset */
		{
			expressionS arg;
			/*char *where;*/
			int src1=0;
			int src2=0;
			
			src1=parse_register_operand(&op_end);
			if(src1 == -1)
				return;
			while (ISSPACE (*op_end))
			op_end++;
			if(*op_end != ',')
			{
				as_bad(_("expecting ',' delimitted operand got >%s< : %s"),op_end,str);
				return;
			}
			op_end++; // skip ,
			while (ISSPACE (*op_end))
			op_end++;
			
			src2=parse_register_operand(&op_end);
			if(src2 == -1)
				return;
			while (ISSPACE (*op_end))
			op_end++;
			if(*op_end != ',')
			{
				as_bad(_("expecting ',' delimitted operand got >%s< : %s"),op_end,str);
				return;
			}
			op_end++; // skip ,
			while (ISSPACE (*op_end))
			op_end++;
			if(*op_end == 0)
			{
				as_bad(_("expecting an operand in %s"),str);
				return;
			}
			op_end = parse_exp_save_ilp (op_end, &arg);
			
			switch (arg.X_op)
			{
				case O_constant:
				
					immediate=arg.X_add_number;
					p = frag_more(eco32_insn_byte); //number of bytes in insn
					iword |= (opcode->opcode << opc_offset);
					iword |= src1 << RRB_src1;
					iword |= src2 << RRB_src2;
					iword |= ((immediate -((unsigned)frag_now->fr_address+4))/4)& lower_part;
					md_number_to_chars(p,iword,eco32_insn_byte);
				break;
				case O_symbol:
					p = frag_more (eco32_insn_byte);
					iword=0x0;
					iword |= (opcode->opcode << opc_offset);
					iword |= src1 << RRB_src1;
					iword |= src2 << RRB_src2;
					fix_new_exp (frag_now,		/* Which frag?  */
							 (p-frag_now->fr_literal),			/* Where in that frag?  */
							 (eco32_insn_byte),			/* 1, 2, or 4 usually.  */
							 &arg,		/* Expression.  */
							 FALSE,			/* TRUE if PC-relative relocation.  */
							 BFD_RELOC_ECO32_METHOD_R16		/* Relocation type.  */);
					md_number_to_chars(p,iword,eco32_insn_byte);
				break;
			}
		}
		break;
		case ECO32_RRR: /* three registers  */
		{
			p = frag_more(eco32_insn_byte); //number of bytes in insn
			iword |= (opcode->opcode << opc_offset);
			regno=parse_register_operand(&op_end);
			if(regno == -1)
				return;
			while (ISSPACE (*op_end))
			op_end++;
			
			if(*op_end != ',')
			{
				as_bad(_("RRR OP2 expecting ',' delimitted operand got >%s< : %s"),op_end,str);
				return;
			}
			iword |= regno << RRR_dest_reg;
			op_end++; // skip ,
			while (ISSPACE (*op_end))
			op_end++;
			
			regno=parse_register_operand(&op_end);
			if(regno == -1)
				return;
			while (ISSPACE (*op_end))
			op_end++;
			
			if(*op_end != ',')
			{
				as_bad(_("RRR OP3 expecting ',' delimitted operand got >%s< : %s"),op_end,str);
				return;
			}
			iword |= regno << RRR_src1_reg;
			op_end++; // skip ,
			while (ISSPACE (*op_end))
			op_end++;
			
			regno=parse_register_operand(&op_end);
			if(regno == -1)
				return;
			iword |= regno << RRR_src2_reg;

			md_number_to_chars(p,iword,eco32_insn_byte);
		}
		break;
		default:
		{
			/* this should never happen */
			as_bad (_("unrecognizable instruction %s\n"),str);
		}
		break;
	}
 
 if (pending_reloc)
 as_bad ("Something forgot to clean up\n");
 
}

/* Turn a string in input_line_pointer into a floating point constant
 of type type, and store the appropriate bytes in *LITP. The number
 of LITTLENUMS emitted is stored in *SIZEP . An error message is
 returned, or NULL on OK. */

char *
md_atof (int type, char *litP, int *sizeP)
{
 int prec;
 LITTLENUM_TYPE words[4];
 char *t;
 int i;

 switch (type)
 {
 case 'f':
 prec = 2;
 break;

 case 'd':
 prec = 4;
 break;

 default:
 *sizeP = 0;
 return _("bad call to md_atof");
 }

 t = atof_ieee (input_line_pointer, type, words);
 if (t)
 input_line_pointer = t;

 *sizeP = prec * 2;

 for (i = prec - 1; i >= 0; i--)
 {
 md_number_to_chars (litP, (valueT) words[i], 2);
 litP += 2;
 }

 return NULL;
}

const char *md_shortopts = "";

struct option md_longopts[] =
{
 {NULL, no_argument, NULL, 0}
};
size_t md_longopts_size = sizeof (md_longopts);

/* We have no target specific options yet, so these next
 two functions are empty.  */
int
md_parse_option (int c ATTRIBUTE_UNUSED, char *arg ATTRIBUTE_UNUSED)
{
 return 0;
}

void
md_show_usage (FILE *stream ATTRIBUTE_UNUSED)
{
}

/* Apply a fixup to the object file. */

void
md_apply_fix (fixS *fixP ATTRIBUTE_UNUSED, valueT * valP ATTRIBUTE_UNUSED, segT seg ATTRIBUTE_UNUSED)
{
 /* Empty forever. */
}

/* Put number into target byte order. */

void
md_number_to_chars (char *ptr, valueT use, int nbytes)
{
 number_to_chars_bigendian (ptr, use, nbytes);
}

/* Generate a machine-dependent relocation.  */
arelent *
tc_gen_reloc (asection *section ATTRIBUTE_UNUSED, fixS *fixP)
{
  arelent *relP;
  bfd_reloc_code_real_type code;
  /* fixes off-by-one error in jumps and branches 
   *  bfd_elf_generic_reloc for rewrite  */
  switch (fixP->fx_r_type)
    {
	case BFD_RELOC_ECO32_METHOD_R26:
    case BFD_RELOC_ECO32_METHOD_R16:
		fixP->fx_offset -=1; 
		/* This hack fixes the off-by-one error in branches and jumps
		 * as long as it is working it will be kept this way
		 * looking into how to fix this isn't currently a priority */
    case BFD_RELOC_32:
    case BFD_RELOC_ECO32_METHOD_L16:
    case BFD_RELOC_ECO32_METHOD_H16:
      code = fixP->fx_r_type;
      break;
    default:
      as_bad_where (fixP->fx_file, fixP->fx_line,
		    _("Semantics error.  This type of operand can not be relocated, it must be an assembly-time constant"));
      return 0;
    }

  relP = xmalloc (sizeof (arelent));
  gas_assert (relP != 0);
  relP->sym_ptr_ptr = xmalloc (sizeof (asymbol *));
  *relP->sym_ptr_ptr = symbol_get_bfdsym (fixP->fx_addsy);
  relP->address = fixP->fx_frag->fr_address + fixP->fx_where;

  relP->addend = fixP->fx_offset;

  relP->howto = bfd_reloc_type_lookup (stdoutput, code);
  if (! relP->howto)
    {
      const char *name;

      name = S_GET_NAME (fixP->fx_addsy);
      if (name == NULL)
	name = _("<unknown>");
      as_fatal (_("Cannot generate relocation type for symbol %s, code %s"),
		name, bfd_get_reloc_code_name (code));
    }

  return relP;
}
