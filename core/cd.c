///////////////////////////////////////////////////////////////////
// 
// CDROM対応にあたり派手に PCEP070のソースコードを引用させて頂きました。
// 
//////////////////////////////////////////////////////////////////
#include "stdinc.h"
#include "pce.h"
#include "string.h"
#include "main.h"

typedef long seek_unit;

// =-=-=- For CDROM^2
#define CD_FRAMES 75
#define CD_SECS 60
#define CD_BUF_LENGTH 8


char cd_last_read[1024]={"test"};

// byte cd_buf[CD_BUF_LENGTH*2048]; 2005.11.22

//BYTE can_write_debug = 0;
//byte CD_emulation = 0;
Track CD_track[0x100];
// Track
// beg_min -> beginning in minutes since the begin of the CD(BCD)
// beg_sec -> beginning in seconds since the begin of the CD(BCD)
// beg_fr -> beginning in frames   since the begin of the CD(BCD)
// type -> 0 = audio, 4 = data
// beg_lsn -> beginning in number of sector (2048 bytes)
// length -> number of sector
byte cd_fade;
// the byte set by the fade function
// extra ram provided by the system CD card

DWORD  msf2nb_sect(byte min, byte sec, byte fra);
void    nb_sect2msf(DWORD lsn,byte *min, byte *sec, byte *frm);
void fill_cd_info();


unsigned char binbcd[0x100] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09,
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19,
	0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
	0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
	0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,
	0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,
	0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,
	0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
	0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99,
};


unsigned char bcdbin[0x100] = {
	0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0, 0, 0, 0, 0, 0,
	0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13, 0, 0, 0, 0, 0, 0,
	0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0, 0, 0, 0, 0, 0,
	0x1e, 0x1f, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0, 0, 0, 0, 0, 0,
	0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 0x30, 0x31, 0, 0, 0, 0, 0, 0,
	0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0, 0, 0, 0, 0, 0,
	0x3c, 0x3d, 0x3e, 0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0, 0, 0, 0, 0, 0,
	0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 0, 0, 0, 0, 0, 0,
	0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0, 0, 0, 0, 0, 0,
	0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 0x60, 0x61, 0x62, 0x63, 0, 0, 0, 0, 0, 0,
};


byte cd_port_1800 = 0;
byte cd_port_1801 = 0;
byte cd_port_1802 = 0;
byte cd_port_1804 = 0;
byte cd_port_180b = 0;
byte pce_cd_adpcm_trans_done = 0;
byte pce_cd_curcmd;
byte pce_cd_cmdcnt;
byte cd_sectorcnt;

DWORD packed_iso_filesize = 0;
byte cd_sector_buffer[0x2000];	// contain really data
// DWORD pce_cd_read_datacnt;
DWORD pce_cd_sectoraddy;
DWORD pce_cd_read_datacnt;
byte pce_cd_sectoraddress[3];
byte pce_cd_temp_dirinfo[4];
byte pce_cd_temp_play[4];
byte pce_cd_temp_stop[4];
byte *cd_read_buffer;
byte pce_cd_dirinfo[4];
// struct cdrom_tocentry pce_cd_tocentry;
byte nb_max_track = 24;	//(NO MORE BCD!!!!!)
//char cdsystem_path[256];
//extern char   *pCartName;
//extern char snd_bSound;
// Pre declaration of reading function routines


// define
#define CD_MSF_OFFSET 150

// internal function
int cd_toc_read(void);

typedef struct
{
    unsigned int   LBA;
	byte  min;
	byte  sec;
	byte  fra;
    byte  type;
}struct_cd_toc;

struct_cd_toc	cd_toc[0x100];

void issue_ADPCM_dma(void);
void pce_cd_handle_command(void);

//=============================================================================
//
//
//
//=============================================================================
void CD_init(void)
{
//    CD_emulation = 0;
}

