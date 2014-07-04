localdir := tests

# To add a new test binary:
# testname := xxx
# objs-$(testname) := x1.o x2.o ...
# $(eval $(call make-test,$(localdir),$(testname),$(objs-$(testname))))

testname := matrix
objs-$(testname) := matrix.o
$(eval $(call make-test,$(localdir),$(testname),$(objs-$(testname))))

testname := sort
objs-$(testname) := sort.o
$(eval $(call make-test,$(localdir),$(testname),$(objs-$(testname))))
