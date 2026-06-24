// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef WIN32_SYMBOL_SERVER_H
#define WIN32_SYMBOL_SERVER_H

typedef enum W32_SMSV_TaskStatus
{
  W32_SMSV_TaskStatus_Null,
  W32_SMSV_TaskStatus_Requested,
  W32_SMSV_TaskStatus_DoneDownloading,
  W32_SMSV_TaskStatus_Failed,
}
W32_SMSV_TaskStatus;

typedef struct W32_SMSV_Task W32_SMSV_Task;
struct W32_SMSV_Task
{
  W32_SMSV_Task *next;
  W32_SMSV_Task *prev;
  
  // rjf: key / parameters
  Arena *arena;
  String8 local_path;
  
  // rjf: working state
  W32_SMSV_TaskStatus status;
  String8Node *current_server_url_node;
  String8List file_pieces;
};

typedef struct W32_SMSV_TaskNode W32_SMSV_TaskNode;
struct W32_SMSV_TaskNode
{
  W32_SMSV_TaskNode *next;
  W32_SMSV_Task *task;
};

typedef struct W32_SMSV_TaskSlot W32_SMSV_TaskSlot;
struct W32_SMSV_TaskSlot
{
  W32_SMSV_Task *first;
  W32_SMSV_Task *last;
};

typedef struct W32_SMSV_State W32_SMSV_State;
struct W32_SMSV_State
{
  Arena *arena;
  String8 symbol_cache_path;
  String8List symbol_server_urls;
  U64 task_slots_count;
  W32_SMSV_TaskSlot *task_slots;
  StripeArray task_stripes;
  GuardedRing *http_response_ring;
  GuardedRing *new_task_ring;
  Arena *retry_arena;
  W32_SMSV_TaskNode *first_retry_request;
  W32_SMSV_TaskNode *last_retry_request;
};

global W32_SMSV_State *w32_smsv_state = 0;

////////////////////////////////
//~ rjf: Helpers

internal String8 w32_smsv_dbg_name_from_local_path(String8 path);
internal String8 w32_smsv_unique_identifier_from_local_path(String8 path);

#endif // WIN32_SYMBOL_SERVER_H
