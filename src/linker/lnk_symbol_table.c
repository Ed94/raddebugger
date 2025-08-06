// Copyright (c) 2025 Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

internal LNK_Symbol *
lnk_make_defined_symbol(Arena *arena, String8 name, struct LNK_Obj *obj, U32 symbol_idx)
{
  LNK_Symbol *symbol = push_array(arena, LNK_Symbol, 1);
  symbol->name                 = name;
  symbol->u.defined.obj        = obj;
  symbol->u.defined.symbol_idx = symbol_idx;
  return symbol;
}

internal LNK_Symbol *
lnk_make_lib_symbol(Arena *arena, String8 name, struct LNK_Lib *lib, U64 member_offset)
{
  LNK_Symbol *symbol = push_array(arena, LNK_Symbol, 1);
  symbol->name                = name;
  symbol->u.lib.lib           = lib;
  symbol->u.lib.member_offset = member_offset;
  return symbol;
}

internal LNK_Symbol *
lnk_make_undefined_symbol(Arena *arena, String8 name, struct LNK_Obj *obj)
{
  LNK_Symbol *symbol = push_array(arena, LNK_Symbol, 1);
  symbol->name        = name;
  symbol->u.undef.obj = obj;
  return symbol;
}

internal B32
lnk_symbol_defined_is_before(void *raw_a, void *raw_b)
{
  LNK_Symbol *a = raw_a, *b = raw_b;
  return a->u.defined.obj->input_idx < b->u.defined.obj->input_idx;
}

internal B32
lnk_symbol_lib_is_before(void *raw_a, void *raw_b)
{
  LNK_Symbol *a = raw_a, *b = raw_b;
  return a->u.lib.lib->input_idx < b->u.lib.lib->input_idx;
}

internal void
lnk_symbol_list_push_node(LNK_SymbolList *list, LNK_SymbolNode *node)
{
  SLLQueuePush(list->first, list->last, node);
  list->count += 1;
}

internal LNK_SymbolNode *
lnk_symbol_list_push(Arena *arena, LNK_SymbolList *list, LNK_Symbol *symbol)
{
  LNK_SymbolNode *node = push_array(arena, LNK_SymbolNode, 1);
  node->data           = symbol;
  lnk_symbol_list_push_node(list, node);
  return node;
}

internal void
lnk_symbol_list_concat_in_place(LNK_SymbolList *list, LNK_SymbolList *to_concat)
{
  SLLConcatInPlace(list, to_concat);
}

internal void
lnk_symbol_concat_in_place_array(LNK_SymbolList *list, LNK_SymbolList *to_concat, U64 to_concat_count)
{
  SLLConcatInPlaceArray(list, to_concat, to_concat_count);
}

internal LNK_SymbolList
lnk_symbol_list_from_array(Arena *arena, LNK_SymbolArray arr)
{
  LNK_SymbolList list = {0};
  LNK_SymbolNode *node_arr = push_array_no_zero(arena, LNK_SymbolNode, arr.count);
  for (U64 i = 0; i < arr.count; i += 1) {
    LNK_SymbolNode *node = &node_arr[i];
    node->next           = 0;
    node->data           = &arr.v[i];
    lnk_symbol_list_push_node(&list, node);
  }
  return list;
}

internal LNK_SymbolNodeArray
lnk_symbol_node_array_from_list(Arena *arena, LNK_SymbolList list)
{
  LNK_SymbolNodeArray result = {0};
  result.count               = 0;
  result.v                   = push_array_no_zero(arena, LNK_SymbolNode *, list.count);
  for (LNK_SymbolNode *i = list.first; i != 0; i = i->next, ++result.count) {
    result.v[result.count] = i;
  }
  return result;
}

internal LNK_SymbolArray
lnk_symbol_array_from_list(Arena *arena, LNK_SymbolList list)
{
  LNK_SymbolArray arr = {0};
  arr.count           = 0;
  arr.v               = push_array_no_zero(arena, LNK_Symbol, list.count);
  for (LNK_SymbolNode *node = list.first; node != 0; node = node->next) {
    arr.v[arr.count++] = *node->data;
  }
  return arr;
}

