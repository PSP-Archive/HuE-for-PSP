//-----------------------------------------------------------------------------
//
// mp3 player function
//
//-----------------------------------------------------------------------------
// maman氏に提供して頂いたファイル群を、ほとんどそのまま利用しています。
// maman氏に感謝。
//-----------------------------------------------------------------------------
// MP3再生補助関数群 maman

#include "syscall.h"
#include "pg.h"
//#include "string.h"
#include "mad.h"
#include "main.h"

#include "pspstd.h"

#include <stdio.h>
#include <string.h>

static int mp3_loop = 1;
unsigned char * MP3BUFFER = 0;
static int decode(unsigned char const *, unsigned long);

// for repeat play
//#define LOOP

// sampling buffer size
// 1152の倍数なのはMADのデコードサイズと揃えるため
#define SAMPLE_SIZE (1152*3)

//-----------------------------------------------------------------------------

// mp3 file infomation
typedef struct MP3_INFO_ {
    int handle;
    FILE* fp; //int fd;
    int fsize;
    signed short buf_wav[2][SAMPLE_SIZE*4];
    void *buf_mp3;
} MP3_INFO;

MP3_INFO minf;

#define MP3PLAY_UNDEF     0
#define MP3PLAY_STOP     -1
#define MP3PLAY_RESUME   -2
#define MP3PLAY_PLAY      1

static int mp3_volume = 128;
static int mp3_enable = 1;
static int mp3_install=0;
int mp3_play = MP3PLAY_UNDEF;
int mp3_track= -1;
int mp3_tid  = -1;
int mp3_check=0;
int mp3_running=0;

char mp3name[512];

char *Mp3Name(int track);
int mp3play(char *fname);

void mp3_thread(void);


//-----------------------------------------------------------------------------
//
// MP3再生に必要な初期化を行う
//
//-----------------------------------------------------------------------------
int mp3_init(void)
{
    if(!mp3_install) {

        if((minf.handle = sceAudioChReserve(-1,SAMPLE_SIZE,0))<0){
            return -1;
        }
        
        mp3_tid = sceKernelCreateThread("mp3play",mp3_thread,0x21,0x10000,0,NULL);
        
        if(mp3_tid>=0) {
            sceKernelStartThread(mp3_tid,0,0);
        }
        
        mp3_install = 1;
        return 1;
    }
    return -1;
}

//-----------------------------------------------------------------------------
//
// mp3_initで取得したリソースを開放する
//
//-----------------------------------------------------------------------------
int mp3_close(void)
{
    if(minf.handle<0) {
        return -1;
    }

    sceAudioChRelease(minf.handle);
//    sceKernelTerminateThread(mp3_tid);
    
    minf.handle = -1;
    mp3_install = 0;
    return 1;
}

//-----------------------------------------------------------------------------
//
// mp3の再生を支援するスレッド
//
//-----------------------------------------------------------------------------
void mp3_thread(void)
{
    int play_track=-1;
    char *name=0;

    // Init mp3inf
    minf.fp=0; // minf.fd = 0;
    minf.fsize = 0;
    
    while(!PSP_IsEsc()) {
        // 基本的にSleep状態
        sceKernelSleepThread();
        
        if(mp3_track!=-1) {
            name = Mp3Name(mp3_track);
            if(name) {
                mp3play(name);
            }
            play_track = mp3_track;
        }
    }
}

int mp3play(char *fname)
{
    int size,ret=-1;
    
    strcpy(mp3name,fname);    // copy mp3name (for debug)

    minf.fp = fopen(fname,"r");

    if(!minf.fp){ //if(minf.fd<0) {
        core_error("file can not open");
        return -1;
    }

    mh_print(0,0,fname,-1);
    pgScreenFlip();
    while(1);

    fseek(minf.fp,0,SEEK_END);
    size = ftell(minf.fp);
    fseek(minf.fp,0,SEEK_SET);
    
    if(MP3BUFFER) { free(MP3BUFFER); }
    MP3BUFFER = malloc(size);

    if(MP3BUFFER) {
        minf.buf_mp3 = MP3BUFFER;
        minf.fsize = fread(minf.buf_mp3,1,size,minf.fp);
        fclose(minf.fp);
        minf.fp = 0;

        if(minf.fsize>0) {
            mp3_running = mp3_check;
            ret = decode(minf.buf_mp3, minf.fsize);
        }
    }

    if(MP3BUFFER) {
        free(MP3BUFFER);
        MP3BUFFER=0;
    }
    
    return ret;
}

struct madbuffer {
    unsigned char const *start;
    unsigned long length;
};

// デコードバッファが無い時に呼ばれるCallback関数
static enum mad_flow input(void *data, struct mad_stream *stream)
{
    struct madbuffer *buffer = data;
    
