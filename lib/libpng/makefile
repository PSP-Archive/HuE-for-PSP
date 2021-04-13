SDIRS = .
SRC_S := $(foreach dir,$(SDIRS),$(wildcard $(dir)/*.s))
OBJ_S := $(notdir $(SRC_S:.s=.o))
SRC_C := $(foreach dir,$(SDIRS),$(wildcard $(dir)/*.c))
OBJ_C := $(notdir $(SRC_C:.c=.o))
OBJ = $(OBJ_S) $(OBJ_C)


LIBRARY = libpng.a

DEFINE = -DPNG_USER_CONFIG

CFLAG = $(DEFINE) -march=r4000 -O3 -mgp32 -fomit-frame-pointer -mlong32 -Wall 
#CFLAG = -march=r4000 -O3 -g -mgp32 -fomit-frame-pointer -mlong32 -Wall
INCS  = -I. -I..\zlib

#  -o $@ $<

$(LIBRARY): $(OBJ)
	ee-ar cru $(LIBRARY) $(OBJ)
	ee-ranlib $(LIBRARY)
	mv $(LIBRARY) ../
	rm *.o
	
%.o: %.c 
	ee-gcc $(CFLAG) $(INCS) -c $<

%.o: %.s
	ee-gcc $(CFLAG) -c $<

clean:
	rm $(OBJ) $(LIBRARY)
