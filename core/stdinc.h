#if !defined(INC_STDINC_H)
#define INC_STDINC_H

#define LSB_FIRST
#define HuC6280
//#define SOUND

//#define BOOL int
#define VARCALL
#define TRUE    1
#define FALSE   0

#define min(a,b) (((a)<(b))?(a):(b))
#define max(a,b) (((a)>(b))?(a):(b))

//typedef unsigned char byte;
typedef unsigned char BYTE;
typedef unsigned int DWORD;
typedef unsigned short WORD;
typedef unsigned int BOOL;

#define TRACE
#define TRACE0
#define TRACE1
#define TRACE2
#define TRACE3
#define NDEBUG

#define REALROM_SIZE   (3*1024*1024) // support sf2'ce
//#define PALETTE_SIZE   65536

int exit_check(void);

//extern unsigned char xbuf[WIDTH*HEIGHT+8];
//extern unsigned char REALROM[1*1024*1024];

#endif // !defined(INC_STDINC_H)
