PROM = simulator
TEST_DIR = ./test
ASM_TESTS = 1 2 3 4 5 6 7 8 9 10 11 12 a-plus-b fib memcpy-hello-world
# SIM_TESTS = a-plus-b fib memcpy-hello-world
SIM_TESTS = fib

.PHONY: all clean
.ONESHELL:

all: $(PROM) sim_test
	@echo "All tests passed!"

$(PROM): $(PROM).cpp
	g++ $(PROM).cpp -o $(PROM) -std=c++17

clean:
	rm $(PROM)
	rm $(TEST_DIR)/*.tasmout
	rm $(TEST_DIR)/*.out

asm_test: $(PROM)
	for t in $(ASM_TESTS); do \
		./$(PROM) $(TEST_DIR)/$$t.asm $(TEST_DIR)/$$t.tasmout 2>&1; \
		diff -q $(TEST_DIR)/$$t.asmout $(TEST_DIR)/$$t.tasmout > /dev/null || \
			echo "Test $$t failed"; \
	done
	echo -e "All assembler tests passed!\n"


sim_test: $(PROM)
	for t in $(SIM_TESTS); do \
		./$(PROM) $(TEST_DIR)/$$t.asm $(TEST_DIR)/$$t.in $(TEST_DIR)/$$t.out 2>&1; \
		diff -q $(TEST_DIR)/$$t.out $(TEST_DIR)/$$t.simout > /dev/null || \
			echo "Test $$t failed"; \
	done
	echo -e "All simulator tests passed!\n"