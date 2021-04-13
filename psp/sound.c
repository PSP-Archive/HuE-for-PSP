//---------------------------------------------------------------------------------------
// make sound buffer
//---------------------------------------------------------------------------------------

#if defined(SOUND)

#include "stdinc.h"
#include "string.h"
#include "syscall.h"
#include "pce.h"
#include "sound.h"
#include "pg.h"
#include "main.h"

void   pgaSetChannelCallback(int channel, void *callback);

static int mseq(DWORD *rand_val);
int    WriteBuffer2(int ch, DWORD dwSize);
void   WriteSoundData(int ch, DWORD dwSize);

// internal 
static int CycleLimit;            // 音(1byte)合成に必要な最少Clock値 (7160000/44100=162.3582...)
static int sound_enable=1;        // 0:Sound Off / 1:Sound On

#define    SAMPLE_RATE    ((DWORD)44100)// sampling rate
#define    SAMPLE10       (SAMPLE_RATE*10)

#define    SND_L            1
#define    SND_R            0

//------------------------------------------------------------------------------
// Buffer Size
//------------------------------------------------------------------------------
#define    SND_FRMSIZE    736     // 1 frame sound data size (=44100/59.94)
#define    SND_BNKSIZE    512     // buffer read size (see pg.c)

#define    SND_UNDERLIMIT   (2*SND_FRMSIZE)          // underflow limit
#define    SND_OVERLIMIT    (10*SND_FRMSIZE)         // overflow  limit
#define    SND_RNGSIZE      (40*SND_BNKSIZE)         // sound ring buffer 

//------------------------------------------------------------------------------
// last cycle timing
//------------------------------------------------------------------------------
static DWORD dwOldPos[6]={0,0,0,0,0,0};
static DWORD CycleLocal[6]={0,0,0,0,0,0};

//------------------------------------------------------------------------------
// 16bit Sound Ring Buffer
//------------------------------------------------------------------------------
static int   snd_wr=0;                  // Sound Write Pointer
static int   snd_rd=0;                  // Sound Read  Pointer
static short sndbuffer[SND_RNGSIZE][2]; // Sound Ring Buffer
static int   over_wr=0,over_rd=0;       // (Debug) overrun counter
static int   unlk_wr=0,unlk_rd=0;       // (Debug) unlock error

//------------------------------------------------------------------------------
// debug 
//------------------------------------------------------------------------------
static int   snd_sound_skip=0;          // sound write skip (overflow limit check)
static int   snd_vsync_skip=0;          // underflow vsync skip counter
static int   snd_frame_skip=0;          // underflow frame skip counter

//------------------------------------------------------------------------------
// Sound Mix Buffer
// 1/60秒毎に合成してRing Bufferへ移動する
//------------------------------------------------------------------------------
static int  mix_wave[SND_FRMSIZE*4][2]; // 1/60に必要なbuffer x 4 
static int  mix_ch[6]={0,0,0,0,0,0};    // 処理済みindex値(ch別)

// internal function
static int    bufLen(void);
static short *bufSetLock(int size);
static void   bufSetUnlock(void* p,int size);
static short *bufGetLock(int size);
static void   bufGetUnlock(void* p,int size);

//------------------------------------------------------------------------------
// rest table
//------------------------------------------------------------------------------
void reset_sound(void)
{
    memset(dwOldPos,0,sizeof(dwOldPos));
    memset(CycleLocal,0,sizeof(CycleLocal));
    memset(mix_wave,0,sizeof(mix_wave));
    memset(mix_ch  ,0,sizeof(mix_ch)  );
}

//------------------------------------------------------------------------------
// enable or disable
//------------------------------------------------------------------------------
void enable_sound(int mode)
{
    if(mode) {
        if(sound_enable==0) {
            reset_sound();
        }
        sound_enable=1;
    }
    else {
        sound_enable=0;
    }
}


//------------------------------------------------------------------------------
// Init Parameter
//------------------------------------------------------------------------------
void initSound(void)
{
    snd_wr=snd_rd=0;
    memset(sndbuffer,0,sizeof(sndbuffer));
    memset(CycleLocal ,0,sizeof(CycleLocal) );
    memset(dwOldPos ,0,sizeof(dwOldPos) );
    memset(mix_wave ,0,sizeof(mix_wave) );
    memset(mix_ch   ,0,sizeof(mix_ch)   );
}

//------------------------------------------------------------------------------
// Cleanup Parameter
//------------------------------------------------------------------------------
void closeSound(void)
{
    
}