//=============================================================================
// 
// Cd write access
// 
//=============================================================================
void CD_write(word A,byte V)
{
    switch(A&15){
      case 7: cd.backup = ENABLE; return;
        /*	case 8: io.adpcm_ptr.B.l = V; return;
		case 9: io.adpcm_ptr.B.h = V; return;
		case 0xa: PCM[io.adpcm_wptr++] = V; return;
		case 0xd:
			if (V&4) io.adpcm_wptr = io.adpcm_ptr.W;
			else { io.adpcm_rptr = io.adpcm_ptr.W; io.adpcm_firstread = 1; }
			return;
	*/
      case 0: if (V == 0x81) cd_port_1800 = 0xD0; return;
        
      case 1:
        cd_port_1801 = V;
        if (!pce_cd_cmdcnt) {
            switch (V) {
              case 0x81:	// Another Reset?
                cd_port_1800 = 0x40;
                return;
              case 0:		// RESET?
              case 3:		// Get System Status?
              case 8:		// Read Sector
              case 0xD8:	// Play Audio?
              case 0xD9:	// Play Audio?
              case 0xDA:	// Pause Audio?
              case 0xDD:	// Read Q Channel?
              case 0xDE:	// Get Directory Info?
              default:
                return;
            }
        }
        return;
        
      case 2:
        if ((!(cd_port_1802 & 0x80)) && (V & 0x80)) {
            cd_port_1800 &= ~0x40;
        } else if ((cd_port_1802 & 0x80) && (!(V & 0x80))) {
            cd_port_1800 |= 0x40;
            if (pce_cd_adpcm_trans_done) {
                cd_port_1800 |= 0x10;
                pce_cd_curcmd = 0x00;
                pce_cd_adpcm_trans_done = 0;
            }
            
            if (cd_port_1800 & 0x08) {
                if (cd_port_1800 & 0x20) {
                    cd_port_1800 &= ~0x80;
                } else if (!pce_cd_read_datacnt) {
                    if (pce_cd_curcmd == 0x08) {
                        if (!--cd_sectorcnt) {
                            cd_port_1800 |= 0x10;	/* wrong */
                            pce_cd_curcmd = 0x00;
                        } else {
                            pce_cd_read_sector();
                        }
                    } else {
                        if (cd_port_1800 & 0x10) {
                            cd_port_1800 |= 0x20;
                        } else {
                            cd_port_1800 |= 0x10;
                        }
                    }
                } else {
                    pce_cd_read_datacnt--;
                }
            } else {
                pce_cd_handle_command();
            }
        }
        
        cd_port_1802 = V;
        return;
        
      case 4:
        if (V & 2) {
            // Reset asked
            // do nothing for now
//            CD_emulation=1;
            cd_toc_read();
            fill_cd_info();
            
            _Wr6502(0x222D, 1);
            // This byte is set to 1 if a disc if present
            //cd_port_1800 &= ~0x40;
            cd_port_1804 = V;
        } else {
            // Normal utilisation
            cd_port_1804 = V;
            // cd_port_1800 |= 0x40; // Maybe the previous reset is enough
            // cd_port_1800 |= 0xD0;
            // Indicates that the Hardware is ready after such a reset
        }
        return;
        
      case 8:
        cd.adpcm_ptr.B.l = V;
        return;
        
      case 9:
        cd.adpcm_ptr.B.h = V;
        return;
        
      case 0x0A:
        cd.PCM[cd.adpcm_wptr++] = V;
        return;
        
      case 0x0B:		// DMA enable ?
        if ((V & 2) && (!(cd_port_180b & 2))) {
            issue_ADPCM_dma ();
            cd_port_180b = V;
            return;
        }
        /* TEST */
        if (!V) {
            cd_port_1800 &= ~0xF8;
            cd_port_1800 |= 0xD8;
        }
        cd_port_180b = V;
        return;
        
      case 0x0C:		/* TEST, not nyef code */
        // well, do nothing
        return;
        
      case 0x0D:
        if ((V & 0x03) == 0x03) {
            cd.adpcm_dmaptr = cd.adpcm_ptr.W;	// set DMA pointer
        }
        
        if (V & 0x04) {
            cd.adpcm_wptr = cd.adpcm_ptr.W;	// set write pointer
        }
        
        if (V & 0x08) {		// set read pointer
            cd.adpcm_rptr = cd.adpcm_ptr.W;
				cd.adpcm_firstread = 2;
        }
        
        if (V & 0x80) {			// ADPCM reset
        } else {			// Normal ADPCM utilisation
        }
        return;
        
      case 0xe:		// Set ADPCM playback rate
        cd.adpcm_rate = 32 / (16 - (V & 15));
        return;
        
      case 0xf:		// don't know how to use it
        cd_fade = V;
			return;
		}			// A&15 switch, i.e. CD ports
}


