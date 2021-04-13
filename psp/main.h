//--------------------------------------
// configuration
typedef struct {
  unsigned int clock;           // cpu clock
  unsigned int vsync;           // vsync wait
  unsigned int sound;           // sound [stereo/mono/disable]
  unsigned int skip;            // frame skip timing
  unsigned int debug;           // deug
  unsigned int padno;           // pad number
  char cdrom[512];     // CDROM image name
  unsigned int video;           // video mode

  unsigned int button6;         // 6button enable
  unsigned int key[2][16];      // key config
  unsigned int rap[2][16];      // rapid pattern
  unsigned int rapm[2][16];     // rapit pattern mode
  unsigned int autoflag;        // autofire config
  unsigned int autofire;        // autofire flag
} EmuConfig;

extern EmuConfig eConf,pConf;

//--------------------------------------
// runtime paramemeter
typedef struct {
  int frame;
  int rapid[6];
  char hue_path[512];  // hue boot path
  char cart_name[512]; // rom name(full path)
  int state;           // state file
} EmuRuntime;

extern EmuRuntime eRun;


// èÛë‘ëJà⁄óp
enum {
  STATE_MAIN = 0,
  STATE_PLAY = 1,
  STATE_CONT = 2,
  STATE_QUIT = 3,
  STATE_ROM  = 4,
  STATE_RESET= 5
};

int menu_Main(void);
void changeClock(void);
void clockDown(void);
int psp_ExitCheck(void);