//------------------------------------------------------------------------------
// bufferの長さをゲッツ
//------------------------------------------------------------------------------
static int bufLen(void)
{
    register int rd=snd_rd;
    register int wr=snd_wr;
    if(wr==rd) return 0;
    if(wr >rd) return wr-rd;;
    return SND_RNGSIZE - rd + wr;
}

//------------------------------------------------------------------------------
// 次に書き込む音声データ領域をゲッツする関数
// 実際にゲッツしたアドレスは戻値を使ってほしい
//------------------------------------------------------------------------------
static short *bufSetLock(int size)
{
    int p = (snd_wr+size)%SND_RNGSIZE;

    if(p!=snd_rd) {
        return sndbuffer[snd_wr];
    }
    over_wr++;
    
    return 0;
}

//------------------------------------------------------------------------------
// [Write] ロックを解除
//------------------------------------------------------------------------------
static void bufSetUnlock(void*ptr,int size)
{
    if( ptr==(void*)&sndbuffer[snd_wr] ) {
        snd_wr=(snd_wr+size)%SND_RNGSIZE;
    } else {
        unlk_wr++;
    }
}

//------------------------------------------------------------------------------
// Sound Ring Bufferから音データを拾う処理
//------------------------------------------------------------------------------
static short *bufGetLock(int size)
{
    if(bufLen()>=size) { // 必要以上のデータがあるか？
        return sndbuffer[snd_rd];
    }
    over_rd++;
    return 0;
}

//------------------------------------------------------------------------------
// [Read] 
//------------------------------------------------------------------------------
static void bufGetUnlock(void* ptr,int size)
{
    if(ptr==(void*)&sndbuffer[snd_rd]) {
        snd_rd=(snd_rd+size)%SND_RNGSIZE;
    } else {
        unlk_rd++;
    }
}


int prev_skip_frame=0;

//------------------------------------------------------------------------------
//【音の安定化処理】
// サウンド作成が間に合わない場合...
// (1)VSYNCをOFFにしてみる。
// (2)FRAMESKIPをONにしてみる。
// 以上の２点で頑張ってみる。
//------------------------------------------------------------------------------
int SoundStabilizer(void)
{
    if(bufLen()<=SND_UNDERLIMIT) {
        if(g_skip_next_vsync==0) { //if(io.g_skip_next_vsync==0) {
            g_skip_next_vsync++; //io.g_skip_next_vsync++;
            snd_vsync_skip++;
        } else {
            g_skip_next_frame++; //io.g_skip_next_frame++;
            snd_frame_skip++;
        }
    } else {
        //io.g_skip_next_vsync=0;
        //io.g_skip_next_frame=0;
        g_skip_next_vsync=0;
        g_skip_next_frame=0;
    }
    
    // 直前がframe skipならframe skipしない。
    // skipを連続させると画面表示されずゲームにならない
    if(prev_skip_frame) {
        //io.g_skip_next_frame=0;
        g_skip_next_frame=0;
    }
    
    //prev_skip_frame = io.g_skip_next_frame;
    prev_skip_frame = g_skip_next_frame;
    
    //return ((io.g_skip_next_frame+io.g_skip_next_vsync)>0);
    return ((g_skip_next_frame+g_skip_next_vsync)>0);
}


//------------------------------------------------------------------------------
// 1/60秒間隔で実行して欲しい関数
// 6ch分の音声を合成する
//------------------------------------------------------------------------------
int SoundProcessing(int control)
{
    if(control==SOUND_PLAY) {
        if(sound_enable) {
            // calc sound data
            WriteSoundData(0,SND_FRMSIZE); 
            WriteSoundData(1,SND_FRMSIZE);
            WriteSoundData(2,SND_FRMSIZE);
            WriteSoundData(3,SND_FRMSIZE);
            WriteSoundData(4,SND_FRMSIZE);
            WriteSoundData(5,SND_FRMSIZE);
            
            // convert int to short
            {
                register int i;
                register int wave;
                register int swr = snd_wr;
                
                for(i=0;i<SND_FRMSIZE;i++) {
                    wave = mix_wave[i][SND_R];
                    
                    if(wave > 0x7FFF)      wave =  0x07FFF;
                    else if(wave< -0x8000) wave = -0x08000;
                    
                    sndbuffer[swr][SND_R] = (short)wave;
                    mix_wave[i][SND_R]=0;
                    
                    wave = mix_wave[i][SND_L];
                    
                    if(wave > 0x7FFF)      wave =  0x07FFF;
                    else if(wave< -0x8000) wave = -0x08000;
                    
                    sndbuffer[swr][SND_L] = (short)wave;
                    mix_wave[i][SND_L]=0;
                    swr++;
                    
                    if(swr>=SND_RNGSIZE){
                        swr=0;
                    }
                }
                
                // buffer overflow limit 未満なら追加
                if(bufLen()<SND_OVERLIMIT) {
                    snd_wr = swr;
                }else {
                    snd_sound_skip++;
                }
                
                mix_ch[0]=mix_ch[1]=mix_ch[2]=0;
                mix_ch[3]=mix_ch[4]=mix_ch[5]=0;
                
                if(PSP_Is()){
                    sceKernelWakeupThread(pga_threadhandle[0]);
                }
            }
        }
    }
    else {
        if(control==SOUND_RESET) {
            reset_sound();
        }
    }
}
    
