/* def.h
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

typedef uint8_t uchar;
typedef uint16_t ushort;
typedef uint32_t uint;

#ifdef __LP64__
#define _TT_LP64_
#else
#undef _TT_LP64_
#endif
