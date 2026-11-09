.RECIPEPREFIX = >

CC       = clang
OBJCOPY  = llvm-objcopy
SIZE     = llvm-size

TARGET   = --target=arm-none-eabi
CPU      = -mcpu=cortex-m4
FPU      = -mfpu=fpv4-sp-d16 -mfloat-abi=hard
MCU      = $(TARGET) $(CPU) -mthumb $(FPU)

CFLAGS   = $(MCU) -ffreestanding -Wall -Wextra -O2 -Iinclude
LDFLAGS  = $(MCU) -fuse-ld=lld -nostdlib -T linker.ld

SRCS       = startup.c src/pwm.c app/main.c
OBJS       = $(SRCS:.c=.o)
TARGET_BIN = pwm-demo

all: $(TARGET_BIN).elf $(TARGET_BIN).bin

%.o: %.c
> $(CC) $(CFLAGS) -c $< -o $@

$(TARGET_BIN).elf: $(OBJS) linker.ld
> $(CC) $(LDFLAGS) $(OBJS) -o $@
> $(SIZE) $@

$(TARGET_BIN).bin: $(TARGET_BIN).elf
> $(OBJCOPY) -O binary $< $@

clean:
> rm -f $(OBJS) $(TARGET_BIN).elf $(TARGET_BIN).bin

.PHONY: all clean
