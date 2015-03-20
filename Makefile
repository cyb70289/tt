# Makefile
#
# Copyright (C) 2014 Yibo Cai

#################################################
# General definitions
#################################################
TOPDIR		:= $(CURDIR)

# Show terse build message unless "make V=1"
ifeq ("$(origin V)", "command line")
  VERBOSE := $(V)
else
  VERBOSE := 0
endif
ifeq ($(VERBOSE),1)
  Q :=
  quiet :=
else
  Q := @
  quiet := quiet_
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
ECHO		:= echo

# Extra files to clean
CLEAN_FILES	:=

# Compiler flags
CPPFLAGS	+= -g -O2 -fno-strict-aliasing
CPPFLAGS	+= -I$(TOPDIR)/include -I$(TOPDIR)/src
CFLAGS		+= -Wall -Werror -Wstrict-prototypes -std=gnu99
AFLAGS		+= -I$(TOPDIR)/include

# Extra libraries
LDLIBS		+= -lm -lpthread

# Target specific flags
CFLAGS		+= $(CFLAGS_TARGET)
AFLAGS		+= $(AFLAGS_TARGET)
LDFLAGS		+= $(LDFLAGS_TARGET)

# Check if were are building image
BUILD_IMAGE	:= 1
ifneq "$(filter %help,$(MAKECMDGOALS))" ""
  # help
  BUILD_IMAGE	:= 0
endif
ifneq "$(filter %clean,$(MAKECMDGOALS))" ""
  # clean, distclean
  BUILD_IMAGE	:= 0
endif

# Short building messages
show_msg = $(if $($(quiet)msg_$1),$(ECHO) $($(quiet)msg_$1)$2)
quiet_msg_cc	= "  CC      "
quiet_msg_as	= "  AS      "
quiet_msg_ar	= "  AR      "
quiet_msg_ld	= "  LD      "
quiet_msg_link	= "  LINK    "
quiet_msg_clean	= "  CLEAN   "

# Default target
all:

#################################################
# Non-recursive build
#################################################
LIBS		:=
BINS		:=
DEPS		:=

# $(call to-dep-file,obj-file-list)
# path/source.o -> path/.source.d
to-dep-file = $(foreach f,$(subst .o,.d,$1),$(dir $(f)).$(notdir $(f)))

# Generate test binaries
# $(call make-test-target,dir,target,objs)
define make-test-target
  BINS += $1/$2
  $1/$2: $$(addprefix $1/,$3) $$(LIBS)
	$(Q)$(CC) $$^ $$(LIBS) $$(LDLIBS) -o $$@
	@$$(call show_msg,link,$$@)
endef

# $(call make-test,dir)
define make-test
  bin-y :=

  include $1/config.mk
  bin-y := $$(strip $$(bin-y))

  obj-y :=
  $$(foreach b,$$(bin-y),$$(eval obj-y += $$(strip $$($$(b)-objs))))
  DEPS += $$(call to-dep-file,$$(addprefix $1/,$$(sort $$(obj-y))))

  $$(foreach b,$$(bin-y),$$(eval $$(call \
				  make-test-target,$1,$$(b),$$($$(b)-objs))))
endef

# Parse config.mk recursively
# $(call include-config,dir)
define include-config
  obj-y :=
  dir-y :=
  lib-y :=

  include $1/config.mk
  obj-y := $$(strip $$(obj-y))
  dir-y := $$(strip $$(dir-y))
  lib-y := $$(strip $$(lib-y))

  DEPS += $$(call to-dep-file,$$(addprefix $1/,$$(obj-y)))
  LIBS += $$(addprefix $1/,$$(lib-y))

  $1/built-in.o: $$(addprefix $1/,$$(obj-y)) \
		$$(addsuffix /built-in.o,$$(addprefix $1/,$$(dir-y)))
	$(Q)$$(if $$^,$(LD) -r -o $$@ $$^,$(AR) rcs $$@)
	@$$(call show_msg,ld,$$@)

  ifneq "$$(lib-y)" ""
    $1/$$(lib-y): $1/built-in.o
	$(Q)$(AR) rcs $$@ $$^
	@$$(call show_msg,ar,$$@)
  endif

  cdir := $(subst /,-,$1)
  subdir-$$(cdir) := $$(addprefix $1/,$$(dir-y))
  $$(foreach d,$$(subdir-$$(cdir)),$$(eval $$(call include-config,$$(d))))
endef

# Include all config.mk
$(eval $(call include-config,src))
$(eval $(call make-test,tests))

#################################################
# Dependencies
#################################################
# $(call make-depend,source-file,object-file,depend-file)
define make-depend
  $(CC) $(CFLAGS) $(CPPFLAGS) -M $1 |	\
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
	@$(call make-depend,$<,$@,$(call to-dep-file,$@))
	$(Q)$(COMPILE.c) -o $@ $<
	@$(call show_msg,cc,$@)

%.o: %.S
	@$(call make-depend,$<,$@,$(call to-dep-file,$@))
	$(Q)$(COMPILE.S) -o $@ $<
	@$(call show_msg,as,$@)

ifeq "$(BUILD_IMAGE)" "1"
  -include $(DEPS)
endif

#################################################
# Targets
#################################################
.PHONY: all install clean

all: $(LIBS) $(BINS)

install: all

clean:
	$(Q)find src tests -name *.[oad] -o -name *.tmp | xargs $(RM)
	$(Q)$(RM) $(LIBS) $(BINS)
	@$(call show_msg,clean,src tests)
