// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "http.c"
#if OS_WINDOWS
# include "win32/http/win32_http.c"
#else
# include "http_stub.c"
#endif
