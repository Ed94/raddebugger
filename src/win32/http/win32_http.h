// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef WIN32_HTTP_H
#define WIN32_HTTP_H

#include <winhttp.h>
#pragma comment(lib, "winhttp")

typedef enum W32_HTTP_RequestStatus
{
  W32_HTTP_RequestStatus_Null,
  W32_HTTP_RequestStatus_PendingSend,
  W32_HTTP_RequestStatus_Sent,
  W32_HTTP_RequestStatus_PendingRecv,
  W32_HTTP_RequestStatus_Recvd,
  W32_HTTP_RequestStatus_PendingDataSize,
  W32_HTTP_RequestStatus_DataSized,
  W32_HTTP_RequestStatus_PendingRead,
  W32_HTTP_RequestStatus_Done,
  W32_HTTP_RequestStatus_Failed,
}
W32_HTTP_RequestStatus;

typedef struct W32_HTTP_Request W32_HTTP_Request;
struct W32_HTTP_Request
{
  B32 active;
  Arena *arena;
  
  // rjf: request parameters
  GuardedRing *out_ring;
  U64 id;
  
  // rjf: working state
  W32_HTTP_RequestStatus status;
  HINTERNET hConnect;
  HINTERNET hRequest;
  void *optional; // NOTE(rjf): must persist with in-flight requests
  U64 optional_size;
  HTTP_StatusCode status_code;
  U64 total_response_bytes_sent;
  U64 total_response_bytes;
  U64 arena_start_body_read_pos;
  void *next_body_piece;
  U64 next_body_piece_size;
  
  // rjf: finished response body read state
  Arena *finished_body_arena;
  String8List finished_body_pieces;
};

typedef struct W32_HTTP_Response W32_HTTP_Response;
struct W32_HTTP_Response
{
  W32_HTTP_Response *next;
  GuardedRing *out_ring;
  String8 record;
};

typedef struct W32_HTTP_State W32_HTTP_State;
struct W32_HTTP_State
{
  Arena *arena;
  HINTERNET hSession;
  GuardedRing *request_ring;
  W32_HTTP_Request active_requests[16];
};

global W32_HTTP_State *w32_http_state = 0;

////////////////////////////////
//~ rjf: Status Callback

internal void w32_http_status_callback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, void *lpvStatusInformation, DWORD dwStatusInformationLength);

#endif // WIN32_HTTP_H