internal LNK_SymbolHashTrie *
lnk_symbol_hash_trie_chunk_list_push(Arena *arena, LNK_SymbolHashTrieChunkList *list, U64 cap)
{
  if (list->last == 0 || list->last->count >= list->last->cap) {
    LNK_SymbolHashTrieChunk *chunk = push_array(arena, LNK_SymbolHashTrieChunk, 1);
    chunk->cap                     = cap;
    chunk->v                       = push_array_no_zero(arena, LNK_SymbolHashTrie, cap);
    SLLQueuePush(list->first, list->last, chunk);
    ++list->count;
  }

  LNK_SymbolHashTrie *result = &list->last->v[list->last->count++];
  return result;
}

internal void
lnk_error_multiply_defined_symbol(LNK_Symbol *dst, LNK_Symbol *src)
{
  lnk_error_obj(LNK_Error_MultiplyDefinedSymbol, dst->u.defined.obj, "symbol \"%S\" (No. %#x) is multiply defined in %S (No. %#x)", dst->name, dst->u.defined.symbol_idx, src->u.defined.obj->path, src->u.defined.symbol_idx);
}

internal B32
lnk_can_replace_symbol(LNK_SymbolScope scope, LNK_Symbol *dst, LNK_Symbol *src)
{
  B32 can_replace = 0;
  switch (scope) {
  case LNK_SymbolScope_Defined: {
    LNK_Obj                    *dst_obj    = dst->u.defined.obj;
    LNK_Obj                    *src_obj    = src->u.defined.obj;
    COFF_ParsedSymbol           dst_parsed = lnk_parsed_symbol_from_coff_symbol_idx(dst->u.defined.obj, dst->u.defined.symbol_idx);
    COFF_ParsedSymbol           src_parsed = lnk_parsed_symbol_from_coff_symbol_idx(src->u.defined.obj, src->u.defined.symbol_idx);
    COFF_SymbolValueInterpType  dst_interp = coff_interp_from_parsed_symbol(dst_parsed);
    COFF_SymbolValueInterpType  src_interp = coff_interp_from_parsed_symbol(src_parsed);

    // regular vs abs
    if (dst_interp == COFF_SymbolValueInterp_Regular && src_interp == COFF_SymbolValueInterp_Abs) {
      lnk_error_multiply_defined_symbol(dst, src);
    }
    // abs vs regular
    else if (dst_interp == COFF_SymbolValueInterp_Abs && src_interp == COFF_SymbolValueInterp_Regular) {
      lnk_error_multiply_defined_symbol(dst, src);
    }
    // abs vs common
    else if (dst_interp == COFF_SymbolValueInterp_Abs && src_interp == COFF_SymbolValueInterp_Common) {
      if (lnk_symbol_defined_is_before(dst, src)) {
        can_replace = 1;
      } else {
        lnk_error_multiply_defined_symbol(dst, src);
      }
    }
    // common vs abs
    else if (dst_interp == COFF_SymbolValueInterp_Common && src_interp == COFF_SymbolValueInterp_Abs) {
      if (lnk_symbol_defined_is_before(dst, src)) {
        lnk_error_multiply_defined_symbol(dst, src);
      }
    }
    // abs vs abs
    else if (dst_interp == COFF_SymbolValueInterp_Abs && src_interp == COFF_SymbolValueInterp_Abs) {
      lnk_error_multiply_defined_symbol(dst, src);
    }
    // weak vs weak
    else if (dst_interp == COFF_SymbolValueInterp_Weak && src_interp == COFF_SymbolValueInterp_Weak) {
      can_replace = lnk_symbol_defined_is_before(src, dst);
    }
    // weak vs regular/abs/common
    else if (dst_interp == COFF_SymbolValueInterp_Weak && (src_interp == COFF_SymbolValueInterp_Regular || src_interp == COFF_SymbolValueInterp_Abs || src_interp == COFF_SymbolValueInterp_Common)) {
      can_replace = 1;
    }
    // regular/abs/common vs weak
    else if ((dst_interp == COFF_SymbolValueInterp_Regular || dst_interp == COFF_SymbolValueInterp_Abs || dst_interp == COFF_SymbolValueInterp_Common) && src_interp == COFF_SymbolValueInterp_Weak) {
      can_replace = 0;
    }
    // regular/common vs regular/common
    else if ((dst_interp == COFF_SymbolValueInterp_Regular || dst_interp == COFF_SymbolValueInterp_Common) && (src_interp == COFF_SymbolValueInterp_Regular || src_interp == COFF_SymbolValueInterp_Common)) {
      // parse dst symbol properties
      B32                   dst_is_comdat = 0;
      COFF_ComdatSelectType dst_select;
      U32                   dst_section_length;
      U32                   dst_check_sum;
      if (dst_interp == COFF_SymbolValueInterp_Regular) {
        dst_is_comdat = lnk_try_comdat_props_from_section_number(dst->u.defined.obj, dst_parsed.section_number, &dst_select, 0, &dst_section_length, &dst_check_sum);
      } else if (dst_interp == COFF_SymbolValueInterp_Common) {
        dst_select         = COFF_ComdatSelect_Largest;
        dst_section_length = dst_parsed.value;
        dst_check_sum      = 0;
        dst_is_comdat      = 1;
      }

      // parse src symbol properties
      B32                   src_is_comdat = 0;
      COFF_ComdatSelectType src_select;
      U32                   src_section_length, src_checks;
      U32                   src_check_sum;
      if (src_interp == COFF_SymbolValueInterp_Regular) {
        src_is_comdat = lnk_try_comdat_props_from_section_number(src->u.defined.obj, src_parsed.section_number, &src_select, 0, &src_section_length, &src_check_sum);
      } else if (src_interp == COFF_SymbolValueInterp_Common) {
        src_select         = COFF_ComdatSelect_Largest;
        src_section_length = src_parsed.value;
        src_check_sum      = 0;
        src_is_comdat      = 1;
      }

      // regular non-comdat vs communal
      if (dst_interp == COFF_SymbolValueInterp_Regular && !dst_is_comdat && src_interp == COFF_SymbolValueInterp_Common) {
        can_replace = 0;
      }
      // communal vs regular non-comdat
      else if (dst_interp == COFF_SymbolValueInterp_Common && src_interp == COFF_SymbolValueInterp_Regular && !src_is_comdat) {
        can_replace = 1;
      }
      // handle COMDATs
      else if (dst_is_comdat && src_is_comdat) {
        if ((src_select == COFF_ComdatSelect_Any && dst_select == COFF_ComdatSelect_Largest)) {
          src_select = COFF_ComdatSelect_Largest;
        }
        if (src_select == COFF_ComdatSelect_Largest && dst_select == COFF_ComdatSelect_Any) {
          dst_select = COFF_ComdatSelect_Largest;
        }

        if (src_select == dst_select) {
          switch (src_select) {
          case COFF_ComdatSelect_Null:
          case COFF_ComdatSelect_Any: {
            if (src_section_length == dst_section_length) {
              can_replace = lnk_obj_is_before(src_obj, dst_obj);
            } else {
              // both COMDATs are valid but to get smaller exe pick smallest
              can_replace = src_section_length < dst_section_length;
            }
          } break;
          case COFF_ComdatSelect_NoDuplicates: {
            lnk_error_multiply_defined_symbol(dst, src);
          } break;
          case COFF_ComdatSelect_SameSize: {
            if (dst_section_length == src_section_length) {
              can_replace = lnk_obj_is_before(src_obj, dst_obj);
            } else {
              lnk_error_multiply_defined_symbol(dst, src);
            }
          } break;
          case COFF_ComdatSelect_ExactMatch: {
            COFF_SectionHeader *dst_sect_header = lnk_coff_section_header_from_section_number(dst_obj, dst_parsed.section_number);
            COFF_SectionHeader *src_sect_header = lnk_coff_section_header_from_section_number(src_obj, src_parsed.section_number);
            String8             dst_data        = str8_substr(dst_obj->data, rng_1u64(dst_sect_header->foff, dst_sect_header->foff + dst_sect_header->fsize));
            String8             src_data        = str8_substr(src_obj->data, rng_1u64(src_sect_header->foff, src_sect_header->foff + src_sect_header->fsize));
            B32                 is_exact_match  = 0;
            if (dst_check_sum != 0 && src_check_sum != 0) {
              is_exact_match = dst_check_sum == src_check_sum && str8_match(dst_data, src_data, 0);
            } else {
              is_exact_match = str8_match(dst_data, src_data, 0);
            }

            if (is_exact_match) {
              can_replace = lnk_obj_is_before(src_obj, dst_obj);
            } else {
              lnk_error_multiply_defined_symbol(dst, src);
            }
          } break;
          case COFF_ComdatSelect_Largest: {
            if (dst_section_length == src_section_length) {
              can_replace = lnk_obj_is_before(src_obj, dst_obj);
            } else {
              can_replace = dst_section_length < src_section_length;
            }
          } break;
          case COFF_ComdatSelect_Associative: { /* ignore */ } break;
          default: { InvalidPath; } break;
          }
        } else {
          lnk_error_obj(LNK_Warning_UnresolvedComdat, src_obj,
                        "%S: COMDAT selection conflict detected, current selection %S, leader selection %S from %S", 
                        src->name, coff_string_from_comdat_select_type(src_select), coff_string_from_comdat_select_type(dst_select), dst_obj);
        }
      } else {
        lnk_error_multiply_defined_symbol(dst, src);
      }
    } else {
      lnk_error(LNK_Error_InvalidPath, "unable to find a suitable replacement logic for symbol combination");
    }
  } break;
  case LNK_SymbolScope_Lib: {
    // link.exe picks symbol from lib that is discovered first
    can_replace = lnk_symbol_lib_is_before(src, dst);
  } break;
  default: { InvalidPath; }
  }
  return can_replace;
}

