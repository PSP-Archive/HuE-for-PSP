#define MALLOC_MEMSIZE   (16*1024*1024)

typedef void (*pPowerCB)(int powerflag);

#define PSPCB_POWER              0x80000000
#define PSPCB_HOLD               0x40000000
#define PSPCB_STANDBY            0x00080000
#define PSPCB_RESUME_COMPLETE    0x00040000
#define PSPCB_RESUMING           0x00020000
#define PSPCB_SUSPENDING         0x00010000
#define PSPCB_AC_POWER           0x00001000
#define PSPCB_BATTERY_LOW        0x00000100
#define PSPCB_BATTERY_EXIST      0x00000080
#define PSPCB_BATTPOWER          0x0000007F


int  PSP_Is(void);
int  PSP_IsEsc(void);
void PSP_GoHome(void);
void PSP_SetCallback(pPowerCB);


