// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef SYMBOL_SERVER_INC_H
#define SYMBOL_SERVER_INC_H

#include "symbol_server.h"
#if OS_WINDOWS
# include "win32/symbol_server/win32_symbol_server.h"
#else
# include "symbol_server_stub.h"
#endif

#endif // SYMBOL_SERVER_INC_H
