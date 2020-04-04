m.hex:m.ihx
	packihx m.ihx > m.hex

lcd_1602.rel:lcd_1602.c
	sdcc lcd_1602.c -c

m.ihx:m.c lcd_1602.rel
	sdcc m.c lcd_1602.rel -o m.ihx

clean:
	rm *.asm *.hex *.ihx *.lk *.lst *.map *.mem *.rel *.rst *.sym
