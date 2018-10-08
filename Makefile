CC=xc8
CHIP = 18F4550
CFLAGS = --chip=${CHIP}
TRASH = main.as main.cmf main.elf main.hex main.hxl main.p1 \
        main.pre main.sdb startup.as startup.lst startup.obj \
        startup.rlf main.d main.sym

all: main.hex

main.hex: main.c
	${CC} ${CFLAGS} $^

clean:
	rm -fv ${TRASH}
