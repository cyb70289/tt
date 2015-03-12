/* Buffer management
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>
#include "lib.h"

/* TODO: implement hash pool */

void *_tt_get_buf(size_t sz)
{
	return malloc(sz);
}

void _tt_put_buf(void *buf)
{
	free(buf);
}
