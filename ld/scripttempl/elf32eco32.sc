cat <<EOF
OUTPUT_ARCH(${ARCH})
OUTPUT_FORMAT(${OUTPUT_FORMAT})
${RELOCATING+${LIB_SEARCH_DIRS}}
ENTRY(${ENTRY})

PHDRS
{
  headers PT_PHDR PHDRS ;
  interp PT_INTERP ;
  text PT_LOAD FILEHDR PHDRS ;
  data PT_LOAD ;
  dynamic PT_DYNAMIC ;
}

SECTIONS
{
  . = SIZEOF_HEADERS;
  .interp : { *(.interp) } :text :interp
  . = ${TEXT_START_ADDR};
  .text : { *(.text) } :text
  .rodata : { *(.rodata) } /* defaults to :text */
  /* move to a new page in memory */
  .data ALIGN(${MAXPAGESIZE})  : {*(.data) } :data
  .dynamic : { *(.dynamic) } :data :dynamic
}
EOF