# Makefile
#
# Copyright (C) 2014 Yibo Cai

TOPDIR		:= $(CURDIR)

# Set to 0 to turn off color message
COLOR_MSG	:= 1

# Show terse build message unless "make V=1"
ifeq ("$(origin V)", "command line")
  Q :=
else
  Q := @
endif

# Host
HOSTCC		:= gcc
HOST_STRIP	:= strip
HOST_CFLAGS	:=
HOST_LDLIBS	:=
HOST_SHELL	:= /bin/sh

# Cross compiler
CROSS_COMPILE	?=
CC		:= $(CROSS_COMPILE)gcc
LD		:= $(CROSS_COMPILE)ld
AR		:= $(CROSS_COMPILE)ar
STRIP		:= $(CROSS_COMPILE)strip
OBJCOPY		:= $(CROSS_COMPILE)objcopy
OBJDUMP		:= $(CROSS_COMPILE)objdump

# Tools
RM		:= rm -f
CP		:= cp -f
RMDIR		:= rm -rf
MKDIR		:= mkdir -p
MV		:= mv -f
SED		:= sed
ECHO		:= echo -e

# Extra files to clean
CLEAN_FILES	:=
DISTCLEAN_FILES	:= .config* defconfig

# Compiler flags
CPPFLAGS	:= -g -O2 -fno-strict-aliasing
CPPFLAGS	+= -I$(TOPDIR)/include -I$(TOPDIR)/src/include
CFLAGS		+= -Wall -Werror -Wstrict-prototypes -std=c99
AFLAGS		+= $(CPPFLAGS)
ARFLAGS		:= rcs

# Target specific
CFLAGS_TARGET	:=
LDFLAGS_TARGET	:=

Kconfig		:= Kconfig

# Construct in non recursive method
OBJS		:=

TTLIBS		:=
TTTESTS		:=

# Check if were are building image
BUILD_IMAGE	:= 1
ifneq "$(filter %help,$(MAKECMDGOALS))" ""
  # help
  BUILD_IMAGE	:= 0
endif
ifneq "$(filter %install,$(MAKECMDGOALS))" ""
  # install
  BUILD_IMAGE	:= 0
endif
ifneq "$(filter %clean,$(MAKECMDGOALS))" ""
  # clean, distclean
  BUILD_IMAGE	:= 0
endif
ifneq "$(filter %config,$(MAKECMDGOALS))" ""
  # config, menuconfig, ...
  BUILD_IMAGE	:= 0
endif

# Color message
ifeq ("$(COLOR_MSG)", "0")
  GREEN		:=
  BLUE		:=
  YELLOW	:=
  COLOR_END	:=
else
  GREEN		:= "\033[32m"
  BLUE		:= "\033[36m"
  YELLOW	:= "\033[33m"
  COLOR_END	:= "\033[0m"
endif

# $call(terse_cc,target)
define terse_cc
  [ "$(Q)" = "@" ] && $(ECHO) CC $1 || $(ECHO) > /dev/null
endef

# $call(terse_ar,target)
define terse_ar
  [ "$(Q)" = "@" ] && $(ECHO) $(GREEN)AR $1$(COLOR_END) || $(ECHO) > /dev/null
endef

# $call(terse_ld,target)
define terse_ld
  [ "$(Q)" = "@" ] && $(ECHO) $(YELLOW)LD $1$(COLOR_END) || $(ECHO) > /dev/null
endef

all:

# Kconfig
include scripts/kconfig/config.mk
ifeq "$(BUILD_IMAGE)" "1"
  -include include/config/auto.conf
  -include include/config/auto.conf.cmd
endif

# Regenerate if .config is updated
include/config/%.conf: scripts/kconfig/conf .config include/config/auto.conf.cmd
	@$(MKDIR) include/config include/generated
	$< --silentoldconfig $(Kconfig)

.config include/config/auto.conf.cmd:

# Generate libraries
# $(call make-lib,localdir,libname,object-files)
define make-lib
  tt_locallib := $1/$2
  tt_localobjs := $(addprefix $1/,$3)
  TTLIBS += $$(tt_locallib)
  OBJS += $$(tt_localobjs)

  $$(tt_locallib): $$(tt_localobjs)
	$(Q)$(AR) $(ARFLAGS) $$@ $$^
	@$(call terse_ar,$$@)
endef

# Generate test binaries
# $(call make-test,localdir,testname,object-files)
define make-test
  tt_testname := $1/$2
  tt_localobjs := $(addprefix $1/,$3)
  TTTESTS += $$(tt_testname)
  OBJS += $(filter-out $$(OBJS),$$(tt_localobjs))

  $$(tt_testname): $$(tt_localobjs) $$(TTLIBS)
	$(Q)$(CC) $(LDFLAGS_TARGET) $$^ $$(TTLIBS) -o $$@
	@$(call terse_ld,$$@)
endef

# Include all config.mk
include src/config.mk
include tests/config.mk

# Dependencies

# $(call to-dep-file,suffix,file-list)
# path/source.o -> path/.source.d
define to-dep-file
  $(foreach f,$(subst $1,.d,$2),$(dir $(f)).$(notdir $(f)))
endef

DEPS = $(call to-dep-file,.o,$(OBJS))

# $(call make-depend,source-file,object-file,depend-file)
define make-depend
  $(CC) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -M $1 |	\
	$(SED) 's,\($(notdir $2)\) *:,$2 $3: ,' > $3.tmp
	# Append empty rules
	$(SED)  -e 's/#.*//'		\
		-e 's/^[^:]*: *//'	\
		-e 's/ *\\$$//'		\
		-e '/^$$/ d'		\
		-e 's/$$/ :/'		\
		< $3.tmp >> $3.tmp
  $(MV) $3.tmp $3
endef

%.o: %.c
	@$(call make-depend,$<,$@,$(call to-dep-file,.o,$@))
	$(Q)$(COMPILE.c) $(CFLAGS_TARGET) -o $@ $<
	@$(call terse_cc,$@)

%.o: %.S
	@$(call make-depend,$<,$@,$(call to-dep-file,.o,$@))
	$(Q)$(COMPILE.S) $(CFLAGS_TARGET) -o $@ $<
	@$(call terse_cc,$@)

ifeq "$(BUILD_IMAGE)" "1"
  -include $(DEPS)
endif

# Target
.PHONY: all install clean distclean

$(OBJS): include/config/auto.conf

all: $(TTLIBS) $(TTTESTS)

install: all

clean:
	find src tests -name *.o -o -name .*.d | xargs $(RM)
	$(RM) $(TTLIBS) $(TTTESTS)

distclean: clean
	@find scripts/kconfig -name *.o | xargs $(RM)
	$(RM) $(DISTCLEAN_FILES)
	$(RMDIR) include/generated include/config
