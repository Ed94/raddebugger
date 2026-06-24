// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Build Options

#define BUILD_TITLE "ryan_scratch"
#define BUILD_CONSOLE_INTERFACE 1

////////////////////////////////
//~ rjf: Includes

//- rjf: [h]
#include "base/base_inc.h"
#include "x64/x64.h"
#include "artifact_cache/artifact_cache.h"
#include "rdi/rdi_local.h"
#include "dbg_info/dbg_info.h"
#include "arch/arch_inc.h"
#include "eval2/eval2.h"

//- rjf: [c]
#include "base/base_inc.c"
#include "x64/x64.c"
#include "artifact_cache/artifact_cache.c"
#include "rdi/rdi_local.c"
#include "dbg_info/dbg_info.c"
#include "arch/arch_inc.c"
#include "eval2/eval2.c"

////////////////////////////////
//~ rjf: Entry Point

internal void
entry_point(CmdLine *cmdline)
{
  String8 strings[] =
  {
    s("[123]"),
    s("foo"),
    s("foo.bar"),
    s("123[456]"),
    s("123"),
    s("123 + 456"),
    s("!1"),
    s("!0"),
    s("1 + 1 + 1"),
    s("40 / 0"),
    s("3 * 4"),
    s("3 * 4 + 2"),
    s("(3 * 4) + 2"),
    s("2 + (3 * 4)"),
    s("123 | 456"),
    s("123 + "),
    s("-123"),
    s("sizeof 123"),
    s("1 ? 123 : 456"),
    s("123(1, 2, 3)"),
  };
  for EachElement(idx, strings)
  {
    Temp scratch = scratch_begin(0, 0);
    String8List msgs = {0};
    
    // rjf: string -> expr
    E2_Expr *expr = &e2_expr_nil;
    {
      E2_ParseState state = {0};
      E2_ExprMap expr_map = {0};
      E2_Expr *access_expr = &e2_expr_nil;
      for(;;)
      {
        E2_Parse parse = e2_parse_from_string(scratch.arena, &state, &expr_map, access_expr, strings[idx]);
        expr = parse.expr;
        access_expr = &e2_expr_nil;
        for EachNode(n, E2_Msg, parse.msgs.first)
        {
          str8_list_push(scratch.arena, &msgs, n->string);
        }
        if(parse.status == E2_ParseStatus_MissedIdentifierResolution)
        {
          e2_expr_map_push(scratch.arena, &expr_map, parse.missed_identifier, e2_expr_const_u64_or_smaller(scratch.arena, 123));
        }
        if(parse.status == E2_ParseStatus_IndexAccess)
        {
          access_expr = e2_expr_const_u64_or_smaller(scratch.arena, 111);
        }
        if(parse.status == E2_ParseStatus_MemberAccess)
        {
          access_expr = e2_expr_const_u64_or_smaller(scratch.arena, 456);
        }
        if(e2_parse_status_is_terminal(parse.status))
        {
          break;
        }
      }
    }
    
    // rjf: expr -> bytecode
    String8 bytecode = e2_bytecode_from_expr(scratch.arena, expr);
    
    // rjf: bytecode -> value
    E2_Val val = {0};
    {
      E2_InterpState state = {0};
      E2_SpaceMap space_map = {0};
      for(;;)
      {
        E2_Interp interp = e2_interp_from_bytecode(scratch.arena, &state, &space_map, bytecode);
        val = interp.val;
        if(e2_interp_status_is_terminal(interp.status))
        {
          if(interp.status != E2_InterpStatus_Good)
          {
            str8_list_pushf(scratch.arena, &msgs, "Interpretation error (%i).", interp.status);
          }
          break;
        }
      }
    }
    
    // rjf: message string list -> string
    StringJoin join = {.sep = s(" ")};
    String8 msgs_string = str8_list_join(scratch.arena, &msgs, &join);
    
    // rjf: log
    printf("%.*s -> %I64d *%s* %s%.*s\n", str8_varg(strings[idx]),
           val.s64,
           expr->mode == E2_Mode_Type ? "type" :
           expr->mode == E2_Mode_Value ? "value" :
           "address",
           msgs_string.size != 0 ? " // " : "", str8_varg(msgs_string));
    fflush(stdout);
    
    scratch_end(scratch);
  }
}
