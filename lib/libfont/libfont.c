#define PRINT_BGCOLOR  0

static int back_fill = 0;

#include "libfont.h"

extern const unsigned char hankaku_font10[];
extern const unsigned short zenkaku_font10[];
extern const unsigned char font[];

void pgPrint(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
	while (*str!=0 && x<CMAX_X && y<CMAX_Y) {
		pgPutChar(x*8,y*8,color,0,*str,1,0,1);
		str++;
		x++;
		if (x>=CMAX_X) {
			x=0;
			y++;
		}
	}
}

void pgPrint2(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
	while (*str!=0 && x<CMAX2_X && y<CMAX2_Y) {
		pgPutChar(x*16,y*16,color,0,*str,1,0,2);
		str++;
		x++;
		if (x>=CMAX2_X) {
			x=0;
			y++;
		}
	}
}


void pgPrint4(unsigned long x,unsigned long y,unsigned long color,const char *str)
{
	while (*str!=0 && x<CMAX4_X && y<CMAX4_Y) {
		pgPutChar(x*32,y*32,color,0,*str,1,0,4);
		str++;
		x++;
		if (x>=CMAX4_X) {
			x=0;
			y++;
		}
	}
}

void pgPutChar(unsigned long x,unsigned long y,unsigned long color,unsigned long bgcolor,unsigned char ch,char drawfg,char drawbg,char mag)
{
	unsigned char *vptr0;		//pointer to vram
	unsigned char *vptr;		//pointer to vram
	const unsigned char *cfont;		//pointer to font
	unsigned long cx,cy;
	unsigned long b;
	char mx,my;

	if (ch>255) return;
	cfont=font+ch*8;
	vptr0=pgGetVramAddr(x,y);
    
	for(cy=0; cy<8; cy++) {
		for(my=0; my<mag; my++) {
			vptr=vptr0;
			b=0x80;
			for (cx=0; cx<8; cx++) {
				for (mx=0; mx<mag; mx++) {
					if ((*cfont&b)!=0) {
						if (drawfg) *(unsigned short *)vptr=(short)color;
					} else {
						if (drawbg) *(unsigned short *)vptr=(short)bgcolor;
					}
					vptr+=PIXELSIZE*2;
				}
				b=b>>1;
			}
			vptr0+=LINESIZE*2;
		}
		cfont++;
	}
}


// by kwn
void Draw_Char_Hankaku(int x,int y,const unsigned char c,int col) {
	unsigned short *vr;
	unsigned char  *fnt;
	unsigned char  pt;
	unsigned char ch;
	int x1,y1;

	ch = c;

	// mapping
	if (ch<0x20)
		ch = 0;
	else if (ch<0x80)
		ch -= 0x20;
	else if (ch<0xa0)
		ch = 0;
	else
		ch -= 0x40;

	fnt = (unsigned char *)&hankaku_font10[ch*10];

	// draw
	vr = (unsigned short *)pgGetVramAddr(x,y);
	for(y1=0;y1<10;y1++) {
		pt = *fnt++;
		for(x1=0;x1<5;x1++) {
			if (pt & 1) {
				*vr = col;
			} else {
				if(back_fill)
	                *vr = PRINT_BGCOLOR;
			}
			vr++;
			pt = pt >> 1;
		}
		vr += LINESIZE-5;
	}
}


// by kwn
void Draw_Char_Zenkaku(int x,int y,const unsigned char u,unsigned char d,int col) {
	// ELISA100.FNTに存在しない文字
	const unsigned short font404[] = {
		0xA2AF, 11,
		0xA2C2, 8,
		0xA2D1, 11,
		0xA2EB, 7,
		0xA2FA, 4,
		0xA3A1, 15,
		0xA3BA, 7,
		0xA3DB, 6,
		0xA3FB, 4,
		0xA4F4, 11,
		0xA5F7, 8,
		0xA6B9, 8,
		0xA6D9, 38,
		0xA7C2, 15,
		0xA7F2, 13,
		0xA8C1, 720,
		0xCFD4, 43,
		0xF4A5, 1030,
		0,0
	};
	unsigned short *vr;
	unsigned short *fnt;
	unsigned short pt;
	int x1,y1;

	unsigned long n;
	unsigned short code;
    int j;//	int i, j;

	// SJISコードの生成
	code = u;
	code = (code<<8) + d;

	// SJISからEUCに変換
	if(code >= 0xE000) code-=0x4000;
	code = ((((code>>8)&0xFF)-0x81)<<9) + (code&0x00FF);
	if((code & 0x00FF) >= 0x80) code--;
	if((code & 0x00FF) >= 0x9E) code+=0x62;
	else code-=0x40;
	code += 0x2121 + 0x8080;

	// EUCから恵梨沙フォントの番号を生成
	n = (((code>>8)&0xFF)-0xA1)*(0xFF-0xA1)
		+ (code&0xFF)-0xA1;
	j=0;
	while(font404[j]) {
		if(code >= font404[j]) {
			if(code <= font404[j]+font404[j+1]-1) {
				n = -1;
				break;
			} else {
				n-=font404[j+1];
			}
		}
		j+=2;
	}
	fnt = (unsigned short *)&zenkaku_font10[n*10];

	// draw
	vr = (unsigned short *)pgGetVramAddr(x,y);
	for(y1=0;y1<10;y1++) {
		pt = *fnt++;
		for(x1=0;x1<10;x1++) {
			if (pt & 1) {
				*vr = col;
			} else {
				if(back_fill)
	                *vr = PRINT_BGCOLOR;
			}
			vr++;
			pt = pt >> 1;
		}
		vr += LINESIZE-10;
	}
}

