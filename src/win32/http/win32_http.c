// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

////////////////////////////////
//~ rjf: Status Callback

internal void
w32_http_status_callback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, void *lpvStatusInformation, DWORD dwStatusInformationLength)
{
  W32_HTTP_Request *req = (W32_HTTP_Request *)dwContext;
  B32 reloop = 1;
  switch(dwInternetStatus)
  {
    default:{reloop = 0;}break;
    case WINHTTP_CALLBACK_STATUS_REQUEST_ERROR:
    {
      ins_atomic_u32_eval_assign(&req->status, W32_HTTP_RequestStatus_Failed);
    }break;
    case WINHTTP_CALLBACK_STATUS_REQUEST_SENT:
    {
      ins_atomic_u32_eval_cond_assign(&req->status, W32_HTTP_RequestStatus_Sent, W32_HTTP_RequestStatus_PendingSend);
    }break;
    case WINHTTP_CALLBACK_STATUS_HEADERS_AVAILABLE:
    {
      req->next_body_piece = 0;
      req->next_body_piece_size = 0;
      ins_atomic_u32_eval_cond_assign(&req->status, W32_HTTP_RequestStatus_Recvd, W32_HTTP_RequestStatus_PendingRecv);
    }break;
    case WINHTTP_CALLBACK_STATUS_DATA_AVAILABLE:
    {
      DWORD *size_ptr = (DWORD *)lpvStatusInformation;
      req->next_body_piece_size = size_ptr[0];
      if(size_ptr[0] != 0)
      {
        ins_atomic_u32_eval_assign(&req->status, W32_HTTP_RequestStatus_DataSized);
      }
      else
      {
        ins_atomic_u32_eval_assign(&req->status, W32_HTTP_RequestStatus_Done);
      }
    }break;
    case WINHTTP_CALLBACK_STATUS_READ_COMPLETE:
    {
      req->next_body_piece_size = dwStatusInformationLength;
      ins_atomic_u32_eval_assign(&req->status, W32_HTTP_RequestStatus_Recvd);
    }break;
  }
  if(reloop)
  {
    ins_atomic_u32_eval_assign(&async_loop_again, 1);
    cond_var_broadcast(async_tick_start_cond_var);
  }
}

////////////////////////////////
//~ rjf: @per_os_impl Top-Level Layer Calls

internal void
http_init(void)
{
  Arena *arena = arena_alloc();
  w32_http_state = push_array(arena, W32_HTTP_State, 1);
  w32_http_state->arena = arena;
  w32_http_state->request_ring = guarded_ring_alloc(arena, MB(1));
  for EachElement(idx, w32_http_state->active_requests)
  {
    w32_http_state->active_requests[idx].arena = arena_alloc();
    w32_http_state->active_requests[idx].finished_body_arena = arena_alloc();
  }
  w32_http_state->hSession = WinHttpOpen(0, WINHTTP_ACCESS_TYPE_NO_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, WINHTTP_FLAG_ASYNC);
  WinHttpSetStatusCallback(w32_http_state->hSession, w32_http_status_callback, WINHTTP_CALLBACK_FLAG_ALL_NOTIFICATIONS, 0);
}