internal void
lnk_on_symbol_replace(LNK_SymbolScope scope, LNK_Symbol *dst, LNK_Symbol *src)
{
  switch (scope) {
  case LNK_SymbolScope_Defined: {
    COFF_ParsedSymbol          dst_parsed = lnk_parsed_symbol_from_coff_symbol_idx(dst->u.defined.obj, dst->u.defined.symbol_idx);
    COFF_SymbolValueInterpType dst_interp = coff_interp_from_parsed_symbol(dst_parsed);
    if (dst_interp == COFF_SymbolValueInterp_Regular) {
      // remove replaced section from the output
      COFF_SectionHeader *dst_sect = lnk_coff_section_header_from_section_number(dst->u.defined.obj, dst_parsed.section_number);
      dst_sect->flags |= COFF_SectionFlag_LnkRemove;

      // remove associated sections from the output
      for (U32Node *associated_section = dst->u.defined.obj->associated_sections[dst_parsed.section_number];
           associated_section != 0;
           associated_section = associated_section->next) {
        COFF_SectionHeader *section_header = lnk_coff_section_header_from_section_number(dst->u.defined.obj, associated_section->data);
        section_header->flags |= COFF_SectionFlag_LnkRemove;
      }
    }

    // make sure leader section is not removed from the output
#if BUILD_DEBUG
    {
      COFF_ParsedSymbol          src_parsed = lnk_parsed_symbol_from_coff_symbol_idx(src->u.defined.obj, src->u.defined.symbol_idx);
      COFF_SymbolValueInterpType src_interp = coff_interp_from_parsed_symbol(src_parsed);
      if (src_interp == COFF_SymbolValueInterp_Regular) {
        COFF_SectionHeader *src_sect = lnk_coff_section_header_from_section_number(src->u.defined.obj, src_parsed.section_number);
        AssertAlways(~src_sect->flags & COFF_SectionFlag_LnkRemove);
      }
    }
#endif
  } break;
  case LNK_SymbolScope_Lib: {
    // nothing to replace
  } break;
  default: { InvalidPath; }
  }
}

