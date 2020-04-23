#CFLAG =--model-large -DUSE_FLOATS=1
GIT_SHA1="$(shell git log --format='_%h ' -1)"
DIRTY="$(shell git diff --quiet || echo 'dirty')"
CLEAN="$(shell git diff --quiet && echo 'clean')"
CFLAGS =-DUSE_FLOATS=1 --stack-auto
CFLAGS+=-DGIT_SHA1=\"$(GIT_SHA1)$(DIRTY)$(CLEAN)\"

m.hex:m.ihx
	packihx m.ihx > m.hex

ui.rel:ui.c
	sdcc $(CFLAGS) ui.c -c

music.rel:music.c
	sdcc $(CFLAGS) music.c -c

lcd_1602.rel:lcd_1602.c
	sdcc $(CFLAGS) lcd_1602.c -c

stc12_drv.rel:stc12_drv.c
	sdcc $(CFLAGS) stc12_drv.c -c

crtstart.rel:crtstart.asm
	sdas8051 -plosgff crtstart.asm

m.ihx:m.c lcd_1602.rel crtstart.rel stc12_drv.rel music.rel ui.rel
	sdcc $(CFLAGS) m.c crtstart.rel lcd_1602.rel stc12_drv.rel music.rel ui.rel -o m.ihx

clean:
	rm *.hex *.ihx *.lk *.lst *.map *.mem *.rel *.rst *.sym lcd_1602.asm  m.asm  stc12_drv.asm music.asm  ui.asm
