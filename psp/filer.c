//#include "main.h"

#include "syscall.h"
#include "filer.h"
#include "pg.h"
#include "string.h"

#include "pspstd.h"

#define MAX_PATH 512		//temp, not confirmed

#define MAXPATH 512
#define MAXNAME 256
#define MAX_ENTRY 1024

static struct dirent files[MAX_ENTRY];
static int nfiles;

// 拡張子管理用
const struct {
	char *szExt;
	int nExtId;
} stExtentions[] = {
    {"pce",EXT_PCE},
    {"toc",EXT_TOC},
    {"zip",EXT_ZIP},
    {NULL, EXT_UNKNOWN}
};

////////////////////////////////////////////////////////////////////////
// クイックソート
int cmpFile(struct dirent *a, struct dirent *b)
{
	char ca, cb;
	int i, n, ret;
	
	if(a->type==b->type){
		n=strlen(a->name);
		for(i=0; i<=n; i++){
			ca=a->name[i]; cb=b->name[i];
			if(ca>='a' && ca<='z') ca-=0x20;
			if(cb>='a' && cb<='z') cb-=0x20;
			
			ret = ca-cb;
			if(ret!=0) return ret;
		}
		return 0;
	}
	
	if(a->type & TYPE_DIR)	return -1;
	else					return 1;
}

////////////////////////////////////////////////////////////////////////
void sort(struct dirent *a, int left, int right) {
	struct dirent tmp, pivot;
	int i, p;
	
	if (left < right) {
		pivot = a[left];
		p = left;
		for (i=left+1; i<=right; i++) {
			if (cmpFile(&a[i],&pivot)<0){
				p=p+1;
				tmp=a[p];
				a[p]=a[i];
				a[i]=tmp;
			}
		}
		a[left] = a[p];
		a[p] = pivot;
		sort(a, left, p-1);
		sort(a, p+1, right);
	}
}


////////////////////////////////////////////////////////////////////////
int getExtId(const char *szFilePath) {
	char *pszExt;
	int i;
	if((pszExt = strrchr(szFilePath, '.'))) {
		pszExt++;
		for (i = 0; stExtentions[i].nExtId != EXT_UNKNOWN; i++) {
			if (!stricmp(stExtentions[i].szExt,pszExt)) {
				return stExtentions[i].nExtId;
			}
		}
	}
	return EXT_UNKNOWN;
}


////////////////////////////////////////////////////////////////////////
void getDir(const char *path,int *fileExt) {
    int i;
	int fd, b=0;
    int ext;
//	char *p;
	
	nfiles = 0;
	
	if(strcmp(path,"ms0:/")){
		strcpy(files[nfiles].name,"..");
		files[nfiles].type = TYPE_DIR;
		nfiles++;
		b=1;
	}
	
	fd = sceIoDopen(path);
	while(nfiles<MAX_ENTRY){
		if(sceIoDread(fd, &files[nfiles])<=0) break;
		if(files[nfiles].name[0] == '.') continue;
		if(files[nfiles].type == TYPE_DIR){
			strcat(files[nfiles].name, "/");
			nfiles++;
			continue;
		}
        
        ext = getExtId(files[nfiles].name);

        if(fileExt==0 || fileExt[0]==EXT_ALL) {
            nfiles++;
        } else {
            for(i=0;i<sizeof(stExtentions)/sizeof(stExtentions[0]);i++) {
                if(fileExt[i]==EXT_NULL) break;
                if(fileExt[i]==ext) {
                    nfiles++;
                    break;
                }
            }
        }
	}
	sceIoDclose(fd);
	if(b) sort(files+1, 0, nfiles-2);
	else  sort(files, 0, nfiles-1);
}

char LastPath[MAX_PATH] = {"\0"};
char FilerMsg[256];
char FileName[MAX_PATH];


