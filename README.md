# zero wing firmware

hardware: https://easyeda.com/a1k0n/zerowing

requires sdcc for compiling: on linux, `sudo apt install sdcc`; on MacOS, `brew install sdcc`

`make` to compile, `make flash` to program.

flashing requires [stm8flash](https://github.com/vdudouyt/stm8flash), an
ST-Link V2 or equivalent, and a header to connect to the SWIM/NRST/GND/+5V pins
on the bottom of the board.

