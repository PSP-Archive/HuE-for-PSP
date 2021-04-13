BINARY = out

# library object files common to compression and decompression
OBJECTS = bit.o decoder.o fixed.o frame.o huffman.o layer3.o layer12.o stream.o timer.o version.o synth.o

LIBRARY = libmad.a

all: $(LIBRARY)

$(LIBRARY): $(OBJECTS)
	ee-ar cru $(LIBRARY) $(OBJECTS)
	ee-ranlib $(LIBRARY)

%.o : %.c
	ee-gcc -march=r4000 -DPSP -DNDEBUG -DFPM_DEFAULT -O3 -fomit-frame-pointer -mgp32 -mlong32 -c $< -o $@

%.o : %.s
	ee-gcc -march=r4000 -mgp32 -c -xassembler -O -o $@ $<

clean:
	del /f *.o *.map *.a
