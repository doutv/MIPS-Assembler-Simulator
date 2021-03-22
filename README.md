# CSC3050 Project 1 MIPS Assembler & Simulator
This project implements a MIPS assembler and simulator in a single `simulator.cpp` file. The assembler and the simulator are designed separately so that each of them is supposed to work independently.

Code: https://github.com/doutv/MIPS-Assembler-Simulator

Ref:
* http://courses.missouristate.edu/kenvollmar/mars/help/syscallhelp.html
* https://github.com/SamuelGong/MIPSAssemblerAndDisassembler

## Features
### Nested class
Ref:
* https://stackoverflow.com/questions/486099/can-inner-classes-access-private-variables
  * Unlike Java though, there is no correlation between an object of type `Outer::Inner` and an object of the parent class. You have to make the parent child relationship manually.
Wrap `Scanner` and `Parser` into `Assembler` using nested class, which enhances code reusability.
```cpp
class Assembler
{
  public:
    inline static vector<string> data_seg;
    inline static vector<string> text_seg;
    inline static vector<string> output;
    class Scanner
    {
      Assembler &assembler;
    };
    class Parser
    {
      Assembler &assembler;
    };
}
```
### Hash table mapping machine code to function pointer
Ref:
* https://stackoverflow.com/questions/2136998/using-a-stl-map-of-function-pointers
* https://stackoverflow.com/questions/55520876/creating-an-unordered-map-of-stdfunctions-with-any-arguments
* https://stackoverflow.com/questions/23962019/how-to-initialize-stdfunction-with-a-member-function
  * `print_add` is a non-static member function of `foo`, which means it must be invoked on an instance of `Foo`; hence it has an implicit first argument, the `this` pointer.

Hash table `unordered_map<string, function<void(const string &)>>` can replace numerous conditional statements (`if` `else if`).  
In its generating function `gen_opcode_to_func` , `std::bind` should be used to locate the function pointer of the current `Simulator` instance. e.g.

```cpp
// in void Simulator::gen_opcode_to_func(unordered_map<string, function<void(const string &)>> &m)
m.emplace("000100", bind(&Simulator::instr_beq, this, placeholders::_1));
```
Thus, the code is much more neater when calling the corresponding function.
```cpp
// in void Simulator::exec_instr(const string &mc)
if (opcode == R_opcode[0] || opcode == R_opcode[1])
{
    // R instructions
    string funct = mc.substr(mc.size() - 6, 6);
    auto it = opcode_funct_to_func.find(opcode + funct);
    if (it == opcode_funct_to_func.end())
        signal_exception("function not found!");
    (it->second)(mc);
}
```
### Test multiple cases with makefile
Ref:
* https://stackoverflow.com/questions/4927676/implementing-make-check-or-make-test
* https://blog.csdn.net/u011630575/article/details/52151995
  * 2>&1  意思是把 标准错误输出 重定向到 标准输出.
  * /dev/null是一个文件，这个文件比较特殊，所有传给它的东西它都丢弃掉
With this `makefile`, I can compile and test multiple cases by a single command `make`.  
Take `sim_test` for example, which tests both assembler and simulator, it will echo fail message and exit when the current case fails.

```makefile
PROM = simulator
TEST_DIR = ./test
ASM_TESTS = 1 2 3 4 5 6 7 8 9 10 11 12 a-plus-b fib memcpy-hello-world
SIM_TESTS = a-plus-b fib memcpy-hello-world

.PHONY: all clean
.ONESHELL:

all: $(PROM) asm_test sim_test
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
```

## Tricks
* Use fixed width integer types such as `int32_t` instead of `int`.
* Get the maximum possible number of a data type, e.g. the maximum number for `int16_t` is `numeric_limits<int16_t>::max()`
* Conversion between binary string `std::string` and integer `int32_t`
  * int to binary string: `bitset<str_length>(int32_t_number).to_string()`
  * binary string to int: `stoi(str,nullptr,base=2)`
* Detect overflow by ` __builtin_add_overflow` , ` __builtin_sub_overflow` , `__builtin_mul_overflow`.
  * https://stackoverflow.com/questions/199333/how-do-i-detect-unsigned-integer-multiply-overflow
  * ```cpp
    unsigned long b, c, c_test;
    if (__builtin_umull_overflow(b, c, &c_test))
    {
        // Returned non-zero: there has been an overflow
    }
    else
    {
        // Return zero: there hasn't been an overflow
    }
    ```
* `inline static` Declare and initialize static member variables together in class.
  * https://www.zhihu.com/question/375445099
  * 在 C++17 加入了内联静态成员变量，只要在声明成员变量时， 在`static`前加入 `inline`，就能顺便定义了它，还可初始化它，这就不用在类外定义了。
* `static const` Declare and initialize constant member variables in class.
* `#define` , `#ifdef` , `#endif` Use C preprocessor to help debug.
* Conversion between `std::string` and `std::array`
  * `std::string` -> `std::array<char,size>`
  * `copy`
  * ```cpp
    word_t word;
    string word_str = bitset<word_size>(get_regv(rt)).to_string();
    copy(word_str.begin(), word_str.end(), word.data());
    ```
  * `std::array<char,size>` -> `std::string`
  * `std::string` constructor
  * ```cpp
    word_t word = get_word_from_memory(addr);
    string word_str(&word[0], word_size);
    ```