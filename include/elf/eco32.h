#ifndef _ELF_ECO32_H
#define _ELF_ECO32_H

#include "elf/reloc-macros.h"

/* Relocations.  */
START_RELOC_NUMBERS (elf_eco32_reloc_type)
  RELOC_NUMBER (R_ECO32_NONE, 0)
  RELOC_NUMBER (R_ECO32_W32, 1)
  RELOC_NUMBER (R_ECO32_HI16, 2)
  RELOC_NUMBER (R_ECO32_LO16, 3)
  RELOC_NUMBER (R_ECO32_R16, 4)
  RELOC_NUMBER (R_ECO32_R26, 5)
END_RELOC_NUMBERS (R_ECO32_max)


#endif /* _ELF_ECO32_H */
