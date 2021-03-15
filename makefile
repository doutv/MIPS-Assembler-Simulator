prom = simulator
test_dir = ./test
asm_tests = 1 2 3 4 5 6 7 8 9 10 11 12 a-plus-b fib memcpy-hello-world
sim_tests = a-plus-b fib memcpy-hello-world

.PHONY: test all clean
.ONESHELL:

all: $(prom) asm_test sim_tests
	@echo "All tests passed!"

$(prom): $(prom).cpp
	g++ $(prom).cpp -o $(prom)

clean:
	rm $(prom)
	rm $(test_dir)/*.tasmout
	rm $(test_dir)/*.tsimout

asm_test: $(prom)
	for t in $(asm_tests); do \
		./$(prom) $(test_dir)/$$t.asm $(test_dir)/$$t.tasmout 2>&1; \
		diff -q $(test_dir)/$$t.asmout $(test_dir)/$$t.tasmout > /dev/null || \
			(echo "Test $$t failed" && exit 1); \
	done

sim_test: $(prom)
	for t in $(sim_tests); do \
		./$(prom) $(test_dir)/$$t.asm $(test_dir)/$$t.in $(test_dir)/$$t.tsimout 2>&1; \
		diff -q $(test_dir)/$$t.out $(test_dir)/$$t.tsimout > /dev/null || \
			(echo "Test $$t failed" && exit 1); \
	done