/* Floating-point exception handling
 *
 * Copyright (C) 2015 Yibo Cai
 */
#define _GNU_SOURCE
#include <tt/tt.h>

#include <signal.h>
#include <string.h>
#include <fenv.h>
#include <ucontext.h>

#pragma GCC diagnostic ignored "-Wunused-variable"

static struct {
	const int exceptions;
	bool hooked;
	struct sigaction sa;
} fpe = {
	.exceptions = FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW | FE_UNDERFLOW,
};

static const struct {
	int code;
	const char *str;
} fpe_traps[] = {
	{ FPE_INTDIV, "Integer divide by zero" },
	{ FPE_INTOVF, "Integer overflow" },
	{ FPE_FLTDIV, "Floating-point divide by zero" },
	{ FPE_FLTOVF, "Floating-point overflow" },
	{ FPE_FLTUND, "Floating-point underflow" },
	{ FPE_FLTRES, "Floating-point inexact result" },
	{ FPE_FLTINV, "Floating-point invalid operation" },
	{ FPE_FLTSUB, "Subscript out of range" },
};

static const char *strfpe(int code)
{
	for (int i = 0; i < ARRAY_SIZE(fpe_traps); i++)
		if (fpe_traps[i].code == code)
			return fpe_traps[i].str;
	return "Unknown";
}

static void fpe_handler(int no, siginfo_t *si, void *data)
{
	tt_error("FPE caught at %p: %d '%s'", si->si_addr,
			si->si_code, strfpe(si->si_code));
	abort();
}

static __attribute__ ((constructor)) void fpe_init(void)
{
	feenableexcept(fpe.exceptions);

	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_sigaction = fpe_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART | SA_SIGINFO;
	if (sigaction(SIGFPE, &sa, &fpe.sa) < 0)
		tt_error("Failed to set SIGFPE handler");
	else
		fpe.hooked = true;
}

static __attribute__ ((destructor)) void fpe_deinit(void)
{
	feclearexcept(fpe.exceptions);
	if (fpe.hooked)
		sigaction(SIGFPE, &fpe.sa, NULL);
}