internal void
lnk_symbol_hash_trie_insert_or_replace(Arena                        *arena,
                                       LNK_SymbolHashTrieChunkList  *chunks,
                                       LNK_SymbolHashTrie          **trie,
                                       U64                           hash,
                                       LNK_SymbolScope               scope,
                                       LNK_Symbol                   *symbol)
{
  LNK_SymbolHashTrie **curr_trie_ptr = trie;
  for (U64 h = hash; ; h <<= 2) {
    // load current pointer
    LNK_SymbolHashTrie *curr_trie = ins_atomic_ptr_eval(curr_trie_ptr);

    if (curr_trie == 0) {
      // init node
      LNK_SymbolHashTrie *new_trie = lnk_symbol_hash_trie_chunk_list_push(arena, chunks, 512);
      new_trie->name               = &symbol->name;
      new_trie->symbol             = symbol;
      MemoryZeroArray(new_trie->child);

      // try to insert new node
      LNK_SymbolHashTrie *cmp = ins_atomic_ptr_eval_cond_assign(curr_trie_ptr, new_trie, curr_trie);

      // was symbol inserted?
      if (cmp == curr_trie) {
        break;
      }

      // rollback chunk list push
      --chunks->last->count;

      // retry insert with trie node from another thread
      curr_trie = cmp;
    }

    // load current symbol
    String8 *curr_name = ins_atomic_ptr_eval(&curr_trie->name);

    if (curr_name && str8_match(*curr_name, symbol->name, 0)) {
      for (LNK_Symbol *src = symbol;;) {
        // try replacing current symbol with zero, otherwise loop back and retry
        LNK_Symbol *leader = ins_atomic_ptr_eval_assign(&curr_trie->symbol, 0);

        // apply replacement
        if (leader) {
          if (lnk_can_replace_symbol(scope, leader, src)) {
            // discard leader
            lnk_on_symbol_replace(scope, leader, src);
            leader = src;
          } else {
            // discard source
            lnk_on_symbol_replace(scope, src, leader);
            src = leader;
          }
        } else {
          leader = src;
        }

        // try replacing symbol, if another thread has already taken the slot, rerun replacement loop again
        LNK_Symbol *was_replaced = ins_atomic_ptr_eval_cond_assign(&curr_trie->symbol, leader, 0);

        // symbol replaced, exit
        if (was_replaced == 0) {
          goto exit;
        }
      }
    }

    // pick child and descend
    curr_trie_ptr = curr_trie->child + (h >> 62);
  }
  exit:;
}

