CC           := gcc
CFLAGS       := -Wall -O0 -march=pentium -m32 -g -fno-stack-protector -ffreestanding -fno-pic
LD           := ld
LDFLAGS      := --script=rom.ld -m elf_i386 --gc-sections
NASM         := nasm
NASM_FLAGS   := -felf
OBJCOPY      := objcopy
OBJCOPYFLAGS := --output-target=binary
TARGET       := rom.bin
SOURCES      := \
	2bmain.nasm \
	2bxcodes.nasm \
	main.c

OBJECTS := $(SOURCES:%.nasm=%.o)
OBJECTS := $(OBJECTS:%.S=%.o)
OBJECTS := $(OBJECTS:%.c=%.o)

DEPS := $(filter %.c,$(SOURCES))
DEPS := $(DEPS:%.c=%.d)

INCLUDE = -I.
CFLAGS += $(INCLUDE)

all: $(TARGET)
.PRECIOUS: $(OBJECTS) $(TARGET:.bin=.elf)

%.bin: %.elf
	@echo "objcopy  $@"
	$(Q) $(OBJCOPY) $(OBJCOPYFLAGS) $^ $@

%.elf: $(OBJECTS) rom.ld
	@echo "ld       $@"
	$(Q) $(LD) $(LDFLAGS) -o $@ $(OBJECTS)

%.o: %.c
	@echo "cc       $@"
	$(Q) $(CC) $(CFLAGS) $($@.cflags) -MMD -o $@ -c $<

%.o: %.S
	@echo "as       $@"
	$(Q) $(CC) $(CFLAGS) -o $@ -c $<

%.o: %.nasm
	@echo "nasm     $@"
	$(Q) $(NASM) -o $@ $(NASM_FLAGS) $<

.PHONY: clean
clean:
	$(Q) rm -f $(TARGET) $(TARGET:.bin=.elf) $(OBJECTS) $(DEPS)

-include $(DEPS)