localdir := src

# General libs should be put last

subdir-y := numerical
subdir-y += common

include $(patsubst %,$(localdir)/%/config.mk,$(subdir-y))