    // 再生終了フラグ
    if(!buffer->length) return MAD_FLOW_STOP;
    
    mad_stream_buffer(stream, minf.buf_mp3, buffer->length);
    
    // 読み込み済みサイズを更新
    // ループ再生の場合はlengthの値は変更しない

    if( !mp3_loop ) {
        buffer->length=0;
    }
    
    return MAD_FLOW_CONTINUE;
}

// convert MAD format to signed short WAVE format
static inline signed int scale(mad_fixed_t sample)
{
    /* round */
    sample += (1L << (MAD_F_FRACBITS - 16));
    
    /* clip */
    if (sample >= MAD_F_ONE) sample =  MAD_F_ONE-1;  else
    if (sample < -MAD_F_ONE) sample = -MAD_F_ONE;
    
    /* quantize */
    return sample >> (MAD_F_FRACBITS + 1 - 16);
}

// MP3デコードが完了した時に呼ばれる関数
static enum mad_flow output(void *data,
                            struct mad_header const *header,
                            struct mad_pcm *pcm)
{
    unsigned int nchannels, nsamples;
    mad_fixed_t const *left_ch, *right_ch;
    static int cnt = 0;
    static int idx = 0;
    int vol = mp3_volume * 0x100;
    
    nchannels = pcm->channels;
    nsamples  = pcm->length;
    left_ch   = pcm->samples[0];
    right_ch  = pcm->samples[1];

    // 終了フラグがONなら中断する
    if(PSP_IsEsc() || mp3_check!=mp3_running) {
        return MAD_FLOW_STOP;
    }
    
    while (nsamples--) {
        signed short left,right;
        left = scale(*left_ch++);
        right = scale(*right_ch++);
        
        // output sample(s) in 16-bit signed little-endian PCM
        minf.buf_wav[idx][cnt++] = (signed short)left;
        if (nchannels == 2) {
            minf.buf_wav[idx][cnt++] = (signed short)right;
        } else {
            // for mono
            minf.buf_wav[idx][cnt++] = (signed short)left;
        }

        
        // 再生バッファが一杯になったら再生
        if(cnt == SAMPLE_SIZE*2) {
            sceAudioOutputPannedBlocking(minf.handle,vol,vol,minf.buf_wav[idx]);
            // swap buffer
            idx = idx?0:1;
            cnt = 0;
        }
    }

    return MAD_FLOW_CONTINUE;
}

static enum mad_flow error(void *data,
                           struct mad_stream *stream,
                           struct mad_frame *frame)
{
//    struct buffer *buffer = data;
    
    return MAD_FLOW_CONTINUE;
}

static int decode(unsigned char const *start, unsigned long length)
{
    struct madbuffer buffer;
    struct mad_decoder decoder;
    int result;
    
    buffer.start  = start;
    buffer.length = length;
    
    mad_decoder_init(&decoder, &buffer,
                     input, 0 /* header */, 0 /* filter */, output,
                     error, 0 /* message */);
    
    result = mad_decoder_run(&decoder, MAD_DECODER_MODE_SYNC);
    
    mad_decoder_finish(&decoder);
    
    return result;
}

//---------------------------------------------------------
// Emulator関連コード
//---------------------------------------------------------

// 指定トラックを再生する
int mp3_play_track(int track,int bLoop)
{
    mp3_play = MP3PLAY_PLAY;
    mp3_track= track;
    mp3_loop = bLoop;
    mp3_check++;
    sceKernelWakeupThread(mp3_tid);
    return 0;
}

// 再生を停止する
int mp3_play_stop(void)
{
    mp3_play = MP3PLAY_STOP;
    mp3_track=-1;
    mp3_check++;
    sceKernelWakeupThread(mp3_tid);
    return 0;
}

// 再生を再開する
void mp3_play_resume(void)
{
    mp3_play = MP3PLAY_RESUME;
    mp3_check++;
    sceKernelWakeupThread(mp3_tid);
}

// ボリュームを下げる
void mp3_play_volume(int vol)
{
    if(vol<=0) {
        mp3_volume = 0;
        mp3_enable = 0;
    } else {
        if(vol>=100) {
            vol = 100;
            mp3_volume=vol;
            mp3_enable=1;
        }
    }

    sceKernelWakeupThread(mp3_tid);
}


void mp3_play_pause(void)
{
    sceKernelSuspendThread(mp3_tid);
}

void mp3_play_pause_cont(void)
{
    sceKernelResumeThread(mp3_tid);
}

//-----------------------------------------------------------------------------
// debug
//-----------------------------------------------------------------------------
void mp3_debug(void)
{
    int y=100;
    mh_print_hex8(0,y,mp3_install,-1); y+=10;
    mh_print_hex8(0,y,mp3_play,-1);y+=10;
    mh_print_hex8(0,y,mp3_track,-1);y+=10;
}
