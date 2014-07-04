/* _types.h
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

#define __likely(x)	(__builtin_expect(!!(x), 1))
#define __unlikely(x)	(__builtin_expect(!!(x), 0))

#define __weak		__attribute__((weak))

#define __align(n)	__attribute__((aligned (n)))
