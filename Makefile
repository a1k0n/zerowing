OBJS = main.o
CC = sdcc
CFLAGS = -lstm8 -mstm8 --std-sdcc11


all:: main.elf

main.elf:
	$(CC) $(CFLAGS) --out-fmt-elf -o $@ main.c

flash: main.elf
	openocd -f interface/stlink.cfg -f target/stm8s003.cfg -c "program main.elf verify reset exit"

clean:
	rm -f main.asm main.cdb main.elf main.lk main.lst main.map main.rel main.rst main.sym
