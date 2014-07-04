localdir := src/numerical
locallib := libttnum.a

objs-y := mtx.o mtx-gaussj.o

$(eval $(call make-lib,$(localdir),$(locallib),$(objs-y)))
