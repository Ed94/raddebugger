// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef DBG_INFO2_H
#define DBG_INFO2_H

////////////////////////////////
//~ rjf: Unique Debug Info Key

typedef struct DI2_Key DI2_Key;
struct DI2_Key
{
  U64 u64[2];
};

typedef struct DI2_KeyNode DI2_KeyNode;
struct DI2_KeyNode
{
  DI2_KeyNode *next;
  DI2_Key v;
};

typedef struct DI2_KeyList DI2_KeyList;
struct DI2_KeyList
{
  DI2_KeyNode *first;
  DI2_KeyNode *last;
  U64 count;
};

typedef struct DI2_KeyArray DI2_KeyArray;
struct DI2_KeyArray
{
  DI2_Key *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Debug Info Path / Timestamp => Key Cache Types

typedef struct DI2_KeyPathNode DI2_KeyPathNode;
struct DI2_KeyPathNode
{
  DI2_KeyPathNode *next;
  DI2_KeyPathNode *prev;
  String8 path;
  U64 min_timestamp;
  DI2_Key key;
};

typedef struct DI2_KeySlot DI2_KeySlot;
struct DI2_KeySlot
{
  DI2_KeyPathNode *first;
  DI2_KeyPathNode *last;
};

////////////////////////////////
//~ rjf: Debug Info Cache Types

typedef struct DI2_Node DI2_Node;
struct DI2_Node
{
  // rjf: links
  DI2_Node *next;
  DI2_Node *prev;
  
  // rjf: key
  DI2_Key key;
  
  // rjf: value
  OS_Handle file;
  OS_Handle file_map;
  void *file_base;
  FileProperties file_props;
  Arena *arena;
  RDI_Parsed rdi;
  
  // rjf: metadata
  AccessPt access_pt;
  U64 refcount;
  U64 batch_request_counts[2];
  U64 working_count;
  U64 completion_count;
};

typedef struct DI2_Slot DI2_Slot;
struct DI2_Slot
{
  DI2_Node *first;
  DI2_Node *last;
};

////////////////////////////////
//~ rjf: Requests

typedef struct DI2_Request DI2_Request;
struct DI2_Request
{
  DI2_Key key;
};

typedef struct DI2_RequestNode DI2_RequestNode;
struct DI2_RequestNode
{
  DI2_RequestNode *next;
  DI2_Request v;
};

typedef struct DI2_RequestBatch DI2_RequestBatch;
struct DI2_RequestBatch
{
  Mutex mutex;
  Arena *arena;
  DI2_RequestNode *first;
  DI2_RequestNode *last;
  U64 count;
};

////////////////////////////////
//~ rjf: Load Tasks

typedef enum DI2_LoadTaskStatus
{
  DI2_LoadTaskStatus_Null,
  DI2_LoadTaskStatus_Active,
  DI2_LoadTaskStatus_Done,
}
DI2_LoadTaskStatus;

typedef struct DI2_LoadTask DI2_LoadTask;
struct DI2_LoadTask
{
  DI2_LoadTask *next;
  DI2_LoadTask *prev;
  
  DI2_Key key;
  DI2_LoadTaskStatus status;
  
  B32 og_analyzed;
  B32 og_is_rdi;
  U64 og_size;
  
  B32 rdi_analyzed;
  B32 rdi_is_stale;
  
  U64 thread_count;
  OS_Handle process;
};

////////////////////////////////
//~ rjf: Search Types

typedef struct DI2_SearchItem DI2_SearchItem;
struct DI2_SearchItem
{
  U64 idx;
  DI2_Key key;
  U64 missed_size;
  FuzzyMatchRangeList match_ranges;
};

typedef struct DI2_SearchItemChunk DI2_SearchItemChunk;
struct DI2_SearchItemChunk
{
  DI2_SearchItemChunk *next;
  U64 base_idx;
  DI2_SearchItem *v;
  U64 count;
  U64 cap;
};

typedef struct DI2_SearchItemChunkList DI2_SearchItemChunkList;
struct DI2_SearchItemChunkList
{
  DI2_SearchItemChunk *first;
  DI2_SearchItemChunk *last;
  U64 chunk_count;
  U64 total_count;
};

typedef struct DI2_SearchItemArray DI2_SearchItemArray;
struct DI2_SearchItemArray
{
  DI2_SearchItem *v;
  U64 count;
};

////////////////////////////////
//~ rjf: Shared State

typedef struct DI2_Shared DI2_Shared;
struct DI2_Shared
{
  Arena *arena;
  U64 load_gen;
  
  // rjf: key -> path cache
  U64 key2path_slots_count;
  DI2_KeySlot *key2path_slots;
  StripeArray key2path_stripes;
  
  // rjf: path -> key cache
  U64 path2key_slots_count;
  DI2_KeySlot *path2key_slots;
  StripeArray path2key_stripes;
  
  // rjf: debug info cache
  U64 slots_count;
  DI2_Slot *slots;
  StripeArray stripes;
  
  // rjf: requests
  DI2_RequestBatch req_batches[2]; // [0] -> high priority, [1] -> low priority
  
  // rjf: conversion tasks
  DI2_LoadTask *first_load_task;
  DI2_LoadTask *last_load_task;
  DI2_LoadTask *free_load_task;
  U64 conversion_process_count;
  U64 conversion_thread_count;
  
  // rjf: conversion completion receiving thread
  String8 conversion_completion_signal_semaphore_name;
  Semaphore conversion_completion_signal_semaphore;
  Thread conversion_completion_signal_receiver_thread;
};

////////////////////////////////
//~ rjf: Globals

global DI2_Shared *di2_shared = 0;

////////////////////////////////
//~ rjf: Helpers

internal DI2_Key di2_key_zero(void);
internal B32 di2_key_match(DI2_Key a, DI2_Key b);

////////////////////////////////
//~ rjf: Main Layer Initialization

internal void di2_init(CmdLine *cmdline);

////////////////////////////////
//~ rjf: Path * Timestamp Cache Submission & Lookup

internal DI2_Key di2_key_from_path_timestamp(String8 path, U64 min_timestamp);

////////////////////////////////
//~ rjf: Debug Info Opening / Closing

internal void di2_open(DI2_Key key);
internal void di2_close(DI2_Key key);

////////////////////////////////
//~ rjf: Debug Info Lookups

internal U64 di2_load_gen(void);
internal DI2_KeyArray di2_push_all_loaded_keys(Arena *arena);
internal RDI_Parsed *di2_rdi_from_key(Access *access, DI2_Key key, B32 high_priority, U64 endt_us);

////////////////////////////////
//~ rjf: Asynchronous Tick

internal void di2_async_tick(void);

////////////////////////////////
//~ rjf: Conversion Completion Signal Receiver Thread

internal void di2_signal_completion(void);
internal void di2_conversion_completion_signal_receiver_thread_entry_point(void *p);

////////////////////////////////
//~ rjf: Search Artifact Cache Hooks / Lookups

internal AC_Artifact di2_search_artifact_create(String8 key, U64 gen, U64 *requested_gen, B32 *retry_out);
internal void di2_search_artifact_destroy(AC_Artifact artifact);
internal DI2_SearchItemArray di2_search_item_array_from_target_query(Access *access, RDI_SectionKind target, String8 query, U64 endt_us);

#endif // DBG_INFO2_H
