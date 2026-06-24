// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

#ifndef UNWIND_INC_H
#define UNWIND_INC_H

#include "unwind.h"
#if defined(PE_H) && defined(X64_H)
# include "pe/x64/unwind/pe_x64_unwind.h"
#endif
#if defined(EH_FRAME_H)
# include "eh_frame/unwind/eh_frame_unwind.h"
#endif

#endif // UNWIND_INC_H
