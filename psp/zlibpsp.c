#include "deflateInterface.h"
#include "zlib.h"

//#define INBUFSIZ   1024         /* 入力バッファサイズ（任意） */
//#define OUTBUFSIZ  1024         /* 出力バッファサイズ（任意） */
#define INBUFSIZ   128         /* 入力バッファサイズ（任意） */
#define OUTBUFSIZ  1024         /* 出力バッファサイズ（任意） */

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

int do_compress_m2m(char *pIn,int nInSize,char *pOut,int nOutSize,int level)          /* 圧縮 */
{
	int flush, status;
	z_stream z;                     /* ライブラリとやりとりするための構造体 */

	char *pI = pIn;
	char *pO = pOut;
	int nUsedSizeIn  = 0;
	int nUsedSizeOut = 0;

	if((level < 0) || (9 < level))	level = 6;
	if(nOutSize < 0)	return DZEXR_OUTBUFFERROR;

    /* すべてのメモリ管理を自前で行う */
    z.zalloc = Z_NULL;
    z.zfree  = Z_NULL;
    z.opaque = Z_NULL;

    /* 初期化 */
    /* 第2引数は圧縮の度合。0～9 の範囲の整数で，0 は無圧縮 */
    if (deflateInit(&z, level) != Z_OK){return DZEXR_INITERROR;}

    z.avail_in  = 0;            /* 入力バッファ中のデータのバイト数 */
    z.next_out  = pO;           /* 出力ポインタ */
    z.avail_out = (nOutSize - nUsedSizeOut >= OUTBUFSIZ) ? OUTBUFSIZ : nOutSize - nUsedSizeOut;    /* 出力バッファのサイズ */
	pO           += z.avail_out;
	nUsedSizeOut += z.avail_out;

    /* 通常は deflate() の第2引数は Z_NO_FLUSH にして呼び出す */
    flush = Z_NO_FLUSH;

    while(1)
	{
        if (z.avail_in == 0)
		{  /* 入力が尽きれば */
            z.next_in  = pI;  /* 入力ポインタを入力バッファの先頭に */
            z.avail_in = (nInSize - nUsedSizeIn >= INBUFSIZ) ? INBUFSIZ : nInSize - nUsedSizeIn;	/* データを読み込む */
			pI          += z.avail_in;
			nUsedSizeIn += z.avail_in;

            /* 入力が最後になったら deflate() の第2引数は Z_FINISH にする */
            if (z.avail_in < INBUFSIZ) flush = Z_FINISH;
        }
        status = deflate(&z, flush); /* 圧縮する */

        /* 実行状況表示 */
        psp_progress(nUsedSizeIn,nInSize,0x001f<<10);
        
        if (status == Z_STREAM_END) break; /* 完了 */
        if (status != Z_OK) {   /* エラー */
            return DZEXR_DEFLATEERROR;
        }
        if (z.avail_out == 0)
		{ /* 出力バッファが尽きれば */
            z.next_out  = pO;        /* 出力ポインタを元に戻す */
            z.avail_out = (nOutSize - nUsedSizeOut >= OUTBUFSIZ) ? OUTBUFSIZ : nOutSize - nUsedSizeOut; /* 出力バッファ残量を元に戻す */
			pO           += z.avail_out;
			nUsedSizeOut += z.avail_out;

			if(z.avail_out == 0)	return DZEXR_OUTBUFFERROR;
        }
    }

    /* 残りを吐き出す */
    nUsedSizeOut -= z.avail_out;

    /* 後始末 */
    if (deflateEnd(&z) != Z_OK)
	{
        return DZEXR_DEFLATEERROR;
    }

	return nUsedSizeOut;
}

int do_decompress_m2m(char *pIn,int nInSize,char *pOut,int nOutSize)        /* 展開（復元） */
{
	int status;
	z_stream z;                     /* ライブラリとやりとりするための構造体 */

	char *pI = pIn;
	char *pO = pOut;
	int nUsedSizeIn  = 0;
	int nUsedSizeOut = 0;

	if(nOutSize < 0)	return DZEXR_OUTBUFFERROR;

   /* すべてのメモリ管理を自前で行う */
    z.zalloc = Z_NULL;
    z.zfree  = Z_NULL;
    z.opaque = Z_NULL;

    /* 初期化 */
    z.next_in  = Z_NULL;
    z.avail_in = 0;
    if (inflateInit(&z) != Z_OK){return DZEXR_INITERROR;}

    z.next_out  = pO;           /* 出力ポインタ */
    z.avail_out = (nOutSize - nUsedSizeOut >= OUTBUFSIZ) ? OUTBUFSIZ : nOutSize - nUsedSizeOut;    /* 出力バッファ残量 */
 	pO           += z.avail_out;
	nUsedSizeOut += z.avail_out;
    status = Z_OK;

    while (status != Z_STREAM_END)
	{
        if (z.avail_in == 0)
		{  /* 入力残量がゼロになれば */
            z.next_in  = pI;  /* 入力ポインタを元に戻す */
            z.avail_in = (nInSize - nUsedSizeIn >= INBUFSIZ) ? INBUFSIZ : nInSize - nUsedSizeIn;	/* データを読み込む */
			pI          += z.avail_in;
			nUsedSizeIn += z.avail_in;
        }
        status = inflate(&z, Z_NO_FLUSH); /* 展開 */

        /* 実行状況表示 */
        psp_progress(nUsedSizeIn,nInSize,0x1f<<8);
        
        if (status == Z_STREAM_END) break; /* 完了 */
        if (status != Z_OK)
		{   /* エラー */
            return DZEXR_INFLATEERROR;
        }
        if (z.avail_out == 0)
		{ /* 出力バッファが尽きれば */
            z.next_out    = pO;        /* 出力ポインタを元に戻す */
            z.avail_out   = (nOutSize - nUsedSizeOut >= OUTBUFSIZ) ? OUTBUFSIZ : nOutSize - nUsedSizeOut; /* 出力バッファ残量を元に戻す */
			pO           += z.avail_out;
			nUsedSizeOut += z.avail_out;

			if(z.avail_out == 0)	return DZEXR_OUTBUFFERROR;
        }
    }

    /* 残りを吐き出す */
    nUsedSizeOut -= z.avail_out;

    /* 後始末 */
    if (inflateEnd(&z) != Z_OK)
	{
        return DZEXR_INFLATEERROR;
    }

	return nUsedSizeOut;
}


///////////////////////////////////////////////////////////////////////////////
// 
// ZIP圧縮をバックグラウンドで実行する
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

    if((level < 1) || (9 < level))	level = 6;  /* 無圧縮は設定不可 */


    if(nOutSize < 0) {
        pFunc(ZIPTHD_ERROR,DZEXR_OUTBUFFERROR);
        return DZEXR_OUTBUFFERROR;
    }

    /* すべてのメモリ管理を自前で行う */
    z.zalloc = Z_NULL;
    z.zfree  = Z_NULL;
    z.opaque = Z_NULL;

    /* 初期化 */
    /* 第2引数は圧縮の度合。0～9 の範囲の整数で，0 は無圧縮 */
    if (deflateInit(&z, level) != Z_OK){
        pFunc(ZIPTHD_ERROR,DZEXR_INITERROR);
        return DZEXR_INITERROR;
    }

    z.avail_in  = 0;            /* 入力バッファ中のデータのバイト数 */
    z.next_out  = pO;           /* 出力ポインタ */
    z.avail_out = (nOutSize - nUsedSizeOut >= OUTBUFSIZ) ? OUTBUFSIZ : nOutSize - nUsedSizeOut;    /* 出力バッファのサイズ */
	pO           += z.avail_out;
	nUsedSizeOut += z.avail_out;

    flush = Z_NO_FLUSH;

    while( 1 )
	{
        /* 継続するか問い合わせる */
        if( pFunc(ZIPTHD_QUERY,0)!=0 ) {
            pFunc(ZIPTHD_CANCELED,0);
            break;
        }
        
        pFunc(ZIPTHD_PROGRESS,nUsedSizeIn);
        pFunc(ZIPTHD_MAX     ,nUsedSizeOut);
        
        
        if(z.avail_in == 0) {  /* 入力が尽きれば */
            z.next_in  = pI;  /* 入力ポインタを入力バッファの先頭に */
            z.avail_in = (nInSize - nUsedSizeIn >= INBUFSIZ) ? INBUFSIZ : nInSize - nUsedSizeIn;	/* データを読み込む */
            pI          += z.avail_in;
            nUsedSizeIn += z.avail_in;

            /* 入力が最後になったら deflate() の第2引数は Z_FINISH にする */
            if (z.avail_in < INBUFSIZ) flush = Z_FINISH;
        }
        
        status = deflate(&z, flush); /* 圧縮する */
        
        if(status == Z_STREAM_END) break; /* 完了 */
        if(status != Z_OK) {   /* エラー */
            pFunc(ZIPTHD_ERROR,DZEXR_DEFLATEERROR);
            return DZEXR_DEFLATEERROR;
        }
        
        if(z.avail_out == 0) { /* 出力バッファが尽きれば */
            z.next_out  = pO;        /* 出力ポインタを元に戻す */
            z.avail_out = (nOutSize - nUsedSizeOut >= OUTBUFSIZ) ? OUTBUFSIZ : nOutSize - nUsedSizeOut; /* 出力バッファ残量を元に戻す */
            pO           += z.avail_out;
            nUsedSizeOut += z.avail_out;

            if(z.avail_out == 0) {
                pFunc(ZIPTHD_ERROR,DZEXR_OUTBUFFERROR);
                return DZEXR_OUTBUFFERROR;
            }
        }
    }
    
    /* 残りを吐き出す */
    nUsedSizeOut -= z.avail_out;
    
    /* 後始末 */
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
    /* 圧縮スレッドを終わらせる */
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

