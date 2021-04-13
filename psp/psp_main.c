//*****************************************************************************
// 
// PSP���g����ł̂����܂莖�����R�R�ɏ������Ƃɂ���
// 
// 
//*****************************************************************************
#include "psp_main.h"

typedef int (*pg_threadfunc_t)(int args, void *argp);

void sceDisplayWaitVblankStart(void);
void scePowerRegisterCallback(int zero, int cbid);
long scePowerSetClockFrequency(long,long,long);
void sceKernelExitGame();
int  sceKernelCreateThread(const char *name, pg_threadfunc_t func, unsigned long priority, unsigned long stacksize, unsigned long attrib, void *unk);
int  sceKernelStartThread(int hthread, int arg0, void *arg1);
void sceKernelExitThread(int ret);
int  sceKernelWaitThreadEnd(int hthread, void *unk);
int  sceKernelDeleteThread(int hthread);
int  sceKernelCreateCallback(const char *name, void* func, void *arg);
void sceKernelSetExitCallback(int cbid);
int sceExitSetCallback(int);
int KernelPollCallbacks(void);


//------------------------------------------------------------------------
// malloc���C�u�����̓R�m��`���g���ă������Ǘ�����B
// �������Ǘ���z��ł�肽���ꍇ�̓R�R�̒�`�����������Ă�
// PSP�̃������Ǘ����g���ꍇ�̓|�C���^�ƃT�C�Y��ݒ肵�Ă�
//------------------------------------------------------------------------
char malloc_mem[ MALLOC_MEMSIZE ];
int  malloc_size = sizeof(malloc_mem);

//------------------------------------------------------------------------
// PSP MAIN ROUTINE
//------------------------------------------------------------------------
extern int pspMain(int argc,char **argv);

//------------------------------------------------------------------------
// 
//------------------------------------------------------------------------
static int flag_psp = 1;     // PSPE�Ŏ��s����Ă��邩�������t���O
static int flag_end = 0;     // HOME KEY�ȂǂŒ�~��ԂɂȂ����ꍇ1�ɂȂ�
static pPowerCB pPSP_PowerCB = 0;


//==========================================================================
// Run On PSP 
//==========================================================================
int PSP_Is(void)
{
    return flag_psp;
}

//==========================================================================
// RUN
//==========================================================================
int PSP_IsEsc(void)
{
    return flag_end;
}

//==========================================================================
// RUN
//==========================================================================
void PSP_GoHome(void)
{
    flag_end = 1;
}

//==========================================================================
// Register PowerCallback
//==========================================================================
void PSP_SetCallback(pPowerCB pFunc)
{
    if(pFunc) {
        pPSP_PowerCB = pFunc;
    }
}


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
static int exit_callback(int arg1, int arg2)
{
    flag_end = 1;
    return 0;
}

//--------------------------------------------------------------------------
//
// 
//--------------------------------------------------------------------------
static int power_callback(int unknown, int pwrflags)
{
    if(pPSP_PowerCB) {
        pPSP_PowerCB(pwrflags);
    }
    
    sceDisplayWaitVblankStart();
    return 0;
}

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
static int callbackthread(void *arg) 
{
	int cbid = sceKernelCreateCallback("Exit Callback", exit_callback,0);
    sceKernelRegisterExitCallback(cbid);

    cbid = sceKernelCreateCallback("Power Callback", power_callback,0);
    scePowerRegisterCallback(0, cbid);
    
    KernelPollCallbacks();
	return 0;
}


//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
static int SetupCallback(void)
{
	int thid = 0;
    if ( ( thid = sceKernelCreateThread("Update Thread", (void*)callbackthread, 0x11, 0xFA0, 0, 0) ) < 0 ) {
        return thid;
    }
    sceKernelStartThread(thid, 0, 0);
	return thid;
}


//--------------------------------------------------------------------------
// setup
//--------------------------------------------------------------------------
static void setup(void)
{
    SetupCallback();
    
}

//--------------------------------------------------------------------------
// cleanup
//--------------------------------------------------------------------------
static void cleanup(void)
{
    if(PSP_Is()) {
        scePowerSetClockFrequency(222,222,111);
    }
    
    sceKernelExitGame();
}



//==========================================================================
// PSP�ŕK�{�ƂȂ鏈�����R�R�Ń���
// �A�v���P�[�V�������ɖʓ|�ȏ����������t���Ȃ�
//==========================================================================
int main(int argc,char ** argv)
{
    flag_psp = argc;  // PSPE(argc=0)

    setup();

    pspMain(argc,argv);

    cleanup();
    
    return 0;
}

