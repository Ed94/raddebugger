// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "dwarf/dwarf.c"
#include "dwarf/dwarf_parse.c"
#include "dwarf/dwarf_expr.c"
#include "dwarf/dwarf_unwind.c"
#include "dwarf/dwarf_dump.c"
#if defined(COFF_H)
# include "dwarf/dwarf_parse_coff.c"
#endif
#if defined(ELF_H)
# include "dwarf/dwarf_parse_elf.c"
#endif
