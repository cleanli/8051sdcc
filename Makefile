m.hex:m.ihx
	packihx m.ihx > m.hex

m.ihx:m.c
	sdcc m.c -o m.ihx

clean:
	rm *.asm *.hex *.ihx *.lk *.lst *.map *.mem *.rel *.rst *.sym
