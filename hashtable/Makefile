CC := gcc
AS := nasm
ASFLAGS := -f elf64
CFLAGS := $(FLAGS) -Wall -Wextra -D _GNU_SOURCE -Iinclude -march=native

LDFLAGS := -lm
BUILD_DIR := build
SANITIZER_FLAGS := -fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr

DBG_CFLAGS := -D _DEBUG -ggdb3 -O0 -Wall -Wextra -Waggressive-loop-optimizations -Wmissing-declarations -Wcast-align -Wcast-qual -Wchar-subscripts  -Wconversion -Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness -Wformat=2 -Winline -Wlogical-op -Wopenmp-simd -Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion -Wstrict-overflow=2 -Wsuggest-attribute=noreturn -Wsuggest-final-methods -Wsuggest-final-types -Wswitch-default -Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused -Wuseless-cast -Wvariadic-macros -Wno-missing-field-initializers -Wno-narrowing -Wno-varargs -Wstack-protector -fcheck-new -fstack-protector -fstrict-overflow -fno-omit-frame-pointer -Wlarger-than=8192 -Wstack-usage=8192 -pie -fPIE -Werror=vla $(SANITIZER_FLAGS)

REL_CFLAGS := -DNDEBUG -O3 -ggdb3 -g -fno-omit-frame-pointer

ifeq ($(DEBUG), 1)
    CFLAGS += $(DBG_CFLAGS)
else
    CFLAGS += $(REL_CFLAGS)
endif

ifndef TESTER_HASHING_ALGO
	TESTER_HASHING_ALGO := src/ht_chains.c
endif

TESTER_SRC := src/tester_main.c $(TESTER_HASHING_ALGO) src/list.c crc32.c 
TESTER_ASM := src/my_strlen.S
TESTER_OBJ := $(TESTER_SRC:%.c=$(BUILD_DIR)/%.o)
TESTER_APP := $(BUILD_DIR)/tester

INCPDSRC := $(HEAPSLIB_SRC) $(TESTER_SRC)
incpd := $(INCPDSRC:%.c=$(BUILD_DIR)/%.d)

OBJFILES := $(TESTER_OBJ)
ASM_OBJFILES := $(TESTER_ASM:%.S=$(BUILD_DIR)/%.o)
OBJDIRS := $(sort $(dir $(OBJFILES) $(ASM_OBJFILES)))


.PHONY: build clean run test document build_test objdirs

.PHONY: build testgens build_tester

build: build_tester
	mkdir -p reports

str_streamhash: str_streamhash.o crc32.o

# testgens: genrandom uint_streamhash float_streamhash str_streamhash gen_tests.sh

gen_tests: testgens
	./gen_tests.sh
	
gen_all_tests: gen_tests

build_tester: $(TESTER_APP) $(HEAPSLIB_STATIC)
	mkdir -p reports

run: $(TESTER_APP)
	./$(TESTER_APP)

$(ASM_OBJFILES): $(BUILD_DIR)/%.o: %.S
	@ mkdir -p $(OBJDIRS)
	$(AS) $(ASFLAGS) $< -o $@

$(OBJFILES): $(BUILD_DIR)/%.o: %.c
	@ mkdir -p $(OBJDIRS)
	$(CC) $(CFLAGS) -MP -MMD -c $< -o $@

$(TESTER_APP): $(TESTER_OBJ) $(ASM_OBJFILES)
	@ mkdir -p $(OBJDIRS)
	$(CC) $(CFLAGS) $(LDFLAGS) $(TESTER_OBJ) $(ASM_OBJFILES) -o $@

clean:
	rm -rf build genrandom uint_streamhash float_streamhash str_streamhash *.o || true

-include $(incpd)