internal void
http_async_tick(void)
{
  //- rjf: pop requests, fill active request slots
  if(lane_idx() == 0)
  {
    for(;;)
    {
      Temp scratch = scratch_begin(0, 0);
      
      //- rjf: find a free active request slot
      U64 free_slot_num = 0;
      for EachElement(idx, w32_http_state->active_requests)
      {
        if(!w32_http_state->active_requests[idx].active)
        {
          free_slot_num = idx+1;
          break;
        }
      }
      
      //- rjf: if we got one -> pop request data
      String8 request_data = {0};
      if(free_slot_num != 0)
      {
        RingGuard guard = guarded_ring_open(w32_http_state->request_ring);
        guarded_ring_try_read_struct(&guard, &request_data.size);
        if(request_data.size != 0)
        {
          request_data.str = push_array(scratch.arena, U8, request_data.size);
          guarded_ring_read_or_wait(&guard, request_data.size, request_data.str, max_U64);
        }
        guarded_ring_close(&guard);
      }
      
      //- rjf: deserialize
      GuardedRing *out_ring = 0;
      HTTP_RequestParams params = {0};
      if(request_data.size != 0)
      {
        U64 off = 0;
        off += str8_deserial_read_struct(request_data, off, &out_ring);
        off += str8_deserial_read_struct(request_data, off, &params.id);
        off += str8_deserial_read_struct(request_data, off, &params.method);
        off += str8_deserial_read_struct(request_data, off, &params.url.size);
        params.url = str8_substr(request_data, r1u64(off, off+params.url.size));
        off += params.url.size;
        off += str8_deserial_read_struct(request_data, off, &params.body.size);
        params.body = str8_substr(request_data, r1u64(off, off+params.body.size));
        off += params.body.size;
        off += str8_deserial_read_struct(request_data, off, &params.user_agent.size);
        params.user_agent = str8_substr(request_data, r1u64(off, off+params.user_agent.size));
        off += params.user_agent.size;
        off += str8_deserial_read_struct(request_data, off, &params.authorization.size);
        params.authorization = str8_substr(request_data, r1u64(off, off+params.authorization.size));
        off += params.authorization.size;
        off += str8_deserial_read_struct(request_data, off, &params.content_type.size);
        params.content_type = str8_substr(request_data, r1u64(off, off+params.content_type.size));
        off += params.content_type.size;
      }
      
      //- rjf: fill active request slot
      W32_HTTP_Request *req = 0;
      if(free_slot_num != 0 && request_data.size != 0)
      {
        req = &w32_http_state->active_requests[free_slot_num-1];
        req->active = 1;
        arena_clear(req->arena);
        arena_clear(req->finished_body_arena);
        req->out_ring = out_ring;
        req->id = params.id;
        req->status = W32_HTTP_RequestStatus_Null;
      }
      
      //- rjf: kick off request
      if(req != 0)
      {
        B32 good = 0;
        
        //- rjf: unpack
        HTTP_Method method    = params.method;
        String8 url           = params.url;
        String8 body          = params.body;
        String8 user_agent    = params.user_agent;
        String8 authorization = params.authorization;
        String8 content_type  = params.content_type;
        
        //- rjf: extract URL info
        String8 url_protocol_part = {0};
        String8 url_port_part = {0};
        String8 url_hostname_part = {0};
        String8 url_path_part = {0};
        {
          U64 protocol_delimiter_pos = str8_find_needle(url, 0, s("://"), 0);
          U64 post_protocol_pos = 0;
          if(protocol_delimiter_pos < url.size)
          {
            post_protocol_pos = protocol_delimiter_pos+3;
            url_protocol_part = str8_prefix(url, post_protocol_pos);
          }
          U64 last_colon_pos = str8_find_needle(url, post_protocol_pos, s(":"), 0);
          if(last_colon_pos < url.size)
          {
            url_port_part = str8_skip(url, last_colon_pos+1);
          }
          U64 first_non_protocol_slash_pos = str8_find_needle(url, post_protocol_pos, s("/"), 0);
          if(first_non_protocol_slash_pos < url.size)
          {
            url_hostname_part = str8_prefix(url, first_non_protocol_slash_pos);
            url_hostname_part = str8_skip(url_hostname_part, post_protocol_pos);
            url_path_part = str8_skip(url, first_non_protocol_slash_pos);
          }
          else
          {
            url_hostname_part = str8_skip(url, post_protocol_pos);
          }
        }
        
        //- rjf: convert url parts to connection parameters
        U16 port = INTERNET_DEFAULT_HTTPS_PORT;
        String16 hostname16 = {0};
        {
          if(url_port_part.size != 0)
          {
            port = (U16)u64_from_str8(url_port_part, 10);
          }
          else if(str8_match(url_protocol_part, s("https://"), StringMatchFlag_CaseInsensitive))
          {
            port = INTERNET_DEFAULT_HTTPS_PORT;
          }
          else if(str8_match(url_protocol_part, s("http://"), StringMatchFlag_CaseInsensitive))
          {
            port = INTERNET_DEFAULT_HTTP_PORT;
          }
          else if(str8_match(url_protocol_part, s("ftp://"), StringMatchFlag_CaseInsensitive))
          {
            port = 21;
          }
          else if(str8_match(url_protocol_part, s("ssh://"), StringMatchFlag_CaseInsensitive))
          {
            port = 22;
          }
          hostname16 = str16_from_8(scratch.arena, url_hostname_part);
        }
        
        //- rjf: convert method to verb wchar
        WCHAR *verb = L"GET";
        switch(method)
        {
          default:
          case HTTP_Method_Get:               {verb = L"GET";}break;
          case HTTP_Method_Head:              {verb = L"HEAD";}break;
          case HTTP_Method_Post:              {verb = L"POST";}break;
          case HTTP_Method_Put:               {verb = L"PUT";}break;
          case HTTP_Method_Delete:            {verb = L"DELETE";}break;
          case HTTP_Method_Connect:           {verb = L"CONNECT";}break;
          case HTTP_Method_Options:           {verb = L"OPTIONS";}break;
          case HTTP_Method_Trace:             {verb = L"TRACE";}break;
          case HTTP_Method_Patch:             {verb = L"PATCH";}break;
        }
        
        //- rjf: convert url parts to request parameters
        WCHAR *path_name = L"";
        if(url_path_part.size != 0)
        {
          String16 url_path_part16 = str16_from_8(scratch.arena, url_path_part);
          path_name = (WCHAR *)url_path_part16.str;
        }
        
        //- rjf: unpack body params
        String16 header16 = {0};
        {
          String8List header_strings = {0};
          if(user_agent.size != 0)    { str8_list_pushf(scratch.arena, &header_strings, "User-Agent: %S\n", user_agent); }
          if(authorization.size != 0) { str8_list_pushf(scratch.arena, &header_strings, "Authorization: %S\n", authorization); }
          if(content_type.size != 0)  { str8_list_pushf(scratch.arena, &header_strings, "Content-Type: %S\n", content_type); }
          String8 header = str8_list_join(scratch.arena, &header_strings, 0);
          header16 = str16_from_8(scratch.arena, header);
        }
        
        //- rjf: convert body/header info to winhttp expected params
        WCHAR *header = WINHTTP_NO_ADDITIONAL_HEADERS;
        U64 header_size = 0;
        void *optional = WINHTTP_NO_REQUEST_DATA;
        U64 optional_size = 0;
        if(header16.size != 0)
        {
          header = (WCHAR *)header16.str;
          header_size = header16.size;
        }
        if(body.size != 0)
        {
          String8 body_copy = str8_copy(req->arena, body);
          optional = body_copy.str;
          optional_size = body_copy.size;
        }
        
        //- rjf: open connection
        HINTERNET hConnect = 0;
        if(w32_http_state->hSession != 0)
        {
          hConnect = WinHttpConnect(w32_http_state->hSession, (WCHAR *)hostname16.str, port, 0);
          good = (hConnect != 0);
        }
        
        //- rjf: open request
        HINTERNET hRequest = 0;
        if(good)
        {
          hRequest = WinHttpOpenRequest(hConnect, verb, path_name, 0, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
          good = (hRequest != 0);
        }
        
        //- rjf: fill info
        if(good)
        {
          req->status = W32_HTTP_RequestStatus_PendingSend;
        }
        else
        {
          req->status = W32_HTTP_RequestStatus_Failed;
        }
        req->hConnect = hConnect;
        req->hRequest = hRequest;
        req->arena_start_body_read_pos = arena_pos(req->arena);
        
        //- rjf: send request
        if(good)
        {
          WinHttpSendRequest(hRequest, header, header_size, optional, optional_size, optional_size, (DWORD_PTR)req);
        }
      }
      
      scratch_end(scratch);
      if(free_slot_num == 0 || request_data.size == 0)
      {
        break;
      }
    }
  }
  lane_sync();
  
  //- rjf: kick off receives
  {
    Rng1U64 range = lane_range(ArrayCount(w32_http_state->active_requests));
    for EachInRange(idx, range)
    {
      W32_HTTP_Request *req = &w32_http_state->active_requests[idx];
      if(req->active && ins_atomic_u32_eval(&req->status) == W32_HTTP_RequestStatus_Sent)
      {
        ins_atomic_u32_eval_assign(&req->status, W32_HTTP_RequestStatus_PendingRecv);
        WinHttpReceiveResponse(req->hRequest, 0);
      }
    }
    lane_sync();
  }
  
  //- rjf: push response output
  {
    Rng1U64 range = lane_range(ArrayCount(w32_http_state->active_requests));
    for EachInRange(idx, range)
    {
      W32_HTTP_Request *req = &w32_http_state->active_requests[idx];
      W32_HTTP_RequestStatus status = ins_atomic_u32_eval(&req->status);
      if(req->active && (status == W32_HTTP_RequestStatus_Done ||
                         status == W32_HTTP_RequestStatus_Failed ||
                         req->finished_body_pieces.total_size >= req->out_ring->ring->size/2))
      {
        Temp scratch = scratch_begin(0, 0);
        
        // rjf: figure out status code
        if(req->status_code == 0)
        {
          DWORD status_code = 0;
          DWORD status_code_size = sizeof(status_code);
          WinHttpQueryHeaders(req->hRequest, 
                              WINHTTP_QUERY_STATUS_CODE|WINHTTP_QUERY_FLAG_NUMBER,
                              WINHTTP_HEADER_NAME_BY_INDEX, 
                              &status_code, &status_code_size, WINHTTP_NO_HEADER_INDEX);
          req->status_code = status_code;
        }
        
        // rjf: figure out total bytes
        if(req->total_response_bytes == 0)
        {
          DWORD buffer_size = sizeof(req->total_response_bytes);
          WinHttpQueryHeaders(req->hRequest,
                              WINHTTP_QUERY_CONTENT_LENGTH|WINHTTP_QUERY_FLAG_NUMBER64,
                              WINHTTP_HEADER_NAME_BY_INDEX,
                              &req->total_response_bytes, 
                              &buffer_size,
                              WINHTTP_NO_HEADER_INDEX);
        }
        
        // rjf: join all currently read body contents
        String8 body = str8_list_join(scratch.arena, &req->finished_body_pieces, 0);
        
        // rjf: clear current body output
        arena_clear(req->finished_body_arena);
        MemoryZeroStruct(&req->finished_body_pieces);
        
        // rjf: compute size per record header
        U64 max_record_size = req->out_ring->ring->size;
        U64 bytes_per_record_header = sizeof(U64) + sizeof(U64) + sizeof(U64) + sizeof(U64) + sizeof(B32) + sizeof(HTTP_StatusCode);
        
        // rjf: push response records
        for(U64 off = 0, next_off = 0; off <= body.size; off = next_off)
        {
          // rjf: determine how much of the body we can fit into this record
          U64 body_data_size = (body.size - off);
          if(bytes_per_record_header + body_data_size > max_record_size)
          {
            body_data_size = max_record_size - bytes_per_record_header;
          }
          
          // rjf: advance by that amount of the body
          next_off = off + body_data_size;
          
          // rjf: determine if we have more responses after this one
          B32 is_last = (next_off == body.size && (status == W32_HTTP_RequestStatus_Done || status == W32_HTTP_RequestStatus_Failed));
          B32 has_more = !is_last;
          
          // rjf: serialize this record
          String8 record_data = {0};
          if(is_last || next_off != off)
          {
            String8List pieces = {0};
            U64 record_size = bytes_per_record_header + (next_off - off);
            str8_list_push(scratch.arena, &pieces, str8_struct(&record_size));
            str8_list_push(scratch.arena, &pieces, str8_struct(&req->id));
            str8_list_push(scratch.arena, &pieces, str8_struct(&req->total_response_bytes_sent));
            str8_list_push(scratch.arena, &pieces, str8_struct(&req->total_response_bytes));
            str8_list_push(scratch.arena, &pieces, str8_struct(&has_more));
            str8_list_push(scratch.arena, &pieces, str8_struct(&req->status_code));
            str8_list_push(scratch.arena, &pieces, str8_substr(body, r1u64(off, next_off)));
            record_data = str8_list_join(scratch.arena, &pieces, 0);
          }
          
          // rjf: push this record
          B32 push_failed = 1;
          if(record_data.size != 0)
          {
            RingGuard g = guarded_ring_open(req->out_ring);
            push_failed = !guarded_ring_write_string_or_wait(&g, record_data, 0);
            guarded_ring_close(&g);
          }
          
          // rjf: push failed -> remember unsent portions of body
          if(push_failed)
          {
            String8 unpushed_body_piece = str8_substr(body, r1u64(off, body.size));
            if(unpushed_body_piece.size != 0)
            {
              str8_list_push(req->finished_body_arena, &req->finished_body_pieces, str8_copy(req->finished_body_arena, unpushed_body_piece));
            }
          }
          
          // rjf: advance total bytes sent
          if(!push_failed)
          {
            req->total_response_bytes_sent += (next_off - off);
          }
          
          // rjf: cancel on no movement
          if(next_off == off || push_failed)
          {
            break;
          }
        }
        
        scratch_end(scratch);
      }
    }
    lane_sync();
  }
  
  //- rjf: kick off body size queries
  {
    Rng1U64 range = lane_range(ArrayCount(w32_http_state->active_requests));
    for EachInRange(idx, range)
    {
      W32_HTTP_Request *req = &w32_http_state->active_requests[idx];
      if(req->active && ins_atomic_u32_eval(&req->status) == W32_HTTP_RequestStatus_Recvd)
      {
        if(req->next_body_piece_size != 0)
        {
          str8_list_push(req->finished_body_arena, &req->finished_body_pieces, str8_copy(req->finished_body_arena, str8((U8 *)req->next_body_piece, req->next_body_piece_size)));
          req->next_body_piece_size = 0;
          req->next_body_piece = 0;
          arena_pop_to(req->arena, req->arena_start_body_read_pos);
        }
        ins_atomic_u32_eval_assign(&req->status, W32_HTTP_RequestStatus_PendingDataSize);
        WinHttpQueryDataAvailable(req->hRequest, 0);
      }
    }
    lane_sync();
  }
  
  //- rjf: kick off body reads
  {
    Rng1U64 range = lane_range(ArrayCount(w32_http_state->active_requests));
    for EachInRange(idx, range)
    {
      W32_HTTP_Request *req = &w32_http_state->active_requests[idx];
      if(req->active && ins_atomic_u32_eval(&req->status) == W32_HTTP_RequestStatus_DataSized)
      {
        req->next_body_piece = push_array(req->arena, U8, req->next_body_piece_size);
        ins_atomic_u32_eval_assign(&req->status, W32_HTTP_RequestStatus_PendingRead);
        WinHttpReadData(req->hRequest, req->next_body_piece, req->next_body_piece_size, 0);
      }
    }
    lane_sync();
  }
  
  //- rjf: complete requests
  {
    Rng1U64 range = lane_range(ArrayCount(w32_http_state->active_requests));
    for EachInRange(idx, range)
    {
      W32_HTTP_Request *req = &w32_http_state->active_requests[idx];
      W32_HTTP_RequestStatus status = ins_atomic_u32_eval(&req->status);
      if(req->active && req->finished_body_pieces.total_size == 0 && req->next_body_piece_size == 0 &&
         (status == W32_HTTP_RequestStatus_Done ||
          status == W32_HTTP_RequestStatus_Failed))
      {
        req->active = 0;
        arena_clear(req->arena);
        arena_clear(req->finished_body_arena);
        MemoryZeroStruct(&req->finished_body_pieces);
        req->next_body_piece = 0;
        req->next_body_piece_size = 0;
        WinHttpCloseHandle(req->hConnect);
        WinHttpCloseHandle(req->hRequest);
      }
    }
    lane_sync();
  }
}

////////////////////////////////
//~ rjf: @per_os_impl I/O Ring Functions

internal B32
http_push_request(GuardedRing *out_ring, HTTP_RequestParams *params, U64 endt_us)
{
  Temp scratch = scratch_begin(0, 0);
  B32 result = 0;
  
  // rjf: serialize
  String8 data = {0};
  {
    String8List pieces = {0};
    str8_list_push(scratch.arena, &pieces, str8_struct(&out_ring));
    str8_list_push(scratch.arena, &pieces, str8_struct(&params->id));
    str8_list_push(scratch.arena, &pieces, str8_struct(&params->method));
    str8_list_push(scratch.arena, &pieces, str8_struct(&params->url.size));
    str8_list_push(scratch.arena, &pieces, params->url);
    str8_list_push(scratch.arena, &pieces, str8_struct(&params->body.size));
    str8_list_push(scratch.arena, &pieces, params->body);
    str8_list_push(scratch.arena, &pieces, str8_struct(&params->user_agent.size));
    str8_list_push(scratch.arena, &pieces, params->user_agent);
    str8_list_push(scratch.arena, &pieces, str8_struct(&params->authorization.size));
    str8_list_push(scratch.arena, &pieces, params->authorization);
    str8_list_push(scratch.arena, &pieces, str8_struct(&params->content_type.size));
    str8_list_push(scratch.arena, &pieces, params->content_type);
    U64 data_size = pieces.total_size;
    str8_list_push_front(scratch.arena, &pieces, str8_struct(&data_size));
    data = str8_list_join(scratch.arena, &pieces, 0);
  }
  
  // rjf: write
  if(data.size < w32_http_state->request_ring->ring->size)
  {
    RingGuard g = guarded_ring_open(w32_http_state->request_ring);
    result = guarded_ring_write_string_or_wait(&g, data, endt_us);
    guarded_ring_close(&g);
  }
  
  scratch_end(scratch);
  return result;
}

internal B32
http_pop_response(Arena *arena, GuardedRing *out_ring, HTTP_Response *response_out, U64 endt_us)
{
  RingGuard g = guarded_ring_open(out_ring);
  U64 data_size = 0;
  B32 result = guarded_ring_read_struct_or_wait(&g, &data_size, endt_us);
  if(result)
  {
    Temp scratch = scratch_begin(&arena, 1);
    U64 data_size_minus_size_prefix = data_size - sizeof(data_size);
    String8 data = {0};
    data.size = data_size_minus_size_prefix;
    data.str = push_array(scratch.arena, U8, data.size);
    guarded_ring_read_or_wait(&g, data.size, data.str, max_U64);
    U64 off = 0;
    off += str8_deserial_read_struct(data, off, &response_out->id);
    off += str8_deserial_read_struct(data, off, &response_out->off);
    off += str8_deserial_read_struct(data, off, &response_out->off_opl);
    off += str8_deserial_read_struct(data, off, &response_out->has_more);
    off += str8_deserial_read_struct(data, off, &response_out->code);
    response_out->body.size = (data.size - off);
    response_out->body.str = push_array(arena, U8, response_out->body.size);
    off += str8_deserial_read(data, off, response_out->body.str, response_out->body.size, 1);
    scratch_end(scratch);
  }
  guarded_ring_close(&g);
  return result;
}
