// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal B32
dw_is_dwarf_present_coff_section_table(String8 string_table, U64 section_count, COFF_SectionHeader *section_table)
{
  B32 is_dwarf_present = 0;
  for EachIndex(idx, section_count)
  {
    COFF_SectionHeader *header = &section_table[idx];
    String8 name = coff_name_from_section_header(string_table, header);
    DW_SectionKind s = dw_section_kind_from_string(name);
    is_dwarf_present = s != DW_SectionKind_Null;
    if(is_dwarf_present)
    {
      break;
    }
  }
  return is_dwarf_present;
}

internal DW_Raw
dw_raw_from_coff_section_table(Arena *arena, String8 raw_image, String8 string_table, U64 section_count, COFF_SectionHeader *section_table)
{
  DW_Raw input = {0};
  B32 sect_status[ArrayCount(input.sec)] = {0};
  for EachIndex(idx, section_count)
  {
    COFF_SectionHeader *header = &section_table[idx];
    Rng1U64 raw_data_range = rng_1u64(header->foff, header->vsize ? header->foff + header->vsize : header->foff + header->fsize);
    String8 name = coff_name_from_section_header(string_table, header);
    DW_SectionKind s = dw_section_kind_from_string(name);
    if(s != DW_SectionKind_Null)
    {
      if(!sect_status[s])
      {
        sect_status[s] = 1;
        DW_Section *d = &input.sec[s];
        d->name = push_str8_copy(arena, name);
        d->data = str8_substr(raw_image, raw_data_range);
      }
    }
  }
  return input;
}
