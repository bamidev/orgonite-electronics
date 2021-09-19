############################
# Programmer Configuration #
############################
# If you use a different programmer, change this, or call `make flash PROGRAMMER_ID=myid`.
# The id should be accepted by `avrdude -c myid`.
PROGRAMMER_ID := avrisp2
# If your programmer is assigned a different filepath than `/dev/USB0`, you can set it here:
DEV_PATH := /dev/ttyUSB0



########################
# Other Configurations #
########################
# For setting CHIP_ID, see https://gcc.gnu.org/onlinedocs/gcc/AVR-Options.html
CHIP_ID := attiny45
# The clock speed of the chip.
CLOCK_SPEED := 16000000
# The compiler command to use.
CC := avr-gcc



# The compiler flags to use.
CFLAGS := -std=c11
AVR_CFLAGS := -O3 -mmcu=$(CHIP_ID) -DF_CPU=$(CLOCK_SPEED)UL 
# The firmware file path
OUT_FILE := bin/firmware.elf

#########################
# End of Configurations #
#########################



SRC_FILES := $(shell find src -name "*.c")
OBJ_FILES := $(SRC_FILES:.c=.o)



firmware: $(OUT_FILE)

clean:
	rm -f bin/firmware.elf $(OBJ_FILES) bin/*

flash: $(OUT_FILE)
	avrdude -p $(CHIP_ID) -c $(PROGRAMMER_ID) -P $(DEV_PATH) -U flash:w:$(OUT_FILE)

setfuses:
# Sets the fuses to:
# * Use external clock input
# * Use brown out detection circuit for 4.3V
# * Slow startup time.
	avrdude -p $(CHIP_ID) -c $(PROGRAMMER_ID) -P $(DEV_PATH) -U lfuse:w:0xE0:m
	avrdude -p $(CHIP_ID) -c $(PROGRAMMER_ID) -P $(DEV_PATH) -U hfuse:w:0xD9:m

$(OUT_FILE): $(OBJ_FILES)
	$(CC) $(CFLAGS) $(AVR_CFLAGS) -o $(OUT_FILE) $(OBJ_FILES)

src/%.o: src/%.c
	$(CC) $(CFLAGS) $(AVR_CFLAGS) -c $< -o $@
