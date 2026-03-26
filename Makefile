LDFLAGS := -m elf_x86_64 
NASMFLAGS := -f elf64 -g

.PHONY: build clean

build: printf_test

run: build
	./printf_test

printf.o: printf.s
	nasm $(NASMFLAGS) -l printf.lst -o printf.o printf.s
	@ # ld $(LDFLAGS) -o printf printf.o

printf_test: printf_test.c printf.o
	gcc printf_test.c printf.o -fPIE -ggdb3 -o printf_test

clean:
	rm -rf printf.o printf.lst


