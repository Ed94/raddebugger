// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef SYMBOL_SERVER_H
#define SYMBOL_SERVER_H

typedef enum SMSV_Status
{
  SMSV_Status_Null,
  SMSV_Status_Pending,
}
SMSV_Status;

internal void smsv_init(void);
internal void smsv_async_tick(void);
internal String8 smsv_cache_path(void);
internal String8 smsv_local_path_from_key(Arena *arena, String8 dbg_name, Guid guid, U64 age);
internal void smsv_fill_local_path(String8 path);
internal SMSV_Status smsv_status_from_local_path(String8 path);

#endif // SYMBOL_SERVER_H
