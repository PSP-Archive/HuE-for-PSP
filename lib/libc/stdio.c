
//
// とりあえず必要な関数をチマチマ作る
// 


#include <stdio.h>
#include <fcntl.h>

#define __O_RDONLY    0x0001 
#define __O_WRONLY    0x0002 
#define __O_RDWR      0x0003 
#define __O_NBLOCK    0x0010 
#define __O_APPEND    0x0100 
#define __O_CREAT     0x0200 
#define __O_TRUNC     0x0400 
#define __O_NOWAIT    0x8000 


//==================================================================
// 
// 
//==================================================================
FILE* fopen(const char* name,const char* pmode)
{
    FILE* fp;
    int fd;
    int mode = 0;

    fp = malloc(sizeof(FILE));

    if(pmode) {
        switch(*pmode) {
          case 0: break;
          case 'w': case 'W':
            //if(PSP_Is()) mode = O_CREAT | O_TRUNC | O_WRONLY;
            //else         mode = O_CREAT | O_TRUNC | O_RDWR  ;
            mode |= __O_CREAT | __O_TRUNC | __O_RDWR;
            break;
          case 'r': case 'R':
            mode |= __O_RDONLY;
            break;
          case 'a': case 'A':
            mode |= __O_WRONLY;
            break;
        }
    }

    fd = sceIoOpen(name,mode,0777);

    if(fd<0) {
        free(fp);
        fp = 0;
    } else {
        *(int*)fp = fd;
    }
    
    return fp;
}

//==================================================================
// 
// 
//==================================================================
int fclose(FILE *fp)
{
    if(fp) {
        int fd = *(int*)fp;

        if( sceIoClose(fd) ) {
            free(fp);
            return 0;
        }
    }

    return -1;
}

//==================================================================
// 
// 
//==================================================================
size_t fread(_PTR buf,size_t size, size_t n,FILE* fp)
{
    int fd = *(int*)fp;
    return sceIoRead(fd,buf,n*size);
}

//==================================================================
// 
// 
//==================================================================
size_t fwrite(const _PTR buf, size_t size, size_t n, FILE* fp)
{
    int fd = *(int*)fp;
    return sceIoWrite(fd,buf,n*size);
}

//==================================================================
// 
// 
//==================================================================
int fseek(FILE* fp,long offset,int whence)
{
    int fd = *(int*)fp;
    int p;

    p = sceIoLseek(fd,(long long int)offset,whence);
    
    return p;
}

//==================================================================
// 
// 
//==================================================================
long ftell(FILE* fp)
{
    int fd = *(int*)fp;
    long p;
    
    p = sceIoLseek(fd,(long long)0,1); /* SEEK_CUR */

    return p;
}

