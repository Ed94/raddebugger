// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Helpers

internal String8List
dw_dump_list_from_expr(Arena *arena, String8 data, Arch arch, DW2_ParseCtx *ctx)
{
  String8List result = {0};
  Temp scratch = scratch_begin(&arena, 1);
  ARCH_Info *arch_info = arch_info_from_arch(arch);
  for(U64 off = 0; off < data.size;)
  {
    U64 start_off = 0;
    
    // rjf: read next op
    U8 op = 0;
    off += str8_deserial_read_struct(data, off, &op);
    
    // rjf: unpack op / operands
    String8 operand_string = {0};
    U64 size_param = 0;
    B32 is_signed  = 0;
    switch(op)
    {
      case DW_ExprOp_Lit0:  case DW_ExprOp_Lit1:  case DW_ExprOp_Lit2:
      case DW_ExprOp_Lit3:  case DW_ExprOp_Lit4:  case DW_ExprOp_Lit5:
      case DW_ExprOp_Lit6:  case DW_ExprOp_Lit7:  case DW_ExprOp_Lit8:
      case DW_ExprOp_Lit9:  case DW_ExprOp_Lit10: case DW_ExprOp_Lit11:
      case DW_ExprOp_Lit12: case DW_ExprOp_Lit13: case DW_ExprOp_Lit14:
      case DW_ExprOp_Lit15: case DW_ExprOp_Lit16: case DW_ExprOp_Lit17:
      case DW_ExprOp_Lit18: case DW_ExprOp_Lit19: case DW_ExprOp_Lit20:
      case DW_ExprOp_Lit21: case DW_ExprOp_Lit22: case DW_ExprOp_Lit23:
      case DW_ExprOp_Lit24: case DW_ExprOp_Lit25: case DW_ExprOp_Lit26:
      case DW_ExprOp_Lit27: case DW_ExprOp_Lit28: case DW_ExprOp_Lit29:
      case DW_ExprOp_Lit30: case DW_ExprOp_Lit31:
      {
        U64 x = op - DW_ExprOp_Lit0;
        operand_string = str8f(scratch.arena, "%I64u", x);
      }break;
      
      case DW_ExprOp_Const1U:size_param = 1;                goto const_n;
      case DW_ExprOp_Const2U:size_param = 2;                goto const_n;
      case DW_ExprOp_Const4U:size_param = 4;                goto const_n;
      case DW_ExprOp_Const8U:size_param = 8;                goto const_n;
      case DW_ExprOp_Const1S:size_param = 1; is_signed = 1; goto const_n;
      case DW_ExprOp_Const2S:size_param = 2; is_signed = 1; goto const_n;
      case DW_ExprOp_Const4S:size_param = 4; is_signed = 1; goto const_n;
      case DW_ExprOp_Const8S:size_param = 8; is_signed = 1; goto const_n;
      const_n:
      {
        if(is_signed)
        {
          S64 x = 0;
          off += str8_deserial_read(data, off, &x, size_param, 1);
          x = extend_sign64(x, size_param);
          operand_string = str8f(scratch.arena, "%I64d", x);
        }
        else
        {
          U64 x = 0;
          off += str8_deserial_read(data, off, &x, size_param, 1);
          operand_string = str8f(scratch.arena, "%I64u", x);
        }
      }break;
      
      case DW_ExprOp_Addr:
      {
        U64 addr = 0;
        off += str8_deserial_read(data, off, &addr, ctx->addr_size, 1);
        operand_string = str8f(scratch.arena, "0x%I64x", addr);
      }break;
      
      case DW_ExprOp_ConstU:
      {
        U64 x = 0;
        off += str8_deserial_read_uleb128(data, off, &x);
        operand_string = str8f(scratch.arena, "%I64u", x);
      }break;
      
      case DW_ExprOp_ConstS:
      {
        S64 x = 0;
        off += str8_deserial_read_sleb128(data, off, &x);
        operand_string = str8f(scratch.arena, "%I64d", x);
      }break;
      
      case DW_ExprOp_Reg0:  case DW_ExprOp_Reg1:  case DW_ExprOp_Reg2:
      case DW_ExprOp_Reg3:  case DW_ExprOp_Reg4:  case DW_ExprOp_Reg5:
      case DW_ExprOp_Reg6:  case DW_ExprOp_Reg7:  case DW_ExprOp_Reg8:
      case DW_ExprOp_Reg9:  case DW_ExprOp_Reg10: case DW_ExprOp_Reg11:
      case DW_ExprOp_Reg12: case DW_ExprOp_Reg13: case DW_ExprOp_Reg14:
      case DW_ExprOp_Reg15: case DW_ExprOp_Reg16: case DW_ExprOp_Reg17:
      case DW_ExprOp_Reg18: case DW_ExprOp_Reg19: case DW_ExprOp_Reg20:
      case DW_ExprOp_Reg21: case DW_ExprOp_Reg22: case DW_ExprOp_Reg23:
      case DW_ExprOp_Reg24: case DW_ExprOp_Reg25: case DW_ExprOp_Reg26:
      case DW_ExprOp_Reg27: case DW_ExprOp_Reg28: case DW_ExprOp_Reg29:
      case DW_ExprOp_Reg30: case DW_ExprOp_Reg31:
      {
        U64 reg_idx = op - DW_ExprOp_Reg0;
        ARCH_RegCode regcode = arch_reg_code_from_dw(arch, (DW_RegCode)reg_idx);
        operand_string = arch_info->reg_code_name_table[regcode];
      }break;
      
      case DW_ExprOp_RegX:
      {
        U64 reg_idx = 0;
        off += str8_deserial_read_uleb128(data, off, &reg_idx);
        ARCH_RegCode regcode = arch_reg_code_from_dw(arch, (DW_RegCode)reg_idx);
        operand_string = arch_info->reg_code_name_table[regcode];
      }break;
      
      case DW_ExprOp_ImplicitValue:
      {
        U64 value_size = 0;
        off += str8_deserial_read_uleb128(data, off, &value_size);
        Rng1U64 value_range = rng_1u64(off, off + value_size);
        String8 value_data  = str8_substr(data, value_range);
        off += value_size;
        String8List value_strings = numeric_str8_list_from_data(scratch.arena, 16, value_data, 1);
        operand_string = str8_list_join(scratch.arena, &value_strings, &(StringJoin){.pre = str8_lit("{ "), .sep = str8_lit(", "), .post = str8_lit(" }")});
      }break;
      
      case DW_ExprOp_Piece:
      {
        U64 size = 0;
        off += str8_deserial_read_uleb128(data, off, &size);
        operand_string = str8f(scratch.arena, "%u", size);
      }break;
      
      case DW_ExprOp_BitPiece:
      {
        U64 bit_size = 0, bit_off = 0;
        off += str8_deserial_read_uleb128(data, off, &bit_size);
        off += str8_deserial_read_uleb128(data, off, &bit_off);
        operand_string = str8f(scratch.arena, "bit size %I64u, bit offset %I64u", bit_size, bit_off);
      }break;
      
      case DW_ExprOp_Pick:
      {
        U8 stack_idx = 0;
        off += str8_deserial_read_struct(data, off, &stack_idx);
        operand_string = str8f(scratch.arena, "stack index %u", stack_idx);
      }break;
      
      case DW_ExprOp_PlusUConst:
      {
        U64 addend = 0;
        off += str8_deserial_read_uleb128(data, off, &addend);
        operand_string = str8f(arena, "addend %I64u", addend);
      }break;
      
      case DW_ExprOp_Skip:
      {
        S16 x = 0;
        off += str8_deserial_read_struct(data, off, &x);
        operand_string = str8f(scratch.arena, "%+d bytes", x);
      }break;
      
      case DW_ExprOp_Bra:
      {
        S16 x = 0;
        off += str8_deserial_read_struct(data, off, &x);
        operand_string = str8f(scratch.arena, "%+d", x);
      }break;
      
      case DW_ExprOp_BReg0:  case DW_ExprOp_BReg1:  case DW_ExprOp_BReg2: 
      case DW_ExprOp_BReg3:  case DW_ExprOp_BReg4:  case DW_ExprOp_BReg5: 
      case DW_ExprOp_BReg6:  case DW_ExprOp_BReg7:  case DW_ExprOp_BReg8:  
      case DW_ExprOp_BReg9:  case DW_ExprOp_BReg10: case DW_ExprOp_BReg11: 
      case DW_ExprOp_BReg12: case DW_ExprOp_BReg13: case DW_ExprOp_BReg14: 
      case DW_ExprOp_BReg15: case DW_ExprOp_BReg16: case DW_ExprOp_BReg17: 
      case DW_ExprOp_BReg18: case DW_ExprOp_BReg19: case DW_ExprOp_BReg20: 
      case DW_ExprOp_BReg21: case DW_ExprOp_BReg22: case DW_ExprOp_BReg23: 
      case DW_ExprOp_BReg24: case DW_ExprOp_BReg25: case DW_ExprOp_BReg26: 
      case DW_ExprOp_BReg27: case DW_ExprOp_BReg28: case DW_ExprOp_BReg29: 
      case DW_ExprOp_BReg30: case DW_ExprOp_BReg31:
      {
        U64 reg_idx = op - DW_ExprOp_BReg0;
        S64 reg_off = 0;
        off += str8_deserial_read_sleb128(data, off, &reg_off);
        ARCH_RegCode regcode = arch_reg_code_from_dw(arch, (DW_RegCode)reg_idx);
        operand_string = str8f(scratch.arena, "%S+%I64d", arch_info->reg_code_name_table[regcode], reg_off);
      }break;
      
      case DW_ExprOp_FBReg:
      {
        S64 reg_off = 0;
        off += str8_deserial_read_sleb128(data, off, &reg_off);
        operand_string = str8f(scratch.arena, "frame base offset %I64d", reg_off);
      }break;
      
      case DW_ExprOp_BRegX:
      {
        U64 reg_idx = 0;
        S64 reg_off = 0;
        off += str8_deserial_read_uleb128(data, off, &reg_idx);
        off += str8_deserial_read_sleb128(data, off, &reg_off);
        ARCH_RegCode regcode = arch_reg_code_from_dw(arch, (DW_RegCode)reg_idx);
        operand_string = str8f(scratch.arena, "%S+%I64d", arch_info->reg_code_name_table[regcode], reg_off);
      }break;
      
      case DW_ExprOp_XDerefSize:
      case DW_ExprOp_DerefSize:
      {
        U8 x = 0;
        off += str8_deserial_read_struct(data, off, &x);
        operand_string = str8f(scratch.arena, "%u", x);
      }break;
      
      case DW_ExprOp_Call2:
      {
        U16 x = 0;
        off += str8_deserial_read_struct(data, off, &x);
        operand_string = str8f(scratch.arena, "%u", x);
      }break;
      case DW_ExprOp_Call4:
      {
        U32 x = 0;
        off += str8_deserial_read_struct(data, off, &x);
        operand_string = str8f(arena, "%u", x);
      }break;
      case DW_ExprOp_CallRef:
      {
        U64 x = 0;
        off += dw2_read_fmt_u64(data, off, ctx->format, &x);
        operand_string = str8f(scratch.arena, "%I64u", x);
      }break;
      case DW_ExprOp_ImplicitPointer:
      case DW_GNU_ExprOp_ImplicitPointer:
      {
        U64 info_off = 0;
        off += dw2_read_fmt_u64(data, off, ctx->format, &info_off);
        S64 ptr = 0;
        off += str8_deserial_read_sleb128(data, off, &ptr);
        operand_string = str8f(scratch.arena, ".debug_info@0x%I64x, ptr %I64x", info_off, ptr);
      } break;
      case DW_ExprOp_Convert:
      case DW_GNU_ExprOp_Convert:
      {
        U64 type_cu_off = 0;
        off += str8_deserial_read_uleb128(data, off, &type_cu_off);
        operand_string = str8f(scratch.arena, "TypeCuOff %#I64x", ctx->unit_base_info_off + type_cu_off);
      }break;
      case DW_GNU_ExprOp_ParameterRef:
      {
        // TODO: always 4 bytes?
        U32 cu_off = 0;
        off += str8_deserial_read_struct(data, off, &cu_off);
        operand_string = str8f(scratch.arena, "CuOff %#x", ctx->unit_base_info_off + cu_off);
      }break;
      case DW_ExprOp_DerefType:
      case DW_GNU_ExprOp_DerefType:
      {
        U8 deref_size  = 0;
        U64 type_cu_off = 0;
        off += str8_deserial_read_struct(data, off, &deref_size);
        off += str8_deserial_read_uleb128(data, off, &type_cu_off);
        operand_string = str8f(scratch.arena, "%#x, TypeCuOff %#I64x", deref_size, ctx->unit_base_info_off + type_cu_off);
      }break;
      case DW_ExprOp_ConstType:
      case DW_GNU_ExprOp_ConstType:
      {
        U64 type_cu_off = 0;
        U8 const_value_size = 0;
        off += str8_deserial_read_uleb128(data, off, &type_cu_off);
        off += str8_deserial_read_struct(data, off, &const_value_size);
        Rng1U64 const_value_range = rng_1u64(off, off + const_value_size);
        String8 const_value_data  = str8_substr(data, const_value_range);
        String8List const_value_strings = numeric_str8_list_from_data(scratch.arena, 16, const_value_data, 1);
        String8 const_value_str = str8_list_join(scratch.arena, &const_value_strings, &(StringJoin){.sep = str8_lit(", ")});
        operand_string = str8f(scratch.arena, "TypeCuOff %#I64x, Const Value { %S }", ctx->unit_base_info_off + type_cu_off, const_value_str);
        off += const_value_size;
      }break;
      case DW_ExprOp_RegvalType:
      case DW_GNU_ExprOp_RegvalType:
      {
        U64 reg_idx = 0, type_cu_off = 0;
        off += str8_deserial_read_uleb128(data, off, &reg_idx);
        off += str8_deserial_read_uleb128(data, off, &type_cu_off);
        ARCH_RegCode regcode = arch_reg_code_from_dw(arch, (DW_RegCode)reg_idx);
        operand_string = str8f(scratch.arena, "%S, TypeCuOff %#I64x", arch_info->reg_code_name_table[regcode], ctx->unit_base_info_off + type_cu_off);
      }break;
      case DW_ExprOp_EntryValue:
      case DW_GNU_ExprOp_EntryValue:
      {
        U64 block_size = 0;
        off += str8_deserial_read_uleb128(data, off, &block_size);
        Rng1U64 block_range = rng_1u64(off, off + block_size);
        String8 block_data  = str8_substr(data, block_range);
        // TODO(rjf): WTF, recursive? let's not... - if we want this, let's just do the stack inside of this function
        // String8List block_expr = dw_string_list_from_expression(scratch.arena, block_data, cu_base, addr_size, arch, ver, ext, format);
        // operand_string = str8_list_join(scratch.arena, &block_expr, &(StringJoin){.pre = str8_lit("{ "), .sep = str8_lit(","), .post = str8_lit(" }")});
        operand_string = s("{...}");
        off += block_size;
      } break;
      case DW_ExprOp_Addrx:
      {
        U64 addr = 0;
        off += str8_deserial_read_uleb128(data, off, &addr);
        operand_string = str8f(scratch.arena, "%#I64x", addr);
      }break;
    }
    
    // rjf: push opcode info
    String8 opcode_string = dw_string_from_expr_op(ctx->version, ctx->exts, op);
    if(operand_string.size == 0)
    {
      str8_list_pushf(arena, &result, "%S", opcode_string);
    }
    else
    {
      str8_list_pushf(arena, &result, "%S(%S)", opcode_string, operand_string);
    }
  }
  scratch_end(scratch);
  return result;
}