//------------------------------------------------------------------------------
// Emulation coreでpsgに変化があるとき音データを作る
// そんとき呼ばれる関数
//------------------------------------------------------------------------------
void write_psg(int ch)
{
    if(sound_enable) {
        register DWORD dwNewPos;
        register int Cycle = (DWORD)io.m6502.User - CycleLocal[ch];
        
    	// オーバーフローしたタイミングで変になるかなぁ
        if(Cycle<0) {
            CycleLocal[ch] = (DWORD)io.m6502.User;
        } else {
            // サウンドデータWrite間隔の閾値を超えているか？
            if(Cycle>=CycleLimit) {
                dwNewPos = Cycle/CycleLimit;
                WriteBuffer2(ch, (dwNewPos-dwOldPos[ch]));
                dwOldPos[ch] = dwNewPos;
            }
        }
    }
}

//------------------------------------------------------------------------------
//
//
//------------------------------------------------------------------------------
static int mseq(DWORD *rand_val)
{
	if (*rand_val & 0x00080000)	{
		*rand_val = ((*rand_val ^ 0x0004) << 1) + 1;
		return 1;
	} else {
		*rand_val <<= 1;
		return 0;
	}
}

//------------------------------------------------------------------------------
// dwSizeは1chあたりのサイズを指定すること
// CHの増減管理はWriteBuffer内で収めたい
//------------------------------------------------------------------------------
void WriteSoundData(int ch, DWORD dwNewPos)
{
    if(dwOldPos[ch] < dwNewPos){
        WriteBuffer2(ch, (dwNewPos-dwOldPos[ch]));
    }
    
    CycleLocal[ch] = (DWORD)io.m6502.User;
    dwOldPos[ch] = 0;
}


