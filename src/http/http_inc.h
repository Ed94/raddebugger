// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef HTTP_INC_H
#define HTTP_INC_H

#include "http.h"
#if OS_WINDOWS
# include "win32/http/win32_http.h"
#else
# include "http_stub.h"
#endif

#endif // HTTP_INC_H
