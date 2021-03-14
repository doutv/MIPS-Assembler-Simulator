prom = assembler
test_dir = ./test
tests = 1 2 3 4 5 6 7 8 9 10 11 12 a-plus-b fib memcpy-hello-world

.PHONY: test all clean
.ONESHELL:

all: $(prom) test
	@echo "All tests passed!"

$(prom): $(prom).cpp
	g++ $(prom).cpp -o $(prom)

clean:
	rm $(prom)
	rm $(test_dir)/*.test

test: $(prom)
	for t in $(tests); do \
		echo In Test $$t >&2; \
		./$(prom) $(test_dir)/$$t.in $(test_dir)/$$t.test 2>&1; \
		diff -q $(test_dir)/$$t.out $(test_dir)/$$t.test > /dev/null || \
			echo Test $$t failed >&2; \
	done
# %.test:  $(prom) %.in %.out
# 	./$(prom) $(test_dir)/$< $(test_dir)/$(word 2, $?) | diff -q $(word 2, $?) -> /dev/null || \
# 	(echo "Test $@ failed" && exit 1)
	