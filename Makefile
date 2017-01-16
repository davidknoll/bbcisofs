.PHONY: all clean
all: iso9660.rom
clean:
	rm -f *.hlr *.lst *.map *.rel *.rom *.rst *.s19

%.rom: %.s19
	srec_cat -Output $@ -Binary $< -offset -0x8000 -Fill 0xFF 0x0000 0x4000

iso9660.s19: header.rel defs.rel osword.rel starhelp.rel
	aslink -mosu -b SWROM=0x8000 $@ $^

%.rel: %.asm
	as6500 -glo $<
