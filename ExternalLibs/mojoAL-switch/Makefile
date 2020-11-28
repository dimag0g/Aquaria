src = $(wildcard *.c)
obj = $(src:.c=.o)
PORTLIBS_PATH = ${DEVKITPRO}/portlibs/switch/
CCFLAGS = -I$(PORTLIBS_PATH)/include
CC  = ${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-gcc $(CCFLAGS)
AR  = ${DEVKITPRO}/devkitA64/bin/aarch64-none-elf-ar

LDFLAGS = 

mojoal.a: $(obj)
	ar rc $@ $^

.PHONY: clean
clean:
	rm -f $(obj) myprog