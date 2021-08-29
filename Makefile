############################
# Programmer Configuration #
############################
# If you use a different programmer, change this, or call `make flash PROGRAMMER_ID=myid`.
# The id should be accepted by `avrdude -c myid`.
PROGRAMMER_ID := avrisp2
# If your programmer is assigned a different filepath than `/dev/USB0`, you can set it here:
DEV_PATH := /dev/USB0



########################
# Other Configurations #
########################
# For setting CHIP_ID, see https://gcc.gnu.org/onlinedocs/gcc/AVR-Options.html
CHIP_ID := atmega328p
# The clock speed of the chip.
CLOCK_SPEED := 8000000
# The compiler command to use.
CC := avr-gcc



# The compiler flags to use.
CFLAGS := -std=c11 -mmcu=$(CHIP_ID) -DF_CPU=$(CLOCK_SPEED)UL
# The firmware file path
OUT_FILE := bin/firmware.elf

#########################
# End of Configurations #
#########################



SRC_FILES := $(shell find src -name "*.c")
OBJ_FILES := $(SRC_FILES:.c=.o) gen/random_field.o



firmware: $(OUT_FILE)

clean:
	rm -f bin/firmware.elf $(OBJ_FILES) gen/random_field.c

flash: $(OUT_FILE)
	avrdude -p attiny2313 -c $(PROGRAMMER_ID) -P /dev/ttyUSB0 -U flash:w:$(OUT_FILE)

$(OUT_FILE): $(OBJ_FILES)
	$(CC) $(CFLAGS) -o $(OUT_FILE) $(OBJ_FILES)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

gen/%.o: gen/%.c
	$(CC) $(CFLAGS) -I ./src -c $< -o $@

gen/random_field.c:
	python3 src/generate_field.py > gen/random_field.c