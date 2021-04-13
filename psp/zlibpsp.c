#include "deflateInterface.h"
#include "zlib.h"

//#define INBUFSIZ   1024         /* ���̓o�b�t�@�T�C�Y�i�C�Ӂj */
//#define OUTBUFSIZ  1024         /* �o�̓o�b�t�@�T�C�Y�i�C�Ӂj */
#define INBUFSIZ   128         /* ���̓o�b�t�@�T�C�Y�i�C�Ӂj */
#define OUTBUFSIZ  1024         /* �o�̓o�b�t�@�T�C�Y�i�C�Ӂj */

void psp_progress(int cur,int max,int color)
{
    int per;
    char dec[3];

    if(max==0) return;

    /* per = 0-320 */
    per = cur*320 / max;

    pgDrawFrame(80,260,402,270,-1);
    pgFillBox(81,261,81+per,269,color);

    per = (per*10) / 32;
    dec[0] = (per/100) + 0x30;
    dec[1] = (per/10)%10+ 0x30;
    dec[2] = (per%10)  + 0x30;
    dec[3] = 0;
      
    mh_print(403,260,dec,-1);
    pgScreenFlip();
}

int do_compress_m2m(char *pIn,int nInSize,char *pOut,int nOutSize,int level)          /* ���k */
{
	int flush, status;
	z_stream z;                     /* ���C�u�����Ƃ��Ƃ肷�邽�߂̍\���� */

	char *pI = pIn;
	char *pO = pOut;
	int nUsedSizeIn  = 0;
	int nUsedSizeOut = 0;

	if((level < 0) || (9 < level))	level = 6;
	if(nOutSize < 0)	return DZEXR_OUTBUFFERROR;

    /* ���ׂẴ������Ǘ������O�ōs�� */
    z.zalloc = Z_NULL;
    z.zfree  = Z_NULL;
    z.opaque = Z_NULL;

    /* ������ */
    /* ��2�����͈��k�̓x���B0�`9 �͈̔͂̐����ŁC0 �͖����k */
    if (deflateInit(&z, level) != Z_OK){return DZEXR_INITERROR;}

    z.avail_in  = 0;            /* ���̓o�b�t�@���̃f�[�^�̃o�C�g�� */
    z.next_out  = pO;           /* �o�̓|�C���^ */
    z.avail_out = (nOutSize - nUsedSizeOut >= OUTBUFSIZ) ? OUTBUFSIZ : nOutSize - nUsedSizeOut;    /* �o�̓o�b�t�@�̃T�C�Y */
	pO           += z.avail_out;
	nUsedSizeOut += z.avail_out;

    /* �ʏ�� deflate() �̑�2������ Z_NO_FLUSH �ɂ��ČĂяo�� */
    flush = Z_NO_FLUSH;

    while(1)
	{
        if (z.avail_in == 0)
		{  /* ���͂��s����� */
            z.next_in  = pI;  /* ���̓|�C���^����̓o�b�t�@�̐擪�� */
            z.avail_in = (nInSize - nUsedSizeIn >= INBUFSIZ) ? INBUFSIZ : nInSize - nUsedSizeIn;	/* �f�[�^��ǂݍ��� */
			pI          += z.avail_in;
			nUsedSizeIn += z.avail_in;

            /* ���͂��Ō�ɂȂ����� deflate() �̑�2������ Z_FINISH �ɂ��� */
            if (z.avail_in < INBUFSIZ) flush = Z_FINISH;
        }
        status = deflate(&z, flush); /* ���k���� */

        /* ���s�󋵕\�� */
        psp_progress(nUsedSizeIn,nInSize,0x001f<<10);
        
        if (status == Z_STREAM_END) break; /* ���� */
        if (status != Z_OK) {   /* �G���[ */
            return DZEXR_DEFLATEERROR;
        }
        if (z.avail_out == 0)
		{ /* �o�̓o�b�t�@���s����� */
            z.next_out  = pO;        /* �o�̓|�C���^�����ɖ߂� */
            z.avail_out = (nOutSize - nUsedSizeOut >= OUTBUFSIZ) ? OUTBUFSIZ : nOutSize - nUsedSizeOut; /* �o�̓o�b�t�@�c�ʂ����ɖ߂� */
			pO           += z.avail_out;
			nUsedSizeOut += z.avail_out;

			if(z.avail_out == 0)	return DZEXR_OUTBUFFERROR;
        }
    }

    /* �c���f���o�� */
    nUsedSizeOut -= z.avail_out;

    /* ��n�� */
    if (deflateEnd(&z) != Z_OK)
	{
        return DZEXR_DEFLATEERROR;
    }

	return nUsedSizeOut;
}

int do_decompress_m2m(char *pIn,int nInSize,char *pOut,int nOutSize)        /* �W�J�i�����j */
{
	int status;
	z_stream z;                     /* ���C�u�����Ƃ��Ƃ肷�邽�߂̍\���� */

	char *pI = pIn;
	char *pO = pOut;
	int nUsedSizeIn  = 0;
	int nUsedSizeOut = 0;

	if(nOutSize < 0)	return DZEXR_OUTBUFFERROR;

   /* ���ׂẴ������Ǘ������O�ōs�� */
    z.zalloc = Z_NULL;
    z.zfree  = Z_NULL;
    z.opaque = Z_NULL;

    /* ������ */
    z.next_in  = Z_NULL;
    z.avail_in = 0;
    if (inflateInit(&z) != Z_OK){return DZEXR_INITERROR;}

    z.next_out  = pO;           /* �o�̓|�C���^ */
    z.avail_out = (nOutSize - nUsedSizeOut >= OUTBUFSIZ) ? OUTBUFSIZ : nOutSize - nUsedSizeOut;    /* �o�̓o�b�t�@�c�� */
 	pO           += z.avail_out;
	nUsedSizeOut += z.avail_out;
    status = Z_OK;

    while (status != Z_STREAM_END)
	{
        if (z.avail_in == 0)
		{  /* ���͎c�ʂ��[���ɂȂ�� */
            z.next_in  = pI;  /* ���̓|�C���^�����ɖ߂� */
            z.avail_in = (nInSize - nUsedSizeIn >= INBUFSIZ) ? INBUFSIZ : nInSize - nUsedSizeIn;	/* �f�[�^��ǂݍ��� */
			pI          += z.avail_in;
			nUsedSizeIn += z.avail_in;
        }
        status = inflate(&z, Z_NO_FLUSH); /* �W�J */

        /* ���s�󋵕\�� */
        psp_progress(nUsedSizeIn,nInSize,0x1f<<8);
        
        if (status == Z_STREAM_END) break; /* ���� */
        if (status != Z_OK)
		{   /* �G���[ */
            return DZEXR_INFLATEERROR;
        }
        if (z.avail_out == 0)
		{ /* �o�̓o�b�t�@���s����� */
            z.next_out    = pO;        /* �o�̓|�C���^�����ɖ߂� */
            z.avail_out   = (nOutSize - nUsedSizeOut >= OUTBUFSIZ) ? OUTBUFSIZ : nOutSize - nUsedSizeOut; /* �o�̓o�b�t�@�c�ʂ����ɖ߂� */
			pO           += z.avail_out;
			nUsedSizeOut += z.avail_out;

			if(z.avail_out == 0)	return DZEXR_OUTBUFFERROR;
        }
    }

    /* �c���f���o�� */
    nUsedSizeOut -= z.avail_out;

    /* ��n�� */
    if (inflateEnd(&z) != Z_OK)
	{
        return DZEXR_INFLATEERROR;
    }

	return nUsedSizeOut;
}


///////////////////////////////////////////////////////////////////////////////
// 
// ZIP���k���o�b�N�O���E���h�Ŏ��s����
// 
///////////////////////////////////////////////////////////////////////////////
#include "syscall.h"
#include "zlibpsp.h"

// Compress with Callback
int do_compress_cb(int flag,INPUT_BUFFER* pIn)
{
    z_stream z;
	int flush, status;
    int level;
	char *pI;
    char *pO;
    int nInSize;
    int nOutSize;
	int nUsedSizeIn  = 0;
	int nUsedSizeOut = 0;
    PZLIB_Q pFunc;

    mh_print_hex8(0, 0,pIn->pfQuery,-1);
    mh_print_hex8(0,10,pIn, -1);
    
    sceKernelDcacheWritebackAll();
    pgScreenFlipV();

    
    if(pIn==0) {
        if(pFunc)
          pFunc(ZIPTHD_ERROR,DZEXR_OUTBUFFERROR);
        return DZEXR_INITERROR;
    }
    
    level = pIn->level;
	pI = pIn->i_ptr;
    pO = pIn->o_ptr;
    nInSize  = pIn->i_size;
    nOutSize = pIn->o_size;
	nUsedSizeIn  = 0;
	nUsedSizeOut = 0;
    pFunc = pIn->pfQuery;

    if((level < 1) || (9 < level))	level = 6;  /* �����k�͐ݒ�s�� */


    if(nOutSize < 0) {
        pFunc(ZIPTHD_ERROR,DZEXR_OUTBUFFERROR);
        return DZEXR_OUTBUFFERROR;
    }

    /* ���ׂẴ������Ǘ������O�ōs�� */
    z.zalloc = Z_NULL;
    z.zfree  = Z_NULL;
    z.opaque = Z_NULL;

    /* ������ */
    /* ��2�����͈��k�̓x���B0�`9 �͈̔͂̐����ŁC0 �͖����k */
    if (deflateInit(&z, level) != Z_OK){
        pFunc(ZIPTHD_ERROR,DZEXR_INITERROR);
        return DZEXR_INITERROR;
    }

    z.avail_in  = 0;            /* ���̓o�b�t�@���̃f�[�^�̃o�C�g�� */
    z.next_out  = pO;           /* �o�̓|�C���^ */
    z.avail_out = (nOutSize - nUsedSizeOut >= OUTBUFSIZ) ? OUTBUFSIZ : nOutSize - nUsedSizeOut;    /* �o�̓o�b�t�@�̃T�C�Y */
	pO           += z.avail_out;
	nUsedSizeOut += z.avail_out;

    flush = Z_NO_FLUSH;

    while( 1 )
	{
        /* �p�����邩�₢���킹�� */
        if( pFunc(ZIPTHD_QUERY,0)!=0 ) {
            pFunc(ZIPTHD_CANCELED,0);
            break;
        }
        
        pFunc(ZIPTHD_PROGRESS,nUsedSizeIn);
        pFunc(ZIPTHD_MAX     ,nUsedSizeOut);
        
        
        if(z.avail_in == 0) {  /* ���͂��s����� */
            z.next_in  = pI;  /* ���̓|�C���^����̓o�b�t�@�̐擪�� */
            z.avail_in = (nInSize - nUsedSizeIn >= INBUFSIZ) ? INBUFSIZ : nInSize - nUsedSizeIn;	/* �f�[�^��ǂݍ��� */
            pI          += z.avail_in;
            nUsedSizeIn += z.avail_in;

            /* ���͂��Ō�ɂȂ����� deflate() �̑�2������ Z_FINISH �ɂ��� */
            if (z.avail_in < INBUFSIZ) flush = Z_FINISH;
        }
        
        status = deflate(&z, flush); /* ���k���� */
        
        if(status == Z_STREAM_END) break; /* ���� */
        if(status != Z_OK) {   /* �G���[ */
            pFunc(ZIPTHD_ERROR,DZEXR_DEFLATEERROR);
            return DZEXR_DEFLATEERROR;
        }
        
        if(z.avail_out == 0) { /* �o�̓o�b�t�@���s����� */
            z.next_out  = pO;        /* �o�̓|�C���^�����ɖ߂� */
            z.avail_out = (nOutSize - nUsedSizeOut >= OUTBUFSIZ) ? OUTBUFSIZ : nOutSize - nUsedSizeOut; /* �o�̓o�b�t�@�c�ʂ����ɖ߂� */
            pO           += z.avail_out;
            nUsedSizeOut += z.avail_out;

            if(z.avail_out == 0) {
                pFunc(ZIPTHD_ERROR,DZEXR_OUTBUFFERROR);
                return DZEXR_OUTBUFFERROR;
            }
        }
    }
    
    /* �c���f���o�� */
    nUsedSizeOut -= z.avail_out;
    
    /* ��n�� */
    if(deflateEnd(&z) != Z_OK) {
        pFunc(ZIPTHD_ERROR,DZEXR_DEFLATEERROR);
        return DZEXR_DEFLATEERROR;
    }
    
    pFunc(ZIPTHD_COMPLETE,nUsedSizeOut);
    pFunc(ZIPTHD_PROGRESS,nUsedSizeIn);
    pFunc(ZIPTHD_MAX     ,nUsedSizeOut);
    
	return nUsedSizeOut;
}


int compress_thread(int flag,INPUT_BUFFER *pInp)
{
    do_compress_cb(flag,pInp);
    /* ���k�X���b�h���I��点�� */
    sceKernelExitThread(0);
    return 0;
}


// 
int ZipThread(INPUT_BUFFER* pInp)
{
    int zip_thd = sceKernelCreateThread("zip",compress_thread,0x21,0x10000,0,0);

    if(zip_thd<0) {
        return -1;
    }

    sceKernelStartThread(zip_thd,sizeof(INPUT_BUFFER),pInp);
    return 0;
}

