#CFLAG =--model-large -DUSE_FLOATS=1
SRCS := $(wildcard src/*c)
ALL_H := $(wildcard include/*h)
OBJS := $(subst .c,.rel, $(SRCS))
GIT_SHA1="$(shell git log --format='_%h ' -1)"
DIRTY="$(shell git diff --quiet || echo 'dirty')"
CLEAN="$(shell git diff --quiet && echo 'clean')"
CFLAGS =-DUSE_FLOATS=1 --stack-auto -Iinclude
CFLAGS+=-DGIT_SHA1=\"$(GIT_SHA1)$(DIRTY)$(CLEAN)\"

m.hex:m.ihx
	packihx m.ihx > m.hex

crtstart.rel:crtstart.asm
	sdas8051 -plosgff crtstart.asm

m.ihx:m.c $(OBJS) crtstart.rel
	sdcc $(CFLAGS) m.c $(OBJS) crtstart.rel -o m.ihx

%.rel:%.c $(ALL_H)
	sdcc $(CFLAGS) $< -c -o $@

clean:
	rm src/*.hex src/*.ihx src/*.lk src/*.lst src/*.map src/*.mem src/*.rel src/*.rst src/*.sym src/*.asm \
	*.hex *.ihx *.lk *.lst *.map *.mem *.rel *.rst *.sym m.asm
