#CFLAG =--model-large -DUSE_FLOATS=1
GIT_SHA1="$(shell git log --format='_%h ' -1)"
DIRTY="$(shell git diff --quiet || echo 'dirty')"
CLEAN="$(shell git diff --quiet && echo 'clean')"
CFLAGS =-DUSE_FLOATS=1 --stack-auto
CFLAGS+=-DGIT_SHA1=\"$(GIT_SHA1)$(DIRTY)$(CLEAN)\"

m.hex:m.ihx
	packihx m.ihx > m.hex

lcd_1602.rel:lcd_1602.c
	sdcc $(CFLAGS) lcd_1602.c -c

printf_large.rel:printf_large.c
	sdcc $(CFLAGS) printf_large.c -c

m.ihx:m.c lcd_1602.rel printf_large.rel
	sdcc $(CFLAGS) m.c lcd_1602.rel printf_large.rel -o m.ihx

clean:
	rm *.asm *.hex *.ihx *.lk *.lst *.map *.mem *.rel *.rst *.sym
