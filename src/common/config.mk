localdir := src/common
locallib := libttcommon.a

objs-y := log.o general.o sort.o
objs-y += stack.o queue.o heap.o tree.o

$(eval $(call make-lib,$(localdir),$(locallib),$(objs-y)))
