#CFLAG =--model-large -DUSE_FLOATS=1
CFLAG =-DUSE_FLOATS=1 --stack-auto

m.hex:m.ihx
	packihx m.ihx > m.hex

lcd_1602.rel:lcd_1602.c
	sdcc $(CFLAG) lcd_1602.c -c

printf_large.rel:printf_large.c
	sdcc $(CFLAG) printf_large.c -c

m.ihx:m.c lcd_1602.rel printf_large.rel
	sdcc $(CFLAG) m.c lcd_1602.rel printf_large.rel -o m.ihx

clean:
	rm *.asm *.hex *.ihx *.lk *.lst *.map *.mem *.rel *.rst *.sym
