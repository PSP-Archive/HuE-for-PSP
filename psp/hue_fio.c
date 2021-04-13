//#define NOZIP

// file i/o //
#include "stdinc.h"
#include "syscall.h"
#include "pg.h"
#include "string.h"
#include "main.h"
#include "pce.h"

#include <stdio.h>

#define _O_RDONLY    0x0001
#define _O_WRONLY    0x0002
#define _O_RDWR      0x0003
#define _O_NBLOCK    0x0010
#define _O_APPEND    0x0100
#define _O_CREAT     0x0200
#define _O_TRUNC     0x0400
#define _O_NOWAIT    0x8000

#define OPEN_WRITE   (PSP_Is()?(_O_CREAT|_O_TRUNC|_O_WRONLY):(_O_CREAT|_O_TRUNC|_O_RDWR))
#define OPEN_READ    (_O_RDONLY)


#include "deflateInterface.h"
#define    ZLIB_LEVEL    6    // 圧縮レベル

#define    STATE_MAX    10

extern void bank_set(byte P,byte V);


char* ConfigName(void);
char* StateName(int);
char* cdName(int track);
char* Mp3Name(int track);
char* IsoName(int track);

static char filename[512];

//
char* cdName(int track)
{
    char *p;
    if(track<0) track=0;
    
    strcpy(filename,eConf.cdrom);
    p=strrchr(filename,'/');
    if(p) {
        p[1] = 0x30 + track/10;
        p[2] = 0x30 + track%10;
        p[3] = '.';
        p[4] = 0;
        return filename;
    }

    return 0;
}

char *Mp3Name(int track)
{
    char*p = cdName(track);
    if(p) {
        strcat(p,"mp3");
        return p;
    }
    return 0;
}

char *IsoName(int track)
{
    char*p = cdName(track);
    if(p) {
        strcat(p,"iso");
        return p;
    }
    return 0;
}

// config file name
char* ConfigName(void)
{
    strcpy(filename,eRun.hue_path);
    strcat(filename,"hue.cfg");
    return filename;
}

char* WramName(void)
{
    strcpy(filename,eRun.hue_path);
    strcat(filename,"wram.dat");
    return filename;
}


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
char* StateName(int num)
{
    char * p;
    strcpy(filename,eRun.cart_name);
    
    if((p=strrchr(filename,'.'))) {
        *(p+1)='s';
        *(p+2)=0x30+(num/10)%10;
        *(p+3)=0x30+(num   )%10;
        *(p+4)=0;
        return filename;
    }

    return 0;
}

//--------------------------------------------------------------------------
// 圧縮対応
//--------------------------------------------------------------------------
int saveWRAM(void)
{
    int fd;
    char* name = WramName();

    if((fd=sceIoOpen(name,OPEN_WRITE,0777))) {
        sceIoWrite(fd,WRAM,sizeof(WRAM));
        sceIoClose(fd);
        return 0;
    }
    return -1;
}

//--------------------------------------------------------------------------
// 圧縮対応
//--------------------------------------------------------------------------
int loadWRAM(void)
{
    int fd;
    char* name=WramName();

    if((fd=sceIoOpen(name,OPEN_READ,0777))) {
        sceIoRead(fd,WRAM,sizeof(WRAM));
        sceIoClose(fd);
        return 0;
    }
    return -1;
}

#include "zlibpsp.h"

#define TAG_CMP   0x80000000 // tag info compressed
#define TAG_IMG   0x00000008 // tag info image
#define TAG_IO    0x00000010 // tag info io
#define TAG_CD    0x000000cd // tag info cd
#define TAG_END   0x000000ed // tag info end

// データ保存は全て
// int tag;  圧縮フラグ+データ種別
// int size; データサイズ
// char []   データ領域
// 以上の形態として扱う
//
// これによりセーブデータの独立性を高める
//
int StateSave(int slot,void* buf,int size)
{
    int fd,len;
    char* name = StateName(slot);

    if((fd=sceIoOpen(name,OPEN_WRITE,0777))>=0) {
        len = sceIoWrite(fd,buf,size);
        sceIoClose(fd);
        return (len==size);
    }
    return 0;
}