//=============================================================================
// 
// CD read access
// 
//=============================================================================
byte CD_read(word A)
{
    if((A&0x18c0)==0x18c0) {
        switch (A & 15) {
          case 5:
          case 1:  return 0xAA;
          case 2:
          case 6:  return 0x55;
          case 3:
          case 7:  return 0x03;
            // case 15: // ACD support ?
            //  return 0x51;
        }
        return 0xff;
    }
    
    switch(A&15){
      case 0:
        return cd_port_1800;	// return 0x40; // ready ?
        break;
      case 1: {
          byte retval;
          
          if(cd_read_buffer) {
              retval = *cd_read_buffer++;
              if (pce_cd_read_datacnt == 2048) {
                  pce_cd_read_datacnt--;
              }
              if (!pce_cd_read_datacnt)
                cd_read_buffer = 0;
          } else
            retval = 0;
          return retval;
      }
        
      case 2: return cd_port_1802;	// Test
        //	case 3: return io.backup = DISABLE;
      case 3: {
          static byte tmp_res = 0x02;
          tmp_res = 0x02 - tmp_res;
          cd.backup = DISABLE;
          /* TEST */// return 0x20;
          return tmp_res | 0x20;
      }
      case 4: return cd_port_1804;	// Test
      case 5: return 0x50;			// Test
      case 6: return 0x05;			// Test
      case 0x0A:
        if (!cd.adpcm_firstread)
          return cd.PCM[cd.adpcm_rptr++];
        else {
            cd.adpcm_firstread--;
            return NODATA;
        }
        
      case 0x0B: return 0x00;			// Test
      case 0x0C: return 0x01;			// Test
      case 0x0D: return 0x00;			// Test
      case 8:
        if (pce_cd_read_datacnt) {
            byte retval;
            if (cd_read_buffer) {
                retval = *cd_read_buffer++;
            } else
              retval = 0;
            
            if (!--pce_cd_read_datacnt) {
                cd_read_buffer = 0;
                if (!--cd_sectorcnt) {
                    cd_port_1800 |= 0x10;
                    pce_cd_curcmd = 0;
                } else {
                    pce_cd_read_sector();
                }
            }
            return retval;
        }
        break;
    }
	return 0xff;
}


//=============================================================================
//
// CDからTOC情報を読みます
//
//=============================================================================
int cd_toc_read(void)
{
	char toc_buf[20480];
	char *p;
	int	size,fd;

	memset( cd_toc, 0, sizeof(cd_toc) );
	
	fd = sceIoOpen( eConf.cdrom, 1, 0777);
	if(fd<0){
		//Error_mes("cd toc read error");
        pgPrint(0,0,-1,"cd toc read error");
        pgScreenFlipV();
		return 1;
	}
    
	size = sceIoRead(fd, toc_buf, 20480);
	sceIoClose(fd);
	p=toc_buf;

    {
		int	n=0,f=0,c=0,s=0;
        
		while(size>0){
			if( *p>='0' && *p<='9' ){
				s*=10;
				s+=*p-'0';
				f=1;
			}else{
				if( f ){
					switch(c){
					case	0:				n = s;	c++;	break;
					case	1:	cd_toc[n].min = s;	c++;	break;
					case	2:	cd_toc[n].sec = s;	c++;	break;
					case	3:	cd_toc[n].fra = s;	c++;	break;
					case	4:	cd_toc[n].LBA = s;	c=0;	
						n++;
						if( n>=0x100 )size=0;
						break;
					}
				}
				if( !memcmp( p, "Data",  4 ) ) cd_toc[n].type=4;
				if( !memcmp( p, "Audio", 5 ) ) cd_toc[n].type=0;
				f=0;
				s=0;
			}
			p++;
			size--;
		}
		nb_max_track=n-1;
	}
	
	return 0;
}


