// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#include "symbol_server.c"
#if OS_WINDOWS
# include "win32/symbol_server/win32_symbol_server.c"
#else
# include "symbol_server_stub.c"
#endif
