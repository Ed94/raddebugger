// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////////////////////////////////////
//~ rjf: @per_os_impl Top-Level Layer Calls

internal void http_init(void){}
internal void http_async_tick(void){}

////////////////////////////////////////////////////////////////
//~ rjf: @per_os_impl I/O Ring Functions

internal B32
http_push_request(GuardedRing *out_ring, HTTP_RequestParams *params, U64 endt_us)
{
  return 0;
}

internal B32
http_pop_response(Arena *arena, GuardedRing *out_ring, HTTP_Response *response_out, U64 endt_us)
{
  return 0;
}