internal LNK_SymbolHashTrie *
lnk_symbol_hash_trie_search(LNK_SymbolHashTrie *trie, U64 hash, String8 name)
{
  LNK_SymbolHashTrie  *result   = 0;
  LNK_SymbolHashTrie **curr_ptr = &trie;
  for (U64 h = hash; ; h <<= 2) {
    LNK_SymbolHashTrie *curr = ins_atomic_ptr_eval(curr_ptr);
    if (curr == 0) {
      break;
    }
    if (curr->name && str8_match(*curr->name, name, 0)) {
      result = curr;
      break;
    }
    curr_ptr = curr->child + (h >> 62);
  }
  return result;
}

internal void
lnk_symbol_hash_trie_remove(LNK_SymbolHashTrie *trie)
{
  ins_atomic_ptr_eval_assign(&trie->name,   0);
  ins_atomic_ptr_eval_assign(&trie->symbol, 0);
}

internal U64
lnk_symbol_hash(String8 string)
{
  XXH3_state_t hasher; XXH3_64bits_reset(&hasher);
  XXH3_64bits_update(&hasher, &string.size, sizeof(string.size));
  XXH3_64bits_update(&hasher, string.str, string.size);
  XXH64_hash_t result = XXH3_64bits_digest(&hasher);
  return result;
}

internal LNK_SymbolTable *
lnk_symbol_table_init(TP_Arena *arena)
{
  LNK_SymbolTable *symtab = push_array(arena->v[0], LNK_SymbolTable, 1);
  symtab->arena           = arena;
  for (U64 i = 0; i < LNK_SymbolScope_Count; ++i) {
    symtab->chunk_lists[i] = push_array(arena->v[0], LNK_SymbolHashTrieChunkList, arena->count);
  }
  return symtab;
}

internal void
lnk_symbol_table_push_(LNK_SymbolTable *symtab, Arena *arena, U64 worker_id, LNK_SymbolScope scope, LNK_Symbol *symbol)
{
  U64 hash = lnk_symbol_hash(symbol->name);
  lnk_symbol_hash_trie_insert_or_replace(arena, &symtab->chunk_lists[scope][worker_id], &symtab->scopes[scope], hash, scope, symbol);
}

internal void
lnk_symbol_table_push(LNK_SymbolTable *symtab, LNK_SymbolScope scope, LNK_Symbol *symbol)
{
  lnk_symbol_table_push_(symtab, symtab->arena->v[0], 0, scope, symbol);
}

