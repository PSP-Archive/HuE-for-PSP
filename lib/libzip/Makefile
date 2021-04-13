BINARY = out

OBJECTS = ioapi.o unzip.o zlibFileMemory.o zlibInterface.o 

all: $(BINARY)

$(BINARY): $(OBJECTS)
	ee-ar rcs unziplib.a $(OBJECTS)

INCS = -I..\zlib

%.o : %.c
	ee-gcc $(INCS) -march=r4000 -O3 -fomit-frame-pointer -g -mgp32 -mlong32 -c $< -o $@

clean:
	del /s /f *.o *.map