//=============================================================================
//
// CDからTOC情報を読みます
//
//=============================================================================
int cd_track_search(int m,int s,int f)
{
	int i;
	for(i=0;i<0x100;i++){
		if(	cd_toc[i].min==m &&
			cd_toc[i].sec==s &&
			cd_toc[i].fra==f ) {
            return	i;
        }
    }
    return 0;
}


//=============================================================================
//
//
//
//=============================================================================
void fill_cd_info()
{
    byte Min, Sec, Fra;
    byte current_track;

    // Track 1 is almost always a audio avertising track
    // 30 sec. seems usual
    
    CD_track[1].beg_min = binbcd[00];
    CD_track[1].beg_sec = binbcd[02];
    CD_track[1].beg_fra = binbcd[00];
    
    CD_track[1].type = 0;
    CD_track[1].beg_lsn = 0;	// Number of sector since the
    // beginning of track 1
    
    CD_track[1].length = 47 * CD_FRAMES + 65;
    
    // CD_track[0x01].length=53 * CD_FRAMES + 65;
    
    // CD_track[0x01].length=0 * CD_FRAMES + 16;
    
    nb_sect2msf (CD_track[1].length, &Min, &Sec, &Fra);
    
    // Fra = CD_track[0x01].length % CD_FRAMES;
    // Sec = (CD_track[0x01].length) % (CD_FRAMES * CD_SECS) / CD_SECS;
    // Min = (CD_track[0x01].length) (CD_FRAMES * CD_SECS);
    
    // Second track is the main code track
    
    CD_track[2].beg_min = binbcd[bcdbin[CD_track[1].beg_min] + Min];
    CD_track[2].beg_sec = binbcd[bcdbin[CD_track[1].beg_sec] + Sec];
    CD_track[2].beg_fra = binbcd[bcdbin[CD_track[1].beg_fra] + Fra];
    
    CD_track[2].type = 4;
    CD_track[2].beg_lsn =
      msf2nb_sect (bcdbin[CD_track[2].beg_min] - bcdbin[CD_track[1].beg_min],
                   bcdbin[CD_track[2].beg_sec] - bcdbin[CD_track[1].beg_sec],
                   bcdbin[CD_track[2].beg_fra] - bcdbin[CD_track[1].beg_fra]);

    CD_track[0x02].length = 140000;
    
    // Now most track are audio
    for (current_track = 3; current_track < bcdbin[nb_max_track];current_track++) {
        
        Fra = (byte) (CD_track[current_track - 1].length % CD_FRAMES);
        Sec = (byte) ((CD_track[current_track - 1].length / CD_FRAMES) % CD_SECS);
        Min = (byte) ((CD_track[current_track - 1].length / CD_FRAMES) / CD_SECS);

        CD_track[current_track].beg_min = binbcd[bcdbin[CD_track[current_track - 1].beg_min] + Min];
        CD_track[current_track].beg_sec = binbcd[bcdbin[CD_track[current_track - 1].beg_sec] + Sec];
        CD_track[current_track].beg_fra = binbcd[bcdbin[CD_track[current_track - 1].beg_fra] + Fra];
        
        CD_track[current_track].type = 0;
        CD_track[current_track].beg_lsn =
          msf2nb_sect (bcdbin[CD_track[current_track].beg_min] -
                       bcdbin[CD_track[1].beg_min],
                       bcdbin[CD_track[current_track].beg_sec] -
                       bcdbin[CD_track[1].beg_sec],
                       bcdbin[CD_track[current_track].beg_fra] -
                       bcdbin[CD_track[1].beg_fra]);
        // 1 min for all
        CD_track[current_track].length = 1 * CD_SECS * CD_FRAMES;
    }
    
    // And the last one is generally also code

    Fra = (byte) (CD_track[nb_max_track - 1].length % CD_FRAMES);
    Sec = (byte) ((CD_track[nb_max_track - 1].length / CD_FRAMES) % CD_SECS);
    Min = (byte) ((CD_track[nb_max_track - 1].length / CD_FRAMES) / CD_SECS);
    
    CD_track[nb_max_track].beg_min = binbcd[bcdbin[CD_track[nb_max_track - 1].beg_min] + Min];
    CD_track[nb_max_track].beg_sec = binbcd[bcdbin[CD_track[nb_max_track - 1].beg_sec] + Sec];
    CD_track[nb_max_track].beg_fra = binbcd[bcdbin[CD_track[nb_max_track - 1].beg_fra] + Fra];
    
    CD_track[nb_max_track].type = 4;
    CD_track[nb_max_track].beg_lsn =
      msf2nb_sect (bcdbin[CD_track[nb_max_track].beg_min] - bcdbin[CD_track[1].beg_min],
                   bcdbin[CD_track[nb_max_track].beg_sec] - bcdbin[CD_track[1].beg_sec],
                   bcdbin[CD_track[nb_max_track].beg_fra] - bcdbin[CD_track[1].beg_fra]);

    CD_track[nb_max_track].length = 14000;

    return;
}



