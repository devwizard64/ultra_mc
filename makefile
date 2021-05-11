TIME    := $(shell date "+%y-%m-%d %H:%M:%S")

SRC_OBJ := \
	build/src/main.o        \
	build/src/gcc.o         \
	build/src/faultasm.o    \
	build/src/fault.o       \
	build/src/apphi.o       \
	build/src/video.o       \
	build/src/applo.o       \
	build/src/mem.o         \
	build/src/math.o        \
	build/src/profiler.o    \
	build/src/printf.o      \
	build/src/printf_draw.o \
	build/src/menu.o        \
	build/src/pong.o        \
	build/src/player.o      \
	build/src/world.o       \
	build/src/chunk.o       \
	build/src/block_gfx.o

OBJ := \
	build/src/header.o  \
	build/src/main.ld.o

CC      := mips-linux-gnu-gcc
LD      := mips-linux-gnu-ld
CPP     := mips-linux-gnu-cpp
OBJCOPY := mips-linux-gnu-objcopy
CC      += -march=vr4300 -mfix4300 -mfp32 -mno-abicalls -mno-check-zero-division
CC      += -fno-PIC -ffreestanding -fno-common -fno-zero-initialized-in-bss
CC      += -fno-toplevel-reorder -G 0

FLAG    := -I ultra/include -I include -I .
CC      += $(FLAG) -D _TIME="\"$(TIME)\"" -Wall -Wextra -Wpedantic -O2
CPP     += $(FLAG)

build/app.z64: build/app.elf | exe/build
	$(OBJCOPY) --pad-to 0x00101000 --gap-fill 0xFF -O binary $< $@
	exe/build/crc $@

build/app.elf: build/make/main.ld $(OBJ)
	$(LD) -Map $(@:.elf=.map) -o $@ -T $<

build/make/main.ld: make/main.ld | build/make
	$(CPP) -MMD -MP -P -o $@ $<

build/src/main.ld.o: $(SRC_OBJ)
	$(LD) -L ultra/lib -r -o $@ $(SRC_OBJ) -l ultra_rom

build/%.o: %.c
	$(CC) -MMD -MP -c -o $@ $<

build/%.o: %.S
	$(CC) -MMD -MP -c -o $@ $<

build/%.d: %.c
	$(CC) -MM -MG -MP -MF $@ -MT $(@:.d=.o) $<

build/%.d: %.S
	$(CC) -MM -MG -MP -MF $@ -MT $(@:.d=.o) $<

build/%.h: %.png | exe/build
	exe/build/texture $@ $^

dirs = $(foreach d,$(wildcard $1*),$(call dirs,$d/,$2) \
	$(filter $(subst *,%,$2),$d))
obj = $(call dirs,src/,$1) $(call dirs,data/,$1)
BUILD_OBJ := $(call obj,*.c *.S *.bin *.png *.ply)
$(foreach f,$(BUILD_OBJ),$(eval $f: | $(addprefix build/,$(dir $f))))
build/make $(sort $(addprefix build/,$(dir $(BUILD_OBJ)))):
	mkdir -p $@

exe/build:
	make -C exe

clean:
	rm -rf build

print-%:
	$(info $* = $(flavor $*): [$($*)]) @true

-include build/make/main.d
include $(addsuffix .d,$(basename $(SRC_OBJ)))

.PHONY: clean
