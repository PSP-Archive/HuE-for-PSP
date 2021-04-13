VERSION = 0.70

CC  = ee-gcc
LD  = ee-ld
PAT = outpatch
E2P = elf2pbp

DEF = SOUND GPU_ENABLE MP3
INC = core psp lib lib/pspsdk lib/libfont lib/zlib lib/libzip

DEFS = $(addprefix -D,$(DEF))
INCS = $(addprefix -I,$(INC))

LIBS  =-Llib
LIBS += -lc
LIBS += -lgu
LIBS += -lmad
LIBS += -lz
LIBS += -lfont
LIBS += -lpng
LIBS += lib/unziplib.a

CFLAGS   = $(INCS) $(DEFS) -march=r4000 -O3 -mgp32 -fomit-frame-pointer -mlong32 -Wall -c -o $@ $<
LDFLAGS  = -s -M -Ttext 8900000 -q -o out

MP3_OBJ = obj/mp3.o

PSPOBJ = obj/filer.o obj/main.o obj/menu.o obj/pg.o obj/hue_joy.o obj/hue_image.o obj/hue_fio.o obj/sound.o obj/zlibpsp.o obj/psp_main.o

COREOBJ= obj/m6502.o obj/pce.o obj/vce.o obj/vdc.o  obj/psg.o obj/cd.o  obj/acd.o obj/joy.o obj/irq.o obj/timer.o


OBJECTS = $(PSPOBJ) $(COREOBJ) $(SOUND_OBJ) $(MP3_OBJ) 


all: $(OBJECTS)
	$(LD) $(LDFLAGS) $(OBJECTS) $(LIBS)  > obj/map.txt
	$(PAT) USERPROG
	$(E2P) outp "HuE for PSP $(VERSION)" "psp/ICON0.PNG"
	@rm out outp

%.o: %.c  psp/pce.h
	$(CC) $(CFLAGS)

obj/%.o: core/%.c core/pceregs.h
	$(CC) $(CFLAGS)

obj/%.o: psp/%.c
	$(CC) $(CFLAGS)

obj/%.o: psp/%.s
	$(CC) $(CFLAGS)

clean:
	rm eboot.pbp obj/*.*