internal LNK_SymbolHashTrie *
lnk_symbol_table_search_(LNK_SymbolTable *symtab, LNK_SymbolScope scope, String8 name)
{
  U64 hash = lnk_symbol_hash(name);
  return lnk_symbol_hash_trie_search(symtab->scopes[scope], hash, name);
}

internal LNK_Symbol *
lnk_symbol_table_search(LNK_SymbolTable *symtab, LNK_SymbolScope scope, String8 name)
{
  LNK_SymbolHashTrie *trie = lnk_symbol_table_search_(symtab, scope, name);
  return trie ? trie->symbol : 0;
}

internal LNK_Symbol *
lnk_symbol_table_searchf(LNK_SymbolTable *symtab, LNK_SymbolScope scope, char *fmt, ...)
{
  Temp scratch = scratch_begin(0, 0);
 
  va_list args; va_start(args, fmt);
  String8 name = push_str8fv(scratch.arena, fmt, args);
  va_end(args);
  
  LNK_Symbol *symbol = lnk_symbol_table_search(symtab, scope, name);

  scratch_end(scratch);
  return symbol;
}

internal
THREAD_POOL_TASK_FUNC(lnk_check_anti_dependecy_task)
{
  LNK_FinalizeWeakSymbolsTask *task   = raw_task;
  LNK_SymbolTable             *symtab = task->symtab;
  LNK_SymbolHashTrieChunk     *chunk  = task->chunks[task_id];

  for EachIndex(i, chunk->count) {
    LNK_Symbol                 *symbol        = chunk->v[i].symbol;
    COFF_ParsedSymbol           symbol_parsed = lnk_parsed_symbol_from_defined(symbol);
    COFF_SymbolValueInterpType  symbol_interp = coff_interp_from_parsed_symbol(symbol_parsed);
    if (symbol_interp == COFF_SymbolValueInterp_Weak) {
      COFF_SymbolWeakExt *weak_ext = coff_parse_weak_tag(symbol_parsed, symbol->u.defined.obj->header.is_big_obj);
      if (weak_ext->characteristics == COFF_WeakExt_AntiDependency) {
        COFF_ParsedSymbol          default_symbol_parsed = lnk_parsed_symbol_from_coff_symbol_idx(symbol->u.defined.obj, weak_ext->tag_index);
        COFF_SymbolValueInterpType default_symbol_interp = coff_interp_from_parsed_symbol(default_symbol_parsed);

        COFF_SymbolValueInterpType actual_default_symbol_interp = default_symbol_interp;
        if (default_symbol_interp == COFF_SymbolValueInterp_Undefined) {
          LNK_Symbol *actual_default_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Defined, default_symbol_parsed.name);
          if (actual_default_symbol) {
            COFF_ParsedSymbol actual_default_symbol_parsed = lnk_parsed_symbol_from_defined(actual_default_symbol);
            actual_default_symbol_interp = coff_interp_from_parsed_symbol(actual_default_symbol_parsed);
          }
        }

        if (actual_default_symbol_interp == COFF_SymbolValueInterp_Weak) {
          LNK_SymbolNode *symbol_n = push_array(arena, LNK_SymbolNode, 1);
          symbol_n->data = symbol;
          lnk_symbol_list_push_node(&task->anti_dependency_symbols[task_id], symbol_n);
        }
      }
    }
  }
}