////////////////////////////////
//~ rjf: Main Dump Entry Point

internal String8List
dw_dump_list_from_raw(Arena *arena, DW_Raw *raw, Arch arch, DW_SectionFlags subset_flags)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(&arena, 1);
  String8 indent = str8_lit("                                                                                                                                ");
  
  //////////////////////////////
  //- rjf: set up
  //
  typedef struct DumpSubsetOutputNode DumpSubsetOutputNode;
  struct DumpSubsetOutputNode
  {
    DumpSubsetOutputNode *next;
    DW_SectionKind section_kind;
    String8List *lane_strings;
  };
  DumpSubsetOutputNode *first_output_node = 0;
  DumpSubsetOutputNode *last_output_node = 0;
  String8List *strings = 0;
#define dump(str)  str8_list_push(arena, strings, (str))
#define dumpf(...) str8_list_pushf(arena, strings, __VA_ARGS__)
#define DumpSubsetKind(kind) \
if(lane_idx() == 0)\
{\
DumpSubsetOutputNode *n = push_array(scratch.arena, DumpSubsetOutputNode, 1);\
SLLQueuePush(first_output_node, last_output_node, n);\
n->section_kind = (kind);\
n->lane_strings = push_array(scratch.arena, String8List, lane_count());\
}\
lane_sync_u64(&first_output_node, 0);\
lane_sync_u64(&last_output_node, 0);\
strings = &last_output_node->lane_strings[lane_idx()];\
lane_sync(); if(subset_flags & (1ull<<(kind))) ProfScope((char *)dw_regular_name_from_section_kind(kind).str)
#define DumpSubset(name) DumpSubsetKind(DW_SectionKind_##name)
  
  //////////////////////////////
  //- rjf: .debug_abbrev
  //
  DumpSubset(Abbrev)
  {
    if(lane_idx() == 0)
    {
      String8 data = raw->sec[DW_SectionKind_Abbrev].data;
      U64 idx = 0;
      dumpf("\nabbrevs:\n{\n");
      for(U64 off = 0; off < data.size; idx += 1)
      {
        Temp scratch2 = scratch_begin(&scratch.arena, 1);
        U64 start_off = off;
        
        // rjf: read abbrev info
        DW2_AbbrevHeader abbrev_header = {0};
        DW2_AbbrevAttribList abbrev_attribs = {0};
        off += dw2_read_abbrev(scratch2.arena, data, off, &abbrev_header, &abbrev_attribs);
        
        // rjf: log
        dumpf(" abbrev: // abbrev[%I64u], .debug_abbrev@0x%I64x\n", idx, start_off);
        dumpf(" {\n");
        dumpf("  id:           %I64u\n", abbrev_header.id);
        dumpf("  tag_kind:     %S (0x%I64x)\n", dw_string_from_tag_kind(DW_Version_Last, DW_ExtFlag_All, abbrev_header.tag_kind), abbrev_header.tag_kind);
        dumpf("  has_children: %s\n", abbrev_header.has_children ? "yes" : "no");
        if(abbrev_attribs.count != 0)
        {
          dumpf("  attribs:\n  {\n");
          for EachNode(n, DW2_AbbrevAttribNode, abbrev_attribs.first)
          {
            DW2_AbbrevAttrib *attrib = &n->v;
            dumpf("   {%-24S %-16S%S}\n",
                  dw_string_from_attrib_kind(DW_Version_Last, DW_ExtFlag_All, attrib->attrib_kind),
                  dw_string_from_form_kind(DW_Version_Last, DW_ExtFlag_All, attrib->form_kind),
                  attrib->form_kind == DW_FormKind_ImplicitConst ? str8f(scratch2.arena, " (0x%I64x, %I64u)", attrib->implicit_const, attrib->implicit_const) : s(""));
          }
          dumpf("  }\n");
        }
        dumpf(" }\n");
        
        if(off == start_off)
        {
          break;
        }
      }
      dumpf("}\n");
    }
  }
  
  //////////////////////////////
  //- rjf: .debug_info
  //
  DumpSubset(Info)
  {
    //- rjf: gather unit ranges from .debug_info
    Rng1U64Array *unit_info_ranges = 0;
    if(lane_idx() == 0)
    {
      unit_info_ranges = push_array(scratch.arena, Rng1U64Array, 1);
      *unit_info_ranges = dw2_unit_ranges_from_data(scratch.arena, raw->sec[DW_SectionKind_Info].data);
    }
    lane_sync_u64(&unit_info_ranges, 0);
    U64 unit_count = unit_info_ranges->count;
    
    //- rjf: parse all .debug_info unit headers
    DW2_UnitHeader *unit_headers = 0;
    Rng1U64 *unit_info_tag_ranges = 0;
    {
      // rjf: set up
      if(lane_idx() == 0)
      {
        unit_headers = push_array(scratch.arena, DW2_UnitHeader, unit_count);
        unit_info_tag_ranges = push_array(scratch.arena, Rng1U64, unit_count);
      }
      lane_sync_u64(&unit_headers, 0);
      lane_sync_u64(&unit_info_tag_ranges, 0);
      
      // rjf: parse all unit headers
      String8 data = raw->sec[DW_SectionKind_Info].data;
      Rng1U64 range = lane_range(unit_count);
      for EachInRange(idx, range)
      {
        Rng1U64 unit_info_range = unit_info_ranges->v[idx];
        U64 bytes_read = dw2_read_unit_header(str8_substr(data, unit_info_range), 0, &unit_headers[idx]);
        unit_info_tag_ranges[idx] = r1u64(unit_info_range.min + bytes_read, unit_info_range.max);
      }
    }
    lane_sync();
    Rng1U64Array unit_info_tag_ranges_array = {unit_info_tag_ranges, unit_count};
    
    //- rjf: build all abbreviation maps, build (unit -> abbrev map)
    DW2_UnitAbbrevMapMap unit_abbrev_map_map = dw2_unit_abbrev_map_map_from_data(scratch.arena, raw->sec[DW_SectionKind_Abbrev].data, unit_headers, unit_count);
    
    //- rjf: parse all unit offsets tables
    DW2_OffsetTableSet *offset_tables = 0;
    if(lane_idx() == 0)
    {
      offset_tables = push_array(scratch.arena, DW2_OffsetTableSet, 1);
      offset_tables[0] = dw2_offset_table_set_from_raw(scratch.arena, raw);
    }
    lane_sync_u64(&offset_tables, 0);
    
    //- rjf: build per-unit parsing contexts
    DW2_ParseCtx *unit_parse_ctxs = 0;
    {
      if(lane_idx() == 0)
      {
        unit_parse_ctxs = push_array(scratch.arena, DW2_ParseCtx, unit_count);
      }
      lane_sync_u64(&unit_parse_ctxs, 0);
      Rng1U64 range = lane_range(unit_count);
      for EachInRange(unit_idx, range)
      {
        DW2_UnitHeader *hdr = &unit_headers[unit_idx];
        DW2_ParseCtx *ctx = &unit_parse_ctxs[unit_idx];
        
        // rjf: equip initial bootstrapping info
        dw2_parse_ctx_equip_unit_header(ctx, hdr, unit_info_ranges->v[unit_idx].min);
        dw2_parse_ctx_equip_unit_abbrev_map(ctx, &unit_abbrev_map_map, unit_idx);
        
        // rjf: do bootstrapping parse of root tag
        DW2_Tag root_tag_bootstrap = {0};
        dw2_read_tag(scratch.arena, raw, ctx, raw->sec[DW_SectionKind_Info].data, unit_info_tag_ranges[unit_idx].min, &root_tag_bootstrap);
        
        // rjf: equip info extracted from bootstrapping parse
        dw2_parse_ctx_equip_unit_root_tag(ctx, &root_tag_bootstrap, offset_tables);
        
        // rjf: do non-bootstrapping parse
        DW2_Tag root_tag = {0};
        dw2_read_tag(scratch.arena, raw, ctx, raw->sec[DW_SectionKind_Info].data, unit_info_tag_ranges[unit_idx].min, &root_tag);
        
        // rjf: equip info extracted from non-bootstrapping parse
        dw2_parse_ctx_equip_unit_root_tag(ctx, &root_tag, offset_tables);
      }
    }
    lane_sync();
    
    //- rjf: dump all unit tag trees
    String8List *unit_dumps = 0;
    if(lane_idx() == 0)
    {
      unit_dumps = push_array(scratch.arena, String8List, unit_count);
    }
    lane_sync_u64(&unit_dumps, 0);
    U64 unit_take_idx_ = 0;
    U64 *unit_take_idx_ptr = &unit_take_idx_;
    lane_sync_u64(&unit_take_idx_ptr, 0);
    String8List *strings_restore = strings;
    for(;;)
    {
      //- rjf: take unit
      U64 unit_idx = ins_atomic_u64_inc_eval(unit_take_idx_ptr)-1;
      if(unit_idx >= unit_count)
      {
        break;
      }
      
      //- rjf: unpack unit
      strings = &unit_dumps[unit_idx];
      DW2_ParseCtx *ctx = &unit_parse_ctxs[unit_idx];
      
      //- rjf: dump
      dumpf(" unit: // unit[%I64u], .debug_info@0x%I64x\n", unit_idx, ctx->unit_base_info_off);
      dumpf(" {\n");
      dumpf("  version:    %u\n", ctx->version);
      dumpf("  format:     %S\n", dw_string_from_format(ctx->format));
      dumpf("  abbrev_off: 0x%I64x\n", unit_headers[unit_idx].abbrev_off);
      dumpf("  addr_size:  %I64u\n", ctx->addr_size);
      dumpf("  kind:       %S\n", dw_string_from_comp_unit_kind(unit_headers[unit_idx].kind));
      dumpf("  dwo_id:     0x%I64x\n", unit_headers[unit_idx].dwo_id);
      dumpf("  tags:\n  {\n");
      {
        U64 depth = 0;
        for(U64 off = unit_info_tag_ranges[unit_idx].min; off < unit_info_tag_ranges[unit_idx].max;)
        {
          U64 start_off = off;
          Temp tag_scratch = scratch_begin(&scratch.arena, 1);
          
          // rjf: parse next tag
          DW2_Tag tag = {0};
          off += dw2_read_tag(tag_scratch.arena, raw, ctx, raw->sec[DW_SectionKind_Info].data, off, &tag);
          
          // rjf: non-zero tag -> dump
          if(tag.kind != DW_TagKind_Null)
          {
            dumpf("   %.*s%S: // .debug_info@0x%I64x\n", depth, indent.str, dw_string_from_tag_kind(ctx->version, ctx->exts, tag.kind), start_off);
            dumpf("   %.*s{\n", depth, indent.str);
            for EachNode(n, DW2_AttribNode, tag.attribs.first)
            {
              DW2_Attrib *attrib = &n->v;
              String8 kind_string = dw_string_from_attrib_kind(ctx->version, ctx->exts, attrib->attrib_kind);
              String8 form_kind_string = dw_string_from_form_kind(ctx->version, ctx->exts, attrib->val.kind);
              String8 value_string = {0};
              DW_AttribClass value_class = dw_attrib_class_from_form_kind(ctx->version, ctx->exts, attrib->val.kind);
              switch(value_class)
              {
                default:
                {
                  value_string = str8f(tag_scratch.arena, "0x%I64x", attrib->val.u128.u64[0]);
                }break;
                case DW_AttribClass_Address:
                {
                  value_string = str8f(tag_scratch.arena, "0x%I64x", attrib->val.addr);
                }break;
                case DW_AttribClass_String:
                {
                  value_string = str8f(tag_scratch.arena, "\"%S\"", attrib->val.string);
                }break;
                case DW_AttribClass_Flag:
                {
                  value_string = attrib->val.u128.u64[0] ? s("true") : s("false");
                }break;
                case DW_AttribClass_Block:
                {
                  String8 block = attrib->val.string;
                  String8List block_strs = numeric_str8_list_from_data(tag_scratch.arena, 16, block, 1);
                  value_string = str8_list_join(tag_scratch.arena, &block_strs, &(StringJoin){.pre = s("{ "), .sep = s(", "), .post = s(" }")});
                }break;
                case DW_AttribClass_ExprLoc:
                {
                  String8 expr = attrib->val.string;
                  String8List expr_strs = dw_dump_list_from_expr(tag_scratch.arena, expr, arch, ctx);
                  value_string = str8_list_join(tag_scratch.arena, &expr_strs, &(StringJoin){.pre = s("{ "), .sep = s(", "), .post = s(" }")});
                }break;
              }
              dumpf("   %.*s %S: %S // %S\n", depth, indent.str, kind_string, value_string, form_kind_string);
            }
          }
          
          // rjf: push/pop
          if(tag.has_children)
          {
            depth += 1;
          }
          else if(tag.kind == DW_TagKind_Null)
          {
            depth -= 1;
          }
          
          // rjf: if tag does not have children -> end info
          if(!tag.has_children)
          {
            dumpf("   %.*s}\n", depth, indent.str);
          }
          
          scratch_end(tag_scratch);
          if(off == start_off)
          {
            break;
          }
        }
      }
      dumpf("  }\n");
      dumpf(" }\n");
    }
    strings = strings_restore;
    
    //- rjf: join unit dumps in order
    if(lane_idx() == 0)
    {
      dumpf("\nunits:\n{\n");
      for EachIndex(unit_idx, unit_count)
      {
        str8_list_concat_in_place(strings, &unit_dumps[unit_idx]);
      }
      dumpf("}\n");
    }
    lane_sync();
  }
  
  //////////////////////////////
  //- rjf: .debug_line
  //
  DumpSubset(Line)
  {
    // TODO(rjf)
  }
  
  //////////////////////////////
  //- rjf: .debug_str
  //
  DumpSubset(Str)
  {
    if(lane_idx() == 0)
    {
      dumpf("\nstrings:\n{\n");
      String8 data = raw->sec[DW_SectionKind_Str].data;
      U64 idx = 0;
      for(U64 off = 0; off < data.size; idx += 1)
      {
        U64 start_off = off;
        String8 string = {0};
        off += str8_deserial_read_cstr(data, off, &string);
        dumpf(" {\"%S\"} // string[%I64u], .debug_str@0x%I64x\n", string, idx, start_off);
        if(off == start_off)
        {
          break;
        }
      }
      dumpf("}\n");
    }
  }
  
  //////////////////////////////
  //- rjf: .debug_frame
  //
