#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"
#include "elf-bfd.h"
#include "elf/eco32.h"

/*
* see bfd-in2.h line ~ 2246
* docu is there
*/
static reloc_howto_type eco32_elf_howto_table[] =
{
HOWTO (R_ECO32_NONE, /* type */
	0, /* rightshift */
	0, /* size (0 = byte, 1 = short, 2 = long) */
	0, /* bitsize */
	FALSE, /* pc_relative */
	0, /* bitpos */
	complain_overflow_dont, /* complain_on_overflow */
	bfd_elf_generic_reloc, /* special_function */
	"R_ECO32_NONE", /* name */
	FALSE, /* partial_inplace */
	0, /* src_mask */
	0, /* dst_mask */
	FALSE), /* pcrel_offset */
	
/* write full 32 bit word with value => full word change */
HOWTO (R_ECO32_W32,	/* type */
	0,	/* rightshift */
	2,	/* size (0 = byte, 1 = short, 2 = long) */
	32,	/* bitsize */
	FALSE,	/* pc_relative */
	0,	/* bitpos */
	complain_overflow_unsigned, /* complain_on_overflow */
	bfd_elf_generic_reloc, /* special_function */
	"METHOD_W32",	/* name */
	FALSE,	/* partial_inplace */
	0,	/* src_mask */
	0xffffffff,	/* dst_mask */
	FALSE),	/* pcrel_offset */

/* write 16 bits with high part of value => LDHI */
HOWTO (R_ECO32_HI16,	/* type */
	16,	/* rightshift */
	2,	/* size (0 = byte, 1 = short, 2 = long) */
	16,	/* bitsize */
	FALSE,	/* pc_relative */
	0,	/* bitpos */
	complain_overflow_dont, /* complain_on_overflow */
	bfd_elf_generic_reloc, /* special_function */
	"METHOD_H16",	/* name */
	FALSE,	/* partial_inplace */
	0,	/* src_mask */
	0x0000FFFF,	/* dst_mask */
	FALSE),	/* pcrel_offset */

/* write 16 bits with low part of value => immediate values */
HOWTO (R_ECO32_LO16,	/* type */
	0,	/* rightshift */
	2,	/* size (0 = byte, 1 = short, 2 = long) */
	0,	/* bitsize */
	FALSE,	/* pc_relative */
	0,	/* bitpos */
	complain_overflow_dont, /* complain_on_overflow */
	bfd_elf_generic_reloc, /* special_function */
	"METHOD_L16",	/* name */
	FALSE,	/* partial_inplace */
	0,	/* src_mask */
	0x0000FFFF,	/* dst_mask */
	FALSE),	/* pcrel_offset */
	
/* write 16 bits with value relative to PC => branch*/
HOWTO (R_ECO32_R16,	/* type */
	2,	/* rightshift */
	2,	/* size (0 = byte, 1 = short, 2 = long) */
	16,	/* bitsize */
	TRUE,	/* pc_relative */
	0,	/* bitpos */
	complain_overflow_signed, /* complain_on_overflow */
	bfd_elf_generic_reloc, /* special_function */
	"METHOD_R16",	/* name */
	FALSE,	/* partial_inplace */
	0,	/* src_mask */
	0x0000FFFF,	/* dst_mask */
	TRUE),	/* pcrel_offset */

/* write 26 bits with value relative to PC => jumps*/
HOWTO (R_ECO32_R26,	/* type */
	2,	/* rightshift */
	2,	/* size (0 = byte, 1 = short, 2 = long) */
	26,	/* bitsize */
	TRUE,	/* pc_relative */
	0,	/* bitpos */
	complain_overflow_signed, /* complain_on_overflow */
	bfd_elf_generic_reloc, /* special_function */
	"METHOD_R26",	/* name */
	FALSE,	/* partial_inplace */
	0,	/* src_mask */
	0x03FFFFFF,	/* dst_mask */
	TRUE),	/* pcrel_offset */	
};

/* This structure is used to map BFD reloc codes to eco32 elf relocs. */

struct elf_reloc_map
{
	bfd_reloc_code_real_type bfd_reloc_val;
	unsigned char elf_reloc_val;
};

static const struct elf_reloc_map eco32_reloc_map [] =
{
	{ BFD_RELOC_ECO32_METHOD_H16, R_ECO32_HI16 },
	{ BFD_RELOC_ECO32_METHOD_L16, R_ECO32_LO16 },
	{ BFD_RELOC_ECO32_METHOD_R16, R_ECO32_R16 },
	{ BFD_RELOC_ECO32_METHOD_R26, R_ECO32_R26 },
	{ BFD_RELOC_32, R_ECO32_W32 },
	{ BFD_RELOC_NONE, R_ECO32_NONE }
};

/* Given a BFD reloc code, return the howto structure for the
corresponding eco32 elf reloc. */
static reloc_howto_type *
eco32_elf_reloc_type_lookup (bfd *abfd ATTRIBUTE_UNUSED,
	bfd_reloc_code_real_type code)
{
	unsigned int i;

	for (i = 0; i < sizeof (eco32_reloc_map) / sizeof (struct elf_reloc_map); i++)
		if (eco32_reloc_map[i].bfd_reloc_val == code)
			return & eco32_elf_howto_table[(int) eco32_reloc_map[i].elf_reloc_val];

	return NULL;
}

