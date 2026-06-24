// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DWARF_UNWIND_H
#define DWARF_UNWIND_H

typedef enum DW_UnwindRuleCode
{
  DW_UnwindRuleCode_SameVal,
  DW_UnwindRuleCode_Undefined,
  DW_UnwindRuleCode_Off,
  DW_UnwindRuleCode_ValOff,
  DW_UnwindRuleCode_Reg,
  DW_UnwindRuleCode_Expr,
  DW_UnwindRuleCode_ValExpr,
  DW_UnwindRuleCode_Architectural,
}
DW_UnwindRuleCode;

typedef struct DW_UnwindRule DW_UnwindRule;
struct DW_UnwindRule
{
  DW_UnwindRuleCode code;
  DW_RegCode reg_code;
  S64 s64;
  String8 expr;
};

typedef struct DW_CFIRow DW_CFIRow;
struct DW_CFIRow
{
  U64 base_pc_off;
  DW_UnwindRule cfa_rule;
  DW_UnwindRule *reg_rules;
};

typedef struct DW_CFIRowNode DW_CFIRowNode;
struct DW_CFIRowNode
{
  DW_CFIRowNode *next;
  DW_CFIRow v;
};

#endif // DWARF_UNWIND_H
