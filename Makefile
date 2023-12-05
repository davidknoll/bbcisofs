OBJS = crt0.o atapi.o hexdump.o language.o osword.o service.o

.PHONY: all clean
all: cdfs.rom
clean:
	rm -f *~ *.map *.o *.rom

cdfs.rom: swrom.cfg $(OBJS)
	ld65 -C swrom.cfg -m cdfs.map -o cdfs.rom $(OBJS) --lib none.lib

%.o: %.c swrom.h
	cl65 -t none -c $< -o $@

%.o: %.s
	cl65 -t none -c $< -o $@
