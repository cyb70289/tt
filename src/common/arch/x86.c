/* x86.c
 *
 * Copyright (C) 2015 Yibo Cai
 */
#include <tt/tt.h>

/* Return "a+b" and carry */
uint _tt_add_uint(uint a, uint b, char *c)
{
	uint r;

	__asm__("add %3, %2;"
		"mov %2, %0;"
		"setc %1;"
		: "=r"(r), "=m"(*c)
		: "r"(a), "m"(b));

	return r;
}

/* Return "a-b" and carry */
uint _tt_sub_uint(uint a, uint b, char *c)
{
	uint r;

	__asm__("sub %3, %2;"
		"mov %2, %0;"
		"setc %1;"
		: "=r"(r), "=m"(*c)
		: "r"(a), "m"(b));

	return r;
}
