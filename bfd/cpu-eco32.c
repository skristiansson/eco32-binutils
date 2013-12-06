#include "sysdep.h"
#include "bfd.h"
#include "libbfd.h"

const bfd_arch_info_type
bfd_eco32_arch =
 {
   32,				/* 32 bits in a word.  */
   32,				/* 32 bits in an address.  */
   8,				/* 8 bits in a byte.  */
   bfd_arch_eco32,		/* Architecture.  */
   bfd_mach_eco32,				/* Machine */
   "eco32",			/* Architecture name.  */
   "eco32",			/* Printable name.  */
   4,				/* Section align power.  */
   TRUE,			/* This is the default architecture.  */
   bfd_default_compatible,	/* Architecture comparison function.  */
   bfd_default_scan,		/* String to architecture conversion.  */
   bfd_arch_default_fill,	/* Default fill.  */
   NULL				/* Next in list.  */
};
