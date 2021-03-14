all: assembler

assembler: assembler.cpp
	g++ ./assembler.cpp -o assembler

clean:
	rm assembler

test:
	./assembler sample.asm