OBJS = main.o
CC = sdcc
CFLAGS = -lstm8 -mstm8 --std-sdcc11 --opt-code-speed
CFILES = main.c


all:: main.hex

main.hex: $(CFILES)
	# $(CC) $(CFLAGS) --out-fmt-elf -o $@ $(CFILES)
	$(CC) $(CFLAGS) -o $@ $(CFILES)

flash: main.hex
	# openocd -f interface/stlink.cfg -f target/stm8s003.cfg -c "program main.elf verify reset exit"
	stm8flash -c stlinkv2 -p stm8s003f3 -w main.hex

flashopt:
	# flash the option bytes, which sets the AFR* peripheral configuration bits
	stm8flash -c stlinkv2 -p stm8s003f3 -s opt -w opt.hex

reformat:
	# requires very recent clang-format for AlignConsecutiveMacros
	clang-format -style="{BasedOnStyle: Google, AlignConsecutiveAssignments: true, AlignConsecutiveDeclarations: true, AlignConsecutiveMacros: true}" -i $(CFILES)

clean:
	rm -f main.asm main.cdb main.elf main.lk main.lst main.map main.rel main.rst main.sym main.hex
