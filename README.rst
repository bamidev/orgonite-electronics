Orgonite Pyramid Firmware
=========================
This project consists of two pieces of firmware.
One for the LED-driving chip, and one for the Mobius coil driving chip.
Instructions on how to physically build the pyramid will be available soon.

The LED-driving firmware is build for the atmega328p chip.
The coil-driving firmware will be build for the attiny2313, and in the future possibly a smaller chip.

Requirements
------------
In order to build these firmwares, you need `avr-gcc`, `avrdude` and Python 3 installed.
Otherwise, `make` is obviously required.
Python is required because it generates some C code that defines the random field.

Building & Flashing
-------------------
To build the LED-driving chip's firmware, go into folder `led-driver`, and run `make`.
To flash the firmware on to the chip, the fuses need to be set once with `make setfuses`, and then the firmware can be flashed with `make flash`.
Both generally need administrator level access.
However, because everyone has a different programmer, you need to set the variable `PROGRAMMER_ID` in the makefile.
Also, if you programmer is not at `/dev/ttyUSB0`, you need to set `DEV_PATH` also.

An example of building and flashing the LED-driving chip with and stk500 programmer that is accessible on `/dev/ttyUSB1`:
```
make
sudo make setfuses PROGRAMMER_ID=stk500v2 DEV_PATH=/dev/ttyUSB1
sudo make flash PROGRAMMER_ID=stk500v2 DEV_PATH=/dev/ttyUSB1
```

Checking the random field
-------------------------
The LED driver make use of a (semi-)random field that got precompiled into the chip.
It uses this 'random field' to generate a semi-random map of colors that it will 'move over'.
That is how it decides the colors to use for the LED's at a particular time.

Running `make bitmap` in `led-driver`, will compile the C code with `gcc` into a binary that produces a bitmap file, and then runs it.
The bitmap will be placed in `led-driver/bin/random_field.bmp`.
The bitmap is smaller than the field that is actually used, because otherwise the resolution would be too big.