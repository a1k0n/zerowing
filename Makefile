OBJS = main.o
CC = sdcc
CFLAGS = -lstm8 -mstm8 --std-sdcc11
CFILES = main.c


all:: main.elf

main.elf:
	$(CC) $(CFLAGS) --out-fmt-elf -o $@ $(CFILES)

flash: main.elf
	openocd -f interface/stlink.cfg -f target/stm8s003.cfg -c "program main.elf verify reset exit"

reformat:
	clang-format -style=Google -i $(CFILES)

clean:
	rm -f main.asm main.cdb main.elf main.lk main.lst main.map main.rel main.rst main.sym