//=============================================================================
//
//
//
//=============================================================================
void cd_test_read(char *p,DWORD s)
{
    int fd;
    int i;
	char n[1024];
	char tn[3];
    char *wp;

    for(i=1;i<0x100;i++){
        if( cd_toc[i].LBA>s )break;
    }
    
    i--;
    tn[0]='0'+(i/10);
    tn[1]='0'+(i%10);
    tn[2]=0;
    strcpy( n, eConf.cdrom);
    wp=strrchr( n, '/' );wp[1]=0;
	strcat( n, tn);
    strcat( n, ".iso");
    //Error_mes(n);

    strcpy(cd_last_read,n);

    //mh_print(400,0,n,RGB_WHITE);

    fd = sceIoOpen( n,1, 0777);
    
    if(fd>=0){
        seek_unit a;
        DWORD w;
        a = (s-cd_toc[i].LBA)*2048;
        w= sceIoLseek(fd,(long long)0, 2);
        if( a<0 || w<a+2048 ){
            //Error_mes("CDシークエラー");
            mh_print(400,10,"cd seek error",-1);
            mh_print(400,20,n,-1);
            //mh_print_hex8(400,20,w,-1);
            //mh_print_hex8(400,30,a,-1);
            //mh_print_hex8(400,40,i,-1);
            //mh_print_hex8(400,50,s,-1);
            //mh_print_hex8(400,60,cd_toc[i].LBA,-1);
        }else{
            int rd;
            w = sceIoLseek( fd, (long long)a, 0);
            rd = sceIoRead(fd, p, 2048);
            
            //mh_print     (400,10,"Read Status",-1);
            //mh_print_hex8(400,20,w,-1);
            //mh_print_hex8(400,30,a,-1);
            //mh_print_hex8(400,40,rd,-1);
            //mh_print_hex8(400,50,s,-1);
            //mh_print_hex8(400,60,cd_toc[i].LBA,-1);
        }
        sceIoClose(fd);
    }else{
        mh_print(0,0,"cd open error",-1);
        mh_print(0,1,n,-1);
        //pgScreenFlipV();
        //pgWaitVn(100);
        //Error_mes("CDオープンエラー");
        //Error_mes(n);
	}
}


//	2トラック目 3590
DWORD first_sector = 0;

//=============================================================================
//
//
//
//=============================================================================
void read_sector_CD(unsigned char *p, DWORD sector)
{
    int i;

    pCddaStopFunc();
    
    if ((sector >= first_sector) && (sector <= first_sector + CD_BUF_LENGTH - 1)) {
        memcpy(p, cd.cd_buf + 2048 * (sector - first_sector), 2048);
        return;
    }
    else {
        for (i = 0; i < CD_BUF_LENGTH; i++)
          cd_test_read(cd.cd_buf + 2048 * i, sector + i);
        first_sector = sector;
        memcpy(p, cd.cd_buf, 2048);
    }
} 


//=============================================================================
//
//
//
//=============================================================================
void pce_cd_read_sector(void)
{
    read_sector_CD( cd_sector_buffer, pce_cd_sectoraddy );
    /* Avoid sound jiggling when accessing some sectors */
    pce_cd_sectoraddy++;
    pce_cd_read_datacnt = 2048;
    cd_read_buffer = cd_sector_buffer;
    /* restore sound volume */
}