int StateLoad(int slot,void* buf,int size)
{
    int fd,len;
    char* name = StateName(slot);

    if((fd=sceIoOpen(name,OPEN_READ,0777))>=0) {
        len = sceIoRead(fd,buf,size);
        sceIoClose(fd);
        
        return (len==size);
    }

    return 0;
}


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void clockDown(void)
{
    pgSetClock(222);
}

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
void changeClock(void)
{
    int c;
    
    switch(eConf.clock) {
      case 2: c=222; break;
      case 1: c=266; break;
      default:
      case 0: c=333; break;
    }

    pgSetClock(c);
}


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
int LoadConfig(void)
{
    char *name = ConfigName();
    int len,ret=0;
    int fd;

    if((fd=sceIoOpen(name,OPEN_READ,0777))>=0){
        len = sceIoRead(fd,&pConf,sizeof(pConf));
        sceIoClose(fd);
        if(len==sizeof(eConf)) {
            memcpy(&eConf,&pConf,sizeof(pConf));
            ret=1;
        }
    }

    return ret;
}

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
int SaveConfig(void)
{
    int len;
    int fd;
    
    char *name = ConfigName();
    if(memcmp(&pConf,&eConf,sizeof(EmuConfig))!=0) {

        if((fd=sceIoOpen(name,OPEN_WRITE,0777))>=0) {
            len = sceIoWrite(fd,&eConf,sizeof(eConf));
            sceIoClose(fd);
        }
    }
    return 0;
}


//
byte * REALROM = 0;
// unsigned char REALROM[3*1024*1024];

#include "filer.h"

#ifndef NOZIP /* zip対応 : 一時中止 */
#ifndef WIN32
#include "zlibInterface.h"

// 宣言
int funcUnzipCallback(int nCallbackId,
                      unsigned long ulExtractSize,
		      unsigned long ulCurrentPosition,
                      const void *pData,
                      unsigned long ulDataSize,
                      unsigned long ulUserData)
{
    const char *pszFileName;
    const unsigned char *pbData;

    switch(nCallbackId) {
      case UZCB_FIND_FILE:
        {
            DWORD * RomSz = (DWORD*)ulUserData;

            //mh_print(0,0,"UZCB_FIND_FILE",-1);
            //pgScreenFlipV();
            //pgWaitVn(100);
            
            pszFileName = (const char *)pData;
            
            if(getExtId(pszFileName)==EXT_PCE) {
                // 展開領域を超えるデータはパスする

                if(REALROM) { free(REALROM); }
                REALROM = malloc(ulExtractSize);

                if(!REALROM) {
                    return UZCBR_PASS;
                }
                
                *RomSz = ulExtractSize;
                return UZCBR_OK;
            }
        }
        return UZCBR_PASS;
        break;

      case UZCB_EXTRACT_PROGRESS:
        pbData = (const unsigned char *)pData;
        memcpy(&REALROM[ulCurrentPosition],pbData,ulDataSize);
        return UZCBR_OK;
        break;

      default: // unknown...
        /*
         現状のバージョンではここには絶対にこない(と思う)が、FAILSAFEのために何か
         デバッグコードをいれておくと良いかも…
         */
        break; 
    }
    return UZCBR_CANCEL;
}
#endif
#endif /* NOZIP :  zip一時停止*/

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
int CartLoad(char *name)
{
    FILE* fp; //int fd;
    int ret=-1;
    int ext = getExtId(name);
    int size,seek;

    pgSetClock(333);

    if(REALROM && strcmp(name,eRun.cart_name)==0) {
        ret = 1;
    }
    else {
        switch(ext) {
          case EXT_PCE:
            if((fp=fopen(name,"r"))) {
                fseek(fp,0,SEEK_END);
                size = ftell(fp);
                
                seek = size & 0x1fff;
                fseek(fp,0,SEEK_SET);
                
                if(REALROM) { free(REALROM); }
                REALROM = (byte*)malloc(size);
                
                if(REALROM) {
                    fseek(fp,seek,SEEK_SET);
                    ROM = REALROM;
                    ROM_size = fread(REALROM,1,size,fp);
                    ROM_size/=0x2000;
                    strcpy(eRun.cart_name,name);
                    ret = 1;
                } else {
                    ROM = 0;
                    ROM_size = 0;
                    strcpy(eRun.cart_name,"ms0:/no.pce");
                    ret = 0;
                }
                fclose(fp); // pgf_Close(fd);
            }
            break;
#ifndef NOZIP
          case EXT_ZIP: {
              int extract;
              Unzip_setCallback(funcUnzipCallback);
              extract = Unzip_execExtract(name,&ROM_size);
              
              if(extract==UZEXR_OK && ROM_size>0) {
                  ROM = REALROM + (ROM_size&0x00001fff);
                  ROM_size&=~0x1fff;
                  ROM_size=ROM_size/0x2000;
                  strcpy(eRun.cart_name,name);
                  ret=1;
              } else {
                  mh_print(0, 0,"Can not read ROM file",-1);
                  pgScreenFlipV();
                  pgWaitVn(60);
                  ret=0;
              }
          }
            break;
#endif /* NOZIP */
          default:
          case EXT_UNKNOWN:
            ret = 0;
            return 0;
        }
    }

    changeClock();
    
    return ret;
}

