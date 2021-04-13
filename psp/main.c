#include "stdinc.h"
#include "pg.h"
#include "pce.h"
#include "string.h"
#include "main.h"
#include "sound.h"
#include "filer.h"
#include "hue_joy.h"
#include "hue_image.h"

#include "pspstd.h"
#include "psp_main.h"

int mp3_init(void);
int mp3_play_stop(void);
int mp3_play_track(int track,int bLoop);

int qclear(void);


EmuRuntime eRun = {
    0,
    {0,0,0,0,0,0}
};

int menu_flag = 0;

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
static int setup(void)
{
    pJoyStick = JoyStick;
    
#ifdef SOUND
    wavoutInit();
    pSoundFunc = SoundProcessing;
#endif

#ifdef ADPCM
    adpcmInit();
#endif//ADPCM
    
#ifdef MP3
    if(PSP_Is()) {
        pCddaPlayFunc = mp3_play_track;
        pCddaStopFunc = mp3_play_stop;
        mp3_init();
    }
#endif//ADPCM

    /* とりあえずデフォルト設定にしておくく */
    memcpy(&pConf,&eConf,sizeof(EmuConfig));

    return 1;
}

//--------------------------------------------------------------------------
//
//--------------------------------------------------------------------------
static int clean(void)
{
#ifdef SOUND
    wavoutClose();
#endif//SOUND
    return 1;
}

//--------------------------------------------------------------------------
// main routine
//--------------------------------------------------------------------------
int pspMain(int argc,char **argv)
{
    char path[MAX_PATH];
    int ext[3]={EXT_PCE,EXT_ZIP,EXT_NULL};

    memset(path,0,sizeof(path));

    pgMain(argc,argv[0]);
    setup();
    qinit();
    
    menu_flag = STATE_ROM;
   
    /* config file name */
    strcpy(eRun.hue_path,pguGetWorkdir());

    LoadConfig();

    loadWRAM();
    
    while(!PSP_IsEsc()){

        // RESETならROMを探さない
        if(menu_flag!=STATE_RESET) {
            if(getFilePath(path,eRun.hue_path,ext)==1) {
                
            }
        }

        if(LoadROM(path)) {
            qclear();
            changeClock();
            image_config_update();
#ifdef SOUND
            initSound();
            enable_sound(eConf.sound);
#endif
            RunPCE();
#ifdef SOUND
            enable_sound(0);
            closeSound();
#endif
            clockDown();
        } else {
            menu_flag = STATE_ROM;
            memset(path,0,sizeof(path));
        }
    }

    saveWRAM();

    
    // 終了するときにデータを保存する
    SaveConfig();
    
    clean();
    
    return 0;
}

