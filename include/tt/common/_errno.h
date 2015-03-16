/* Error numbers
 *
 * Negative: fatal errors. Positive: un-fatal errors.
 *
 * Copyright (C) 2014 Yibo Cai
 */
#pragma once

/* General */
#define TT_ENOMEM		-1	/* Out of memory */
#define TT_EINVAL		-2	/* Invalid parameter */
#define TT_EOVERFLOW		-3	/* Buffer overflow */
#define TT_EUNDERFLOW		-4	/* Buffer underflow */
#define TT_ESTOP		 5	/* Stop operation */
#define TT_ENOBUFS		-6	/* Buffer too small */

/* Numerical */
#define TT_NUM_ESINGULAR	-101	/* Singular matrix */

/* APN */
#define TT_APN_EROUNDED		 201	/* Precision lost */
#define TT_APN_EOVERFLOW	-202
#define TT_APN_EUNDERFLOW	-203
#define TT_APN_EDIV_0		-204
#define TT_APN_EDIV_UNDEF	-205	/* 0 / 0 */
#define TT_APN_EINVAL		-206	/* log(-1), operate NaN, ... */
#define TT_APN_ESANITY		-207	/* APN sanity checking failed */