static reloc_howto_type *
eco32_elf_reloc_name_lookup (bfd *abfd ATTRIBUTE_UNUSED,
	const char *r_name)
{
	unsigned int i;

	for (i = 0; i < sizeof (eco32_elf_howto_table) / sizeof (eco32_elf_howto_table[0]); i++)
		if (eco32_elf_howto_table[i].name != NULL
			&& strcasecmp (eco32_elf_howto_table[i].name, r_name) == 0)
		return &eco32_elf_howto_table[i];

	return NULL;
}

/* Given an ELF reloc, fill in the howto field of a relent. */

static void
eco32_elf_info_to_howto (bfd *abfd ATTRIBUTE_UNUSED, arelent *cache_ptr,
	Elf_Internal_Rela *dst)
{
	unsigned int r;
	r = ELF32_R_TYPE (dst->r_info);
	BFD_ASSERT (r < (unsigned int) R_ECO32_max);
	cache_ptr->howto = &eco32_elf_howto_table[r];
}

static bfd_boolean
eco32_elf_relocate_section (bfd *output_bfd,
			    struct bfd_link_info *info,
			    bfd *input_bfd,
			    asection *input_section,
			    bfd_byte *contents,
			    Elf_Internal_Rela *relocs,
			    Elf_Internal_Sym *local_syms,
			    asection **local_sections)
{
  Elf_Internal_Shdr *symtab_hdr;
  struct elf_link_hash_entry **sym_hashes;
  Elf_Internal_Rela *rel;
  Elf_Internal_Rela *relend;

  symtab_hdr = &elf_tdata (input_bfd)->symtab_hdr;
  sym_hashes = elf_sym_hashes (input_bfd);
  relend = relocs + input_section->reloc_count;

  for (rel = relocs; rel < relend; rel++)
    {
      reloc_howto_type *howto;
      unsigned long r_symndx;
      Elf_Internal_Sym *sym;
      asection *sec;
      struct elf_link_hash_entry *h;
      bfd_vma relocation;
      bfd_reloc_status_type r;
      const char *name = NULL;
      int r_type;

      r_type = ELF32_R_TYPE (rel->r_info);
      r_symndx = ELF32_R_SYM (rel->r_info);

      if (r_type < 0 || r_type >= (int) R_ECO32_max)
        {
	  bfd_set_error (bfd_error_bad_value);
	  return FALSE;
        }

      howto = eco32_elf_howto_table + ELF32_R_TYPE (rel->r_info);
      h = NULL;
      sym = NULL;
      sec = NULL;

      if (r_symndx < symtab_hdr->sh_info)
        {
	  sym = local_syms + r_symndx;
	  sec = local_sections[r_symndx];
	  relocation = _bfd_elf_rela_local_sym (output_bfd, sym, &sec, rel);

	  name = bfd_elf_string_from_elf_section
	    (input_bfd, symtab_hdr->sh_link, sym->st_name);
	  name = (name == NULL) ? bfd_section_name (input_bfd, sec) : name;
        }
      else
        {
	  bfd_boolean unresolved_reloc, warned;

	  RELOC_FOR_GLOBAL_SYMBOL (info, input_bfd, input_section, rel,
				   r_symndx, symtab_hdr, sym_hashes,
				   h, sec, relocation,
				   unresolved_reloc, warned);
        }

      if (sec != NULL && discarded_section (sec))
	RELOC_AGAINST_DISCARDED_SECTION (info, input_bfd, input_section,
					 rel, 1, relend, howto, 0, contents);

      if (info->relocatable)
	continue;

      r = _bfd_final_link_relocate (howto, input_bfd, input_section, contents,
                                    rel->r_offset, relocation, rel->r_addend);

      if (r != bfd_reloc_ok)
	{
	  const char *msg = NULL;

	  switch (r)
	    {
	    case bfd_reloc_overflow:
	      r = info->callbacks->reloc_overflow
		(info, (h ? &h->root : NULL), name, howto->name,
		 (bfd_vma) 0, input_bfd, input_section, rel->r_offset);
	      break;

	    case bfd_reloc_undefined:
	      r = info->callbacks->undefined_symbol
		(info, name, input_bfd, input_section, rel->r_offset, TRUE);
	      break;

	    case bfd_reloc_outofrange:
	      msg = _("internal error: out of range error");
	      break;

	    case bfd_reloc_notsupported:
	      msg = _("internal error: unsupported relocation error");
	      break;

	    case bfd_reloc_dangerous:
	      msg = _("internal error: dangerous relocation");
	      break;

	    default:
	      msg = _("internal error: unknown error");
	      break;
	    }

	  if (msg)
	    r = info->callbacks->warning
	      (info, msg, name, input_bfd, input_section, rel->r_offset);

	  if (!r)
	    return FALSE;
        }
    }

  return TRUE;
}

#define TARGET_BIG_SYM bfd_elf32_eco32_vec
#define TARGET_BIG_NAME "elf32-eco32"

#define ELF_ARCH bfd_arch_eco32
#define ELF_MACHINE_CODE EM_ECO32

#define ELF_MAXPAGESIZE 0x1000
#define ARCH_SIZE 32

#define bfd_elf32_bfd_reloc_type_lookup eco32_elf_reloc_type_lookup
#define bfd_elf32_bfd_reloc_name_lookup eco32_elf_reloc_name_lookup
#define elf_info_to_howto_rel 0
#define elf_info_to_howto eco32_elf_info_to_howto
#define elf_backend_relocate_section eco32_elf_relocate_section
#define elf_backend_rela_normal	1

#include "elf32-target.h"
