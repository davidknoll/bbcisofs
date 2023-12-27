OBJS = crt0.o atapi.o directory.o fs.o hexdump.o osword.o seccache.o service.o
CL65FLAGS = -O -DDEBUG

.PHONY: all clean
all: cdfs.rom
clean:
	rm -f *~ *.map *.o *.rom

cdfs.rom: swrom.cfg $(OBJS)
	ld65 -C swrom.cfg -m cdfs.map -o cdfs.rom $(OBJS) --lib none.lib

%.o: %.c swrom.h
	cl65 $(CL65FLAGS) -t none -c $< -o $@

%.o: %.s
	cl65 $(CL65FLAGS) -t none -c $< -o $@