// by kwn
void mh_print(int x,int y,const unsigned char *str,int col) {
	unsigned char ch = 0,bef = 0;

	while(*str != 0) {
		ch = *str++;
		if (bef!=0) {
			Draw_Char_Zenkaku(x,y,bef,ch,col);
			x+=10;
			bef=0;
		} else {
			if (((ch>=0x80) && (ch<0xa0)) || (ch>=0xe0)) {
				bef = ch;
			} else {
				Draw_Char_Hankaku(x,y,ch,col);
				x+=5;
			}
		}
	}
}

//////////////////////////
void pgPrintHex(int x,int y,short col,unsigned int hex)
{
    char string[9],o;
    int i,a;
    memset(string,0,sizeof(string));

    for(i=0;i<8;i++) {
        a = (hex & 0xf0000000)>>28;
        switch(a) {
          case 0x0a: o='A'; break;
          case 0x0b: o='B'; break;
          case 0x0c: o='C'; break;
          case 0x0d: o='D'; break;
          case 0x0e: o='E'; break;
          case 0x0f: o='F'; break;
          default:   o='0'+a; break;
        }
        hex<<=4;
        string[i]=o;
    }
    pgPrint(x,y,col,string);
}

void pgPrintDec(int x,int y,short col,unsigned int dec)
{
    char string[12],*last=string;
    int i,a;
    memset(string,0,sizeof(string));
    
    for(i=0;i<11;i++) {
        a = dec % 10;
    	dec/=10;
        string[10-i]=0x30+a;
        last=&string[10-i];
        if(dec==0) { break; }
//        if(dec=0 && a==0) break;
    }
    pgPrint(x,y,col,last);
}


//////////////////////////
void mh_print_hex2(int x,int y,unsigned int hex,short col)
{
    char string[3],o;
    int i,a;
    memset(string,0,sizeof(string));

    for(i=0;i<2;i++) {
        a = (hex & 0xf0)>>4;
        switch(a) {
          case 0x0a: o='A'; break;
          case 0x0b: o='B'; break;
          case 0x0c: o='C'; break;
          case 0x0d: o='D'; break;
          case 0x0e: o='E'; break;
          case 0x0f: o='F'; break;
          default:   o='0'+a; break;
        }
        hex<<=4;
        string[i]=o;
    }
    string[2]=0;
	back_fill=1;
    mh_print(x,y,string,col);
	back_fill=0;
}

void mh_print_hex4(int x,int y,unsigned int hex,short col)
{
    mh_print_hex2(x   ,y,hex>> 8,col);
    mh_print_hex2(x+10,y,hex    ,col);
}

void mh_print_hex8(int x,int y,unsigned int hex,short col)
{
    mh_print_hex2(x   ,y,hex>>24,col);
    mh_print_hex2(x+10,y,hex>>16,col);
    mh_print_hex2(x+20,y,hex>> 8,col);
    mh_print_hex2(x+30,y,hex    ,col);
}

void mh_print_hex(int x,int y,unsigned int hex,short col)
{
    if(hex&0xff000000) { mh_print_hex2(x,y,hex>>24,col); x+=10; }
    if(hex&0xffff0000) { mh_print_hex2(x,y,hex>>16,col); x+=10; }
    if(hex&0xffffff00) { mh_print_hex2(x,y,hex>> 8,col); x+=10; }
    if(hex&0xffffffff) { mh_print_hex2(x,y,hex    ,col); }
}

void mh_print_dec(int x,int y,unsigned int dec,short col)
{
    char string[12],*last=string;
    int i,a;
    memset(string,0,sizeof(string));
    
    for(i=0;i<11;i++) {
        a = dec % 10;
    	dec/=10;
        string[10-i]=0x30+a;
        last=&string[10-i];
        if(dec==0) { break; }
//        if(dec=0 && a==0) break;
    }
	back_fill=1;
    mh_print(x,y,last,col);
	back_fill=0;
}