#if 0
  DumpSubset(Frame)
  {
    // TODO(rjf)
  }
#endif
  
  //////////////////////////////
  //- rjf: .debug_loc
  //
#if 0
  DumpSubset(Loc)
  {
    // TODO(rjf)
  }
#endif
  
  //////////////////////////////
  //- rjf: join results
  //
  String8List *result_strings_ptr = push_array(scratch.arena, String8List, 1);
  lane_sync();
  if(lane_idx() == 0)
  {
    for EachNode(n, DumpSubsetOutputNode, first_output_node)
    {
      String8List subset_strings = {0};
      for EachIndex(idx, lane_count())
      {
        str8_list_concat_in_place(&subset_strings, &n->lane_strings[idx]);
      }
      if(subset_strings.total_size != 0)
      {
        str8_list_pushf(arena, result_strings_ptr, "////////////////////////////////\n//~ %S\n", dw_regular_name_from_section_kind(n->section_kind));
        str8_list_concat_in_place(result_strings_ptr, &subset_strings);
        str8_list_pushf(arena, result_strings_ptr, "\n");
      }
    }
  }
  lane_sync();
  lane_sync_u64(&result_strings_ptr, 0);
  String8List result_strings = *result_strings_ptr;
  lane_sync();
  
#undef DumpSubsetKind
#undef DumpSubset
#undef dump
#undef dumpf
  scratch_end(scratch);
  ProfEnd();
  return result_strings;
}
