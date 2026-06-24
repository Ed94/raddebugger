// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_DUMP_H
#define DWARF_DUMP_H

////////////////////////////////
//~ rjf: Helpers

internal String8List dw_dump_list_from_expr(Arena *arena, String8 data, Arch arch, DW2_ParseCtx *ctx);

////////////////////////////////
//~ rjf: Main Dump Entry Point

internal String8List dw_dump_list_from_raw(Arena *arena, DW_Raw *raw, Arch arch, DW_SectionFlags subset_flags);

#endif // DWARF_DUMP_H