internal
THREAD_POOL_TASK_FUNC(lnk_finalize_weak_symbols_task)
{
  Temp scratch = scratch_begin(&arena,1);

  LNK_FinalizeWeakSymbolsTask *task   = raw_task;
  LNK_SymbolTable             *symtab = task->symtab;
  LNK_SymbolHashTrieChunk     *chunk  = task->chunks[task_id];

  for EachIndex(i, chunk->count) {
    LNK_Symbol                 *symbol        = chunk->v[i].symbol;
    COFF_ParsedSymbol           symbol_parsed = lnk_parsed_symbol_from_defined(symbol);
    COFF_SymbolValueInterpType  symbol_interp = coff_interp_from_parsed_symbol(symbol_parsed);
    if (symbol_interp == COFF_SymbolValueInterp_Weak) {
      struct LookupLocation { struct LookupLocation *next; LNK_SymbolDefined symbol; B32 is_anti_dependency; };
      struct LookupLocation *lookup_first = 0, *lookup_last = 0;

      LNK_SymbolDefined current_symbol = symbol->u.defined;
      for (;;) {
        // guard against self-referencing weak symbols
        struct LookupLocation *was_visited = 0;
        for (struct LookupLocation *l = lookup_first; l != 0; l = l->next) {
          if (MemoryCompare(&l->symbol, &current_symbol, sizeof(LNK_SymbolDefined)) == 0) { was_visited = l; break; }
        }
        if (was_visited) {
          Temp temp = temp_begin(scratch.arena);

          String8List ref_list = {0};
          for (struct LookupLocation *l = lookup_first; l != 0; l = l->next) {
            COFF_ParsedSymbol loc_symbol = lnk_parsed_symbol_from_coff_symbol_idx(l->symbol.obj, l->symbol.symbol_idx);
            str8_list_pushf(temp.arena, &ref_list, "\t%S Symbol %S (No. %#x) =>", l->symbol.obj->path, loc_symbol.name, l->symbol.symbol_idx);
          }
          COFF_ParsedSymbol loc_symbol = lnk_parsed_symbol_from_coff_symbol_idx(lookup_first->symbol.obj, lookup_first->symbol.symbol_idx);
          str8_list_pushf(temp.arena, &ref_list, "\t%S Symbol %S (No. %#x)", lookup_first->symbol.obj->path, loc_symbol.name, lookup_first->symbol.symbol_idx);

          COFF_ParsedSymbol parsed_symbol = lnk_parsed_symbol_from_coff_symbol_idx(symbol->u.defined.obj, symbol->u.defined.symbol_idx);
          String8           loc_string    = str8_list_join(temp.arena, &ref_list, &(StringJoin){ .sep = str8_lit("\n") });
          lnk_error_obj(LNK_Error_WeakCycle, symbol->u.defined.obj, "unable to resolve cyclic symbol %S; ref chain:\n%S", parsed_symbol.name, loc_string);

          MemoryZeroStruct(&current_symbol);

          temp_end(temp);
          break;
        }

        COFF_ParsedSymbol          current_parsed = lnk_parsed_symbol_from_coff_symbol_idx(current_symbol.obj, current_symbol.symbol_idx);
        COFF_SymbolValueInterpType current_interp = coff_interp_symbol(current_parsed.section_number, current_parsed.value, current_parsed.storage_class);
        if (current_interp == COFF_SymbolValueInterp_Weak) {
          // record visited symbol
          struct LookupLocation *loc = push_array(scratch.arena, struct LookupLocation, 1);
          loc->symbol                = current_symbol;
          SLLQueuePush(lookup_first, lookup_last, loc);

          // does weak symbol have a definition?
          LNK_Symbol                 *defn_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Defined, current_parsed.name);
          COFF_ParsedSymbol           defn_parsed = lnk_parsed_symbol_from_coff_symbol_idx(defn_symbol->u.defined.obj, defn_symbol->u.defined.symbol_idx);
          COFF_SymbolValueInterpType  defn_interp = coff_interp_symbol(defn_parsed.section_number, defn_parsed.value, defn_parsed.storage_class);
          if (defn_interp != COFF_SymbolValueInterp_Weak) {
            current_symbol = defn_symbol->u.defined;
            break;
          }

          // no definition fallback to the tag
          COFF_SymbolWeakExt         *weak_ext   = coff_parse_weak_tag(current_parsed, current_symbol.obj->header.is_big_obj);
          COFF_ParsedSymbol           tag_parsed = lnk_parsed_symbol_from_coff_symbol_idx(current_symbol.obj, weak_ext->tag_index);
          COFF_SymbolValueInterpType  tag_interp = coff_interp_symbol(tag_parsed.section_number, tag_parsed.value, tag_parsed.storage_class);
          current_symbol = (LNK_SymbolDefined){ .obj = current_symbol.obj, .symbol_idx = weak_ext->tag_index };
        } else if (current_interp == COFF_SymbolValueInterp_Undefined) {
          LNK_Symbol *defn_symbol = lnk_symbol_table_search(symtab, LNK_SymbolScope_Defined, current_parsed.name);
          if (defn_symbol == 0) {
            MemoryZeroStruct(&current_symbol);
            break;
          }
          current_symbol = defn_symbol->u.defined;
        } else {
          break;
        }
      }

      // replace weak symbol with it's tag
      symbol->u.defined = current_symbol;
    }
  }

  scratch_end(scratch);
}

