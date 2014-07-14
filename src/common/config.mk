localdir := src/common
locallib := libttcommon.a

objs-y := log.o general.o stack.o queue.o heap.o sort.o tree.o

$(eval $(call make-lib,$(localdir),$(locallib),$(objs-y)))
