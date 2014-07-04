localdir := src/algorithm
locallib := libttalg.a

objs-y := sort.o

$(eval $(call make-lib,$(localdir),$(locallib),$(objs-y)))
