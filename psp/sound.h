// Sound.h

#if !defined(INC_SOUND_H)
#define INC_SOUND_H

// global variables:

// global functions:
int InitSound(int Mode);
void TrashSound();
void write_psg(int ch);
void enable_sound(int mode);


// Copy from HuGo ?
#define PSG_FREQ_LSB_REG        2 /* lower 8 bits of 12 bit frequency */
#define PSG_FREQ_MSB_REG        3 /* actually most significant nibble */
#define PSG_DATA_INDEX_REG      6
#define PSG_DDA_REG             4
#define PSG_DDA_ENABLE          0x80 /* bit 7 */
#define PSG_DDA_DIRECT_ACCESS   0x40 /* bit 6 */
#define PSG_DDA_VOICE_VOLUME    0x1F /* bits 0-4 */
#define PSG_DIRECT_ACCESS_BUFSIZE 1024

#define CD_NONE 0
#define CD_HCD 1


int wavoutInit(void);
int wavoutClose(void);
void dbgSound(void);

int SoundProcessing(int control);
int  SoundStabilizer(void);


#endif // !defined(INC_SOUND_H)