internal void
lnk_finalize_weak_symbols(TP_Arena *arena, TP_Context *tp, LNK_SymbolTable *symtab)
{
  ProfBeginFunction();
  Temp scratch = scratch_begin(arena->v, arena->count);

  U64 chunks_count = 0;
  for EachIndex(worker_id, tp->worker_count) { chunks_count += symtab->chunk_lists[LNK_SymbolScope_Defined][worker_id].count; }

  LNK_SymbolHashTrieChunk **chunks        = push_array(scratch.arena, LNK_SymbolHashTrieChunk *, chunks_count);
  U64                       chunks_cursor = 0;
  for EachIndex(worker_id, tp->worker_count) {
    for (LNK_SymbolHashTrieChunk *chunk = symtab->chunk_lists[LNK_SymbolScope_Defined][worker_id].first; chunk != 0; chunk = chunk->next) {
      chunks[chunks_cursor++] = chunk;
    }
  }

  LNK_FinalizeWeakSymbolsTask task = { .symtab = symtab, .chunks = chunks };

  {
    TP_Temp temp = tp_temp_begin(arena);
    task.anti_dependency_symbols = push_array(scratch.arena, LNK_SymbolList, tp->worker_count);
    tp_for_parallel(tp, arena, chunks_count, lnk_check_anti_dependecy_task, &task);

    LNK_SymbolList anti_dependency_symbol_list = {0};
    lnk_symbol_concat_in_place_array(&anti_dependency_symbol_list, task.anti_dependency_symbols, tp->worker_count);
    LNK_SymbolArray anti_dependency_symbols = lnk_symbol_array_from_list(scratch.arena, anti_dependency_symbol_list);
    radsort(anti_dependency_symbols.v, anti_dependency_symbols.count, lnk_symbol_defined_is_before);

    for EachIndex(symbol_idx, anti_dependency_symbols.count) {
      LNK_Symbol *s = &anti_dependency_symbols.v[symbol_idx];
      lnk_error_obj(LNK_Error_UnresolvedSymbol, s->u.defined.obj, "unresolved symbol %S", s->name);
    }

    tp_temp_end(temp);
  }

  tp_for_parallel(tp, 0, chunks_count, lnk_finalize_weak_symbols_task, &task);

  scratch_end(scratch);
  ProfEnd();
}

internal ISectOff
lnk_sc_from_symbol(LNK_Symbol *symbol)
{
  COFF_ParsedSymbol parsed_symbol = lnk_parsed_symbol_from_coff_symbol_idx(symbol->u.defined.obj, symbol->u.defined.symbol_idx);

  ISectOff sc = {0};
  sc.isect    = parsed_symbol.section_number;
  sc.off      = parsed_symbol.value;

  return sc;
}

internal U64
lnk_isect_from_symbol(LNK_Symbol *symbol)
{
  return lnk_sc_from_symbol(symbol).isect;
}

internal U64
lnk_sect_off_from_symbol(LNK_Symbol *symbol)
{
  return lnk_sc_from_symbol(symbol).off;
}

internal U64
lnk_virt_off_from_symbol(COFF_SectionHeader **section_table, LNK_Symbol *symbol)
{
  ISectOff sc   = lnk_sc_from_symbol(symbol);
  U64      voff = section_table[sc.isect]->voff + sc.off;
  return voff;
}

internal U64
lnk_file_off_from_symbol(COFF_SectionHeader **section_table, LNK_Symbol *symbol)
{
  ISectOff sc   = lnk_sc_from_symbol(symbol);
  U64      foff = section_table[sc.isect]->foff + sc.off;
  return foff;
}

internal COFF_ParsedSymbol
lnk_parsed_symbol_from_defined(LNK_Symbol *symbol)
{
  return lnk_parsed_symbol_from_coff_symbol_idx(symbol->u.defined.obj, symbol->u.defined.symbol_idx);
}

