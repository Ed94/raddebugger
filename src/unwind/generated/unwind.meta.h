// Copyright (c) Epic Games Tools
// Licensed under the MIT license (https://opensource.org/license/mit/)

//- GENERATED CODE

#ifndef UNWIND_META_H
#define UNWIND_META_H

typedef enum UWND_Unwinder
{
UWND_Unwinder_Null,
UWND_Unwinder_PEx64,
UWND_Unwinder_EHFrame,
UWND_Unwinder_COUNT,
} UWND_Unwinder;

#define UWND_Unwinder_XList \
X(PEx64, pe_x64)\
X(EHFrame, eh)\

#endif // UNWIND_META_H
