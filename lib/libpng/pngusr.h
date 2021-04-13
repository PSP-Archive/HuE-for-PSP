// FOR PSP 
//#define fread(ptr,szblk,cnt,fp)   sceIoRead((fp),(ptr),((szblk)*(cnt)))
//#define fwrite(ptr,szblk,cnt,fp)  sceIoWrite((fp),(ptr),((szblk)*(cnt)))

#define PNG_ABORT() while(1);
//#define PNG_NO_STDIO
#define PNG_NO_CONSOLE_IO
#define PNG_SETJMP_NOT_SUPPORTED
#define PNG_NO_FLOATING_POINT_SUPPORTED
#define PNG_NO_FIXED_POINT_SUPPORTED

#define PNG_NO_READ_cHRM
#define PNG_NO_READ_sRGB

#define PNG_NO_WRITE_SUPPORTED