//
// レジスタを使い過ぎると遅くなる場合もあるみたいなので注意
//
int WriteBuffer2(int ch, DWORD dwSize)
{
	static DWORD n[6] = {0,0,0,0,0,0};
	static DWORD k[6] = {0,0,0,0,0,0};
	static DWORD r[6];
	static DWORD rand_val[6] = {0,0,0,0,0x51F631E4,0x51F631E4};
	/* a*2^(b*x) */
	/* a = 400, b = log2(16384/a), x = 0.0 ~ 1.0 */
	static int	 vol_tbl[32] = {
		 100, 451, 508,  573,  646,  728,  821,  925,
		1043,1175,1325, 1493, 1683, 1898, 2139, 2411,
		2718,3064,3454, 3893, 4388, 4947, 5576, 6285,
		7085,7986,9002,10148,11439,12894,14535,16384,
	};
    register int lvol, rvol;
    
    // 書き込みサイズをオーバーする要求は切り詰める
    if((dwSize+mix_ch[ch])>(SND_FRMSIZE)) {
        dwSize = 0; // (SND_FRMSIZE)-mix_ch[ch];
    }

    if(dwSize==0) {
        return mix_ch[ch];
    }
    
	if (!(io.PSG[ch][4]&0x80)) {
		n[ch] = k[ch] = 0;
        mix_ch[ch]+=dwSize;
        return mix_ch[ch];
	}

    /* make volume data from table & register*/
    {
        int psgv = io.psg_volume;
        int psg4 = io.PSG[ch][4] & 0x1f;
        int psg5 = io.PSG[ch][5];
        
        lvol = ((psgv>>3)&0x1E) + psg4 + ((psg5>>3)&0x1E) - 60;
        if(lvol<0) lvol = vol_tbl[0];
        else       lvol = vol_tbl[lvol];
        
        rvol = ((psgv<<1)&0x1E) + psg4 + ((psg5<<1)&0x1E) - 60;
        if (rvol<0) rvol = vol_tbl[0];
        else        rvol = vol_tbl[rvol];
    }

    // Direct D/A output
    if (io.PSG[ch][4]&0x40) {
        register int wav0 = (((int)io.wave[ch][0])-16)*702;
        register int mixch=mix_ch[ch];
        
        lvol = (int)wav0*lvol>>14;
        rvol = (int)wav0*rvol>>14;
        
        for(;dwSize;dwSize--) {
            mix_wave[mixch][SND_L] += lvol;
            mix_wave[mixch][SND_R] += rvol;
            mixch++;
        }
        
        mix_ch[ch]=mixch;
        
	} else {
        if (ch >= 4 && (io.PSG[ch][7]&0x80)) {
            register DWORD ra = r[ch];
            register DWORD ka = k[ch];
            int ra7020l=(7020*lvol)>>14;
            int ra7020r=(7020*rvol)>>14;
            register int sndL=(ra?ra7020l:-ra7020l);
            register int sndR=(ra?ra7020r:-ra7020r);
            register int mixch=mix_ch[ch];
            register DWORD Np = (io.PSG[ch][7]&0x1F);
            register DWORD t;
            
            Np = 3000 + (Np<<9);
            
            for(;dwSize;dwSize--) {
                ka += Np;
                t = ka / 44100;
                if (t >= 1) {
                    ra = mseq(&rand_val[ch]);
                    ka -= 44100*t;
                    sndL=(ra?+ra7020l:-ra7020l);
                    sndR=(ra?+ra7020r:-ra7020r);
                }
                mix_wave[mixch][SND_L]+= sndL;
                mix_wave[mixch][SND_R]+= sndR;
                mixch++;
            }
            
            r[ch]=ra;
            k[ch]=ka;
            mix_ch[ch]=mixch;
            return mix_ch[ch];
        }
        else {
            DWORD Tp = io.PSG[ch][2]+((DWORD)io.PSG[ch][3]<<8);
            int i;
            DWORD t;

            if (Tp<2) {
                mix_ch[ch]+=dwSize;
                return mix_ch[ch];
            }

            Tp-=1;
            
            {
#if 1
                static short wave[32];
                for(i=0;i<32;i++) {
                    wave[i] = ((short)io.wave[ch][i]-16)*702;
                }
#else
                static WORD wave[32];
                for(i=0;i<32;i++) {
                    wave[i] = ((short)io.wave[ch][i]-16)*702;
                }
#endif
                
                {
                    register DWORD na = n[ch];
                    register DWORD ka = k[ch];
                    register DWORD NTp = (32*1118608)/Tp;
                    register int wavena;
                    register int mixch = mix_ch[ch];
                    
                    for(;dwSize;dwSize--) {
                        wavena = wave[na];
                        mix_wave[mixch][SND_L] += ((int)(short)wavena*lvol>>14);
                        mix_wave[mixch][SND_R] += ((int)(short)wavena*rvol>>14);
                        mixch++;
                        ka += NTp;
                        t = ka/SAMPLE10;
                        na = (na+t)&31;
                        ka -= SAMPLE10*t;
                    }
                    
                    n[ch]=na;
                    k[ch]=ka;
                    mix_ch[ch]=mixch;
                }
            }
        }
    }
    return mix_ch[ch];
}

//------------------------------------------------------------------------------
// Sound Player Thread main function
//------------------------------------------------------------------------------
void* wavout_snd0_callback(short *buf, unsigned long reqn)
{
    static short *src=0;
    int buflen;

    if(sound_enable) {
        if(src) {
            bufGetUnlock(src,reqn);
            src=0;
        }
    
        for(buflen=bufLen();
            buflen<(reqn*2); // (2005.07.01) 音割の原因？
            buflen=bufLen() ) {
            sceKernelSleepThread(); // if no sound buffer, sleep...
        }
        src = bufGetLock(reqn);
        return src;
    }

    return 0;
}

//------------------------------------------------------------------------------
//
// Sound Initialize
// 
//------------------------------------------------------------------------------
int wavoutInit(void)
{
    // Sound data 1byteを作るのに必要なCycle間隔
    CycleLimit = BaseClock / SAMPLE_RATE;

    // bufSetLock(SND_FRMSIZE * SND_CHMAX);
    // setup thread function
    pgaSetChannelCallback(0,wavout_snd0_callback);
    return 0;
}

//------------------------------------------------------------------------------
//
// Sound Close
// 
//------------------------------------------------------------------------------
int wavoutClose(void)
{
    return 1;
}


//------------------------------------------------------------------------------
// デバッグ用の表示など
//------------------------------------------------------------------------------
void dbgSound(void)
{
}


#endif // defined(SOUND)