////////////////////////////////////////////////////////////////////////
int getFilePath(char *out,char *pDefPath,int * fileExt)
{
	unsigned long color=RGB_WHITE;
	int sel=0, rows=21, top=0, x, y, h, i, /*len,*/ bMsg=0, up=0;
	char path[MAXPATH], oldDir[MAXNAME], *p;
    int new_pad=0;

    if(strlen(LastPath)==0) strcpy(path,pDefPath);
    else                     strcpy(path,LastPath);
    
    if(FilerMsg[0])
		bMsg=1;
	
	getDir(path,fileExt);

    load_menu_bg("menu.bmp");
    
    for(;;){

        pgFillBmp(0);

		new_pad = readpad_new();
        
		if(new_pad)
			bMsg=0;
        
		if(new_pad & CTRL_CIRCLE){
			if(files[sel].type == TYPE_DIR){
				if(!strcmp(files[sel].name,"..")){  up=1; }
                else{
					strcat(path,files[sel].name);
					getDir(path,fileExt);
					sel=0;
				}
			}else{
				strcpy(out, path);
				strcat(out, files[sel].name);
				strcpy(LastPath,path);
				return 1;
			}
		}
        else if(new_pad & CTRL_CROSS)   { return 0; }
        else if(new_pad & CTRL_TRIANGLE){ up=1;     }
        else if(new_pad & CTRL_UP)      { sel--;    }
        else if(new_pad & CTRL_DOWN)    { sel++;    }
        else if(new_pad & CTRL_LEFT)    { sel-=10;  }
        else if(new_pad & CTRL_RIGHT)   { sel+=10;  }
        // 左か右のトリガーボタンを押すと名前が違うファイルまで移動するっぽ
        else {
            int findp=0;
            if(new_pad & CTRL_LTRIGGER){ findp=-1; }
            if(new_pad & CTRL_RTRIGGER){ findp=+1;  }
            
            if(findp!=0) {
                int i;
                char selc = files[sel].name[0];
                for(i=sel+findp;1;i+=findp) {
                    if(i<=0) { sel=0; break; }
                    if(i>=nfiles-1) { sel=nfiles-1; break; }
                    if(files[i].name[0]!=selc) {
                        sel=i;
                        break;
                    }
                }
            }
        }

        
		if(up){
			if(strcmp(path,"ms0:/")){
				p=strrchr(path,'/');
				*p=0;
				p=strrchr(path,'/');
				p++;
				strcpy(oldDir,p);
				strcat(oldDir,"/");
				*p=0;
				getDir(path,fileExt);
				sel=0;
				for(i=0; i<nfiles; i++) {
					if(!strcmp(oldDir, files[i].name)) {
						sel=i;
						top=sel-3;
						break;
					}
				}
			}
			up=0;
		}
		
		if(top > nfiles-rows)	top=nfiles-rows;
		if(top < 0)				top=0;
		if(sel >= nfiles)		sel=nfiles-1;
		if(sel < 0)				sel=0;
		if(sel >= top+rows)		top=sel-rows+1;
		if(sel < top)			top=sel;
		
        if(bMsg) {
          mh_print(0,0,FilerMsg,RGB_WHITE);
          mh_print(0,10,"●：OK　×：CANCEL　▲：UP　",RGB_WHITE);
//			rin_frame(FilerMsg,"○：OK　×：CANCEL　△：UP");
        }
        else {
          mh_print(0,0,path,RGB_WHITE);
          mh_print(0,10,"●：OK　×：CANCEL　▲：UP　",RGB_WHITE);
//			rin_frame(path,"○：OK　×：CANCEL　△：UP");
        }

		// スクロールバー
		if(nfiles > rows){
			h = 219;
//			pgDrawFrame(445,25,446,248,setting.color[1]);
			pgDrawFrame(445,25,446,248,RGB_YELLOW);
			pgFillBox(448, h*top/nfiles + 27,
				460, h*(top+rows)/nfiles + 27,RGB_BLUE);
//				460, h*(top+rows)/nfiles + 27,setting.color[1]);
		}
		
		x=28; y=32;
		for(i=0; i<rows; i++){
			if(top+i >= nfiles) break;
			if(top+i == sel) color = RGB_RED;
			else			 color = RGB_WHITE;
//			if(top+i == sel) color = setting.color[2];
//			else			 color = setting.color[3];
			mh_print(x, y, files[top+i].name, color);
			y+=10;
		}
		
        pgScreenFlipV();
        
        if(PSP_IsEsc()) {
            return -1;
        }
	}
}
/*
int InitFiler(char*msg,char*path)
{
    strcpy(FilerMsg,msg);
    strcpy(LastPath,path);
    memset(files,0,sizeof(files));
    return 1;
}
*/