//=============================================================================
//
//
//
//=============================================================================
void lba2msf (int lba, unsigned char *msf)
{
    lba += CD_MSF_OFFSET;
    msf[0] = binbcd[lba / (CD_SECS * CD_FRAMES)];
    lba %= CD_SECS * CD_FRAMES;
    msf[1] = binbcd[lba / CD_FRAMES];
    msf[2] = binbcd[lba % CD_FRAMES];
}


//=============================================================================
//
//
//
//=============================================================================
DWORD msf2nb_sect (byte min, byte sec, byte frm)
{
    DWORD result = frm;
    result += sec * CD_FRAMES;
    result += min * CD_FRAMES * CD_SECS;
    return result;
}

//=============================================================================
//
//
//
//=============================================================================
void nb_sect2msf (DWORD lsn, byte * min, byte * sec, byte * frm)
{
    (*frm) = (byte) (lsn % CD_FRAMES);
    lsn /= CD_FRAMES;
    (*sec) = (byte) (lsn % CD_SECS);
    (*min) = (byte) (lsn / CD_SECS);
    return;
}

void pce_cd_set_sector_address(void);


//=============================================================================
//
//
//
//=============================================================================
void pce_cd_handle_command(void)
{
    if (pce_cd_cmdcnt) {
        if (--pce_cd_cmdcnt)
          cd_port_1800 = 0xd0;
        else
          cd_port_1800 = 0xc8;
        
        //mh_print    (20,50,"pce_cd_handle_command",RGB_BLUE);
        //mh_print_hex(20,60,pce_cd_curcmd,RGB_RED);
        //pgScreenFlip();
        //pgWaitVn(1);
        
        switch (pce_cd_curcmd) {
          case 0x08:
            if (!pce_cd_cmdcnt) {
                cd_sectorcnt = cd_port_1801;
                pce_cd_set_sector_address();
                pce_cd_read_sector();
                
                /* TEST */
                // cd_port_1800 = 0xD0; // Xanadu 2 doesn't block but still crash
                /* TEST */
                
                /* TEST ZEO
                  if (Rd6502(0x20ff)==0xfe)
                    cd_port_1800 = 0x98;
                  else
                    cd_port_1800 = 0xc8;
                 ******** */
            } else
              pce_cd_sectoraddress[3 - pce_cd_cmdcnt] = cd_port_1801;
            break;
            
          case 0xd8:
            
            pce_cd_temp_play[pce_cd_cmdcnt] = cd_port_1801;
            
            if (!pce_cd_cmdcnt) {
                cd_port_1800 = 0xd8;
            }
            break;
            
          case 0xd9:
            pce_cd_temp_stop[pce_cd_cmdcnt] = cd_port_1801;
            if (!pce_cd_cmdcnt) {
                cd_port_1800 = 0xd8;

                pCddaPlayFunc(bcdbin[pce_cd_temp_play[2]],1);
                
                //cd_PlayTrack(bcdbin[pce_cd_temp_play[2]]);
                
                /*
               if (pce_cd_temp_stop[3] == 1)
                 osd_cd_play_audio_track(bcdbin[pce_cd_temp_play[2]]);
               else
                 */
                if ((pce_cd_temp_play[0] | pce_cd_temp_play[1] | pce_cd_temp_stop[0] | pce_cd_temp_stop[1]) == 0) {
                    //osd_cd_play_audio_track(bcdbin[pce_cd_temp_play[2]]);
                    //cd_PlayTrack(bcdbin[pce_cd_temp_play[2]]);
                } else {
                    //osd_cd_play_audio_range(bcdbin[pce_cd_temp_play[2]], bcdbin[pce_cd_temp_play[1]], bcdbin[pce_cd_temp_play[0]], bcdbin[pce_cd_temp_stop[2]], bcdbin[pce_cd_temp_stop[1]], bcdbin[pce_cd_temp_stop[0]]);

                    // ここから
                    //bcdbin[pce_cd_temp_play[2]], bcdbin[pce_cd_temp_play[1]], bcdbin[pce_cd_temp_play[0]];
                    // ここまでの範囲を
                    //bcdbin[pce_cd_temp_stop[2]], bcdbin[pce_cd_temp_stop[1]], bcdbin[pce_cd_temp_stop[0]];
                    // CD再生して欲しいらしいと言われてもですよ・・・トラック番号はどうなるの？同じ？
                    //cd_PlayWithRange(bcdbin[pce_cd_temp_play[2]], bcdbin[pce_cd_temp_play[1]], bcdbin[pce_cd_temp_play[0]],
                    //                 bcdbin[pce_cd_temp_stop[2]], bcdbin[pce_cd_temp_stop[1]], bcdbin[pce_cd_temp_stop[0]]);
                }
            }
            break;
            
          case 0xde:
            if (pce_cd_cmdcnt)
              pce_cd_temp_dirinfo[pce_cd_cmdcnt] = cd_port_1801;
            else {
                // We have received two arguments in pce_cd_temp_dirinfo
                // We can use only one
                // There's an argument indicating the kind of info we want
                // and an optional argument for track number
                pce_cd_temp_dirinfo[0] = cd_port_1801;
                
				switch (pce_cd_temp_dirinfo[1]) {
                  case 0:
                    // We want info on number of first and last track
/*                    
                    switch (CD_emulation) {
                      case 2:
                      case 3:
                        pce_cd_dirinfo[0] = binbcd[01];	// Number of first track  (BCD)
                        pce_cd_dirinfo[1] = binbcd[nb_max_track];	// Number of last track (BCD)
                        break;
                      case 1: {
                          int first_track, last_track;
                          // 未実装
                          //osd_cd_nb_tracks (&first_track, &last_track);
                          cd_nb_tracks(&first_track,&last_track);
                          pce_cd_dirinfo[0] = binbcd[first_track];
                          pce_cd_dirinfo[1] = binbcd[last_track];
                      }
                        break;
                    }// switch CD emulation
*/
					pce_cd_dirinfo[0] = binbcd[1];
					pce_cd_dirinfo[1] = binbcd[nb_max_track];
  
                    cd_read_buffer = pce_cd_dirinfo;
                    pce_cd_read_datacnt = 2;
                    break;
                    
                  case 2:
                    // We want info on the track whose number is pce_cd_temp_dirinfo[0]
/*                    switch (CD_emulation) {
                      case 2:
                      case 3:
                        pce_cd_dirinfo[0] = CD_track[bcdbin[pce_cd_temp_dirinfo[0]]].beg_min;
                        pce_cd_dirinfo[1] = CD_track[bcdbin[pce_cd_temp_dirinfo[0]]].beg_sec;
                        pce_cd_dirinfo[2] = CD_track[bcdbin[pce_cd_temp_dirinfo[0]]].beg_fra;
                        pce_cd_dirinfo[3] = CD_track[bcdbin[pce_cd_temp_dirinfo[0]]].type;
                        break;
                      case 1: {
                          int Min, Sec, Fra, Ctrl;
                          //byte *buffer = (byte *) alloca (7);
                          // 未実装
                          //osd_cd_track_info (bcdbin[pce_cd_temp_dirinfo[0]], &Min, &Sec, &Fra, &Ctrl);
                          cd_track_info(bcdbin[pce_cd_temp_dirinfo[0]], &Min, &Sec, &Fra, &Ctrl);
                          pce_cd_dirinfo[0] = binbcd[Min];
                          pce_cd_dirinfo[1] = binbcd[Sec];
                          pce_cd_dirinfo[2] = binbcd[Fra];
                          pce_cd_dirinfo[3] = Ctrl;
#ifdef WIN32
                          LogDump("The control byte of the audio track #%d is 0x%02X\n", bcdbin[pce_cd_temp_dirinfo[0]], pce_cd_dirinfo[3]);
#endif//WIN32
                          break;
                      }		// case CD emulation = 1
                    }		// switch CD emulation
*/
                    pce_cd_dirinfo[0] = binbcd[cd_toc[bcdbin[pce_cd_temp_dirinfo[0]]].min];
                    pce_cd_dirinfo[1] = binbcd[cd_toc[bcdbin[pce_cd_temp_dirinfo[0]]].sec];
                    pce_cd_dirinfo[2] = binbcd[cd_toc[bcdbin[pce_cd_temp_dirinfo[0]]].fra];
                    pce_cd_dirinfo[3] = binbcd[cd_toc[bcdbin[pce_cd_temp_dirinfo[0]]].type];
                    
                    pce_cd_read_datacnt = 3;
                    cd_read_buffer = pce_cd_dirinfo;
                    break;
                    
                  case 1:
                    pce_cd_dirinfo[0] = cd_toc[nb_max_track].min;//0x25;
                    pce_cd_dirinfo[1] = cd_toc[nb_max_track].sec;//0x06;
                    pce_cd_dirinfo[2] = cd_toc[nb_max_track].fra;//0x00;
                    pce_cd_read_datacnt = 3;
                    cd_read_buffer = pce_cd_dirinfo;
                    break;
                }		// switch command of request 0xde
            }			// end if of request 0xde (receiving command or executing them)
        }			// switch of request
    }				// end if of command arg or new request
    else {
        // it's an command ID we're receiving
        switch (cd_port_1801) {
          case 0x00:
            cd_port_1800 = 0xD8;
            break;
          case 0x08:
            pce_cd_curcmd = cd_port_1801;
            pce_cd_cmdcnt = 4;
            break;
          case 0xD8:
            pce_cd_curcmd = cd_port_1801;
            pce_cd_cmdcnt = 4;
            break;
          case 0xD9:
            pce_cd_curcmd = cd_port_1801;
            pce_cd_cmdcnt = 4;
            break;
          case 0xDA:
            pce_cd_curcmd = cd_port_1801;
            pce_cd_cmdcnt = 0;

            pCddaStopFunc();

            //if (CD_emulation == 1)
				//cd_PlayStop();    //osd_cd_stop_audio ();
            break;
          case 0xDE:
            /* Get CD directory info */
            /* First arg is command? */
            /* Second arg is track? */
            cd_port_1800 = 0xd0;
            pce_cd_cmdcnt = 2;
            pce_cd_read_datacnt = 3;	/* 4 bytes */
            pce_cd_curcmd = cd_port_1801;
            break;
        }
        
        /*
        if (cd_port_1801 == 0x00) {
            cd_port_1800 = 0xd8;
        } else if (cd_port_1801 == 0x08) {
            pce_cd_curcmd = cd_port_1801;
            pce_cd_cmdcnt = 4;
        } else if (cd_port_1801 == 0xd8) {
            pce_cd_cmdcnt = 4;
            pce_cd_curcmd = cd_port_1801;
        } else if (cd_port_1801 == 0xd9) {
            pce_cd_cmdcnt = 4;
            pce_cd_curcmd = cd_port_1801;
        } else if (cd_port_1801 == 0xde) {
            // Get CD directory info
            // First arg is command?
            // Second arg is track?
            cd_port_1800 = 0xd0;
            pce_cd_cmdcnt = 2;
            pce_cd_read_datacnt = 3; // 4 bytes
            pce_cd_curcmd = cd_port_1801;
        }
         */
    }
}



//=============================================================================
//
//
//
//=============================================================================
void pce_cd_set_sector_address(void)
{
    pce_cd_sectoraddy = pce_cd_sectoraddress[0] << 16;
    pce_cd_sectoraddy += pce_cd_sectoraddress[1] << 8;
    pce_cd_sectoraddy += pce_cd_sectoraddress[2];
}

//=============================================================================
//
//
//
//=============================================================================
void issue_ADPCM_dma (void)
{
    while (cd_sectorcnt--) {
        memcpy(cd.PCM + cd.adpcm_dmaptr, cd_read_buffer, pce_cd_read_datacnt);
        cd_read_buffer = NULL;
        cd.adpcm_dmaptr += (unsigned short) pce_cd_read_datacnt;
        pce_cd_read_datacnt = 0;
        pce_cd_read_sector ();
    }
    pce_cd_read_datacnt = 0;
    pce_cd_adpcm_trans_done = 1;
    cd_read_buffer = NULL;
}



