SDIRS = .
SRCS := $(foreach dir,$(SDIRS),$(wildcard $(dir)/*.c))
OBJ  := $(notdir $(SRCS:.c=.o))
LIBRARY = libgu.a

CFLAG = -march=r4000 -O3 -mgp32 -fomit-frame-pointer -mlong32 -Wall
#CFLAG = -march=r4000 -O3 -g -mgp32 -fomit-frame-pointer -mlong32 -Wall
#  -o $@ $<

$(LIBRARY): $(OBJ)
	ee-ar cru $(LIBRARY) $(OBJ)
	ee-ranlib $(LIBRARY)
	rm $(OBJ)
	mv $(LIBRARY) ../
	
%.o: %.c 
	ee-gcc $(CFLAG) -I. -I..\pspsdk -c $<

clean:
	rm $(OBJ) $(LIBRARY)
