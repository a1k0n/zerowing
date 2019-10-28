# zero wing firmware

hardware: https://easyeda.com/a1k0n/zerowing

requires sdcc for compiling: on linux, `sudo apt install sdcc`; on MacOS, `brew install sdcc`

requires latest OpenOCD for STM-8 support, as neither *apt* nor *homebrew* currently have a late enough version:

```
$ git clone git://repo.or.cz/openocd.git
$ cd openocd
$ ./bootstrap && ./configure && make && make install
```

`make` to compile, `make flash` to program.

you'll need an ST-Link V2 and a programming harness that clips onto the side of the board: I use a chip clip and a 4-pin .1" header.

