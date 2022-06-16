#include "gp1_cli.h"
#include "vm/gp1_rom.h"
#include "io/gp1_io_os.h"
#include "gp1/gp1.h"

/* Parse ROM file manually to report things the ROM object decoder discards.
 */
 
static void gp1_info_from_serial(struct gp1_config *config,const uint8_t *src,int srcc,const char *path) {
  fprintf(stdout,"%s: %d bytes total.\n",path,srcc);
  int srcp=0,ok=1;
  while (srcp<srcc) {
    if (srcp>srcc-8) {
      ok=0;
      fprintf(stdout,"!!! Unexpected EOF reading chunk header at %d/%d\n",srcp,srcc);
      break;
    }
    int srcp0=srcp;
    uint8_t sanitized[4];
    const uint8_t *chunktype=src+srcp;
    int len=(src[srcp+4]<<24)|(src[srcp+5]<<16)|(src[srcp+6]<<8)|src[srcp+7];
    srcp+=8;
    if (
      (chunktype[0]<0x21)||(chunktype[0]>0x7e)||
      (chunktype[0]<0x21)||(chunktype[0]>0x7e)||
      (chunktype[0]<0x21)||(chunktype[0]>0x7e)||
      (chunktype[0]<0x21)||(chunktype[0]>0x7e)||
    0) {
      ok=0;
      fprintf(stdout,
        "!!! Illegal chunk type 0x%02x%02x%02x%02x at %d/%d\n",
        chunktype[0],chunktype[1],chunktype[2],chunktype[3],srcp0,srcc
      );
      memset(sanitized,'?',4);
      chunktype=sanitized;
    }
    if ((len<0)||(srcp>srcc-len)) {
      ok=0;
      fprintf(stdout,
        "!!! Illegal chunk length %d for '%.4s' at %d/%d\n",
        len,chunktype,srcp0,srcc
      );
      break;
    }
    
    if (srcp0) {
      if (!memcmp(chunktype,"gpHD",4)) {
        ok=0;
        fprintf(stdout,"!!! 'gpHD' at %d/%d, must be the first chunk\n",srcp0,srcc);
      }
    } else {
      if (memcmp(chunktype,"gpHD",4)) {
        ok=0;
        fprintf(stdout,"!!! First chunk is '%.4s', must be 'gpHD'\n",chunktype);
      }
    }
    
    if (!memcmp(chunktype,"lANG",4)) {
      if (len&1) {
        ok=0;
        fprintf(stdout,"!!! Illegal length %d for 'lANG' at %d/%d, must be multiple of 2\n",len,srcp0,srcc);
      } else {
        fprintf(stdout,"Declared languages:");
        char sep=' ';
        int i=0; for (;i<len;i+=2) {
          char lang[2]={src[srcp+i],src[srcp+i+1]};
          if ((lang[0]<'a')||(lang[0]>'z')) lang[0]='?';
          if ((lang[1]<'a')||(lang[1]>'z')) lang[1]='?';
          fprintf(stdout,"%c%.2s",sep,lang);
          sep=',';
        }
        fprintf(stdout,"\n");
      }
    }
    
    srcp+=len;
  }
  if (ok) {
    fprintf(stdout,"Chunk structure is valid.\n");
  }
}

/* Dump digested info from ROM object.
 */
 
static void gp1_info_from_object(struct gp1_config *config,struct gp1_rom *rom,const char *path) {
  fprintf(stdout,"%s: Decoded OK.\n",path);
  
  #define KFMT "%20s: "
  int i,total;
  const struct gp1_rurl *rurl;
  const struct gp1_stor *stor;
  const struct gp1_imag *imag;
  const struct gp1_song *song;
  const struct gp1_data *data;
  
  /* gpHD */
  fprintf(stdout,KFMT"%d.%d.%d\n","Version",rom->game_version>>24,(rom->game_version>>16)&0xff,rom->game_version&0xffff);
  fprintf(stdout,KFMT"%d.%d.%d\n","GP1 Minimum",rom->gp1_version_min>>24,(rom->gp1_version_min>>16)&0xff,rom->gp1_version_min&0xffff);
  fprintf(stdout,KFMT"%d.%d.%d\n","GP1 Target",rom->gp1_version_target>>24,(rom->gp1_version_target>>16)&0xff,rom->gp1_version_target&0xffff);
  fprintf(stdout,KFMT"%04d-%02d-%02d\n","Publication Date",rom->pub_date>>16,(rom->pub_date>>8)&0xff,rom->pub_date&0xff);
  fprintf(stdout,KFMT"%dx%d\n","Framebuffer Size",rom->fbw,rom->fbh);
  fprintf(stdout,KFMT"%d..%d\n","Player Count",rom->playerc_min,rom->playerc_max);
  
  /* hTXT */
  fprintf(stdout,KFMT"%s\n","Title",rom->ht_title?rom->ht_title:"");
  fprintf(stdout,KFMT"%s\n","Author",rom->ht_author?rom->ht_author:"");
  fprintf(stdout,KFMT"%s\n","Copyright",rom->ht_copyrite?rom->ht_copyrite:"");
  fprintf(stdout,KFMT"%s\n","Website",rom->ht_website?rom->ht_website:"");
  fprintf(stdout,KFMT"%s\n","License",rom->ht_license?rom->ht_license:"");
  fprintf(stdout,KFMT"%s\n","Variant",rom->ht_variant?rom->ht_variant:"");
  fprintf(stdout,KFMT"%s\n","Description",rom->ht_desc?rom->ht_desc:"");
  
  /* Resource counts. */
  fprintf(stdout,KFMT"%d\n","String Count",rom->stringc);
  fprintf(stdout,KFMT"%d\n","Image Count",rom->imagc);
  fprintf(stdout,KFMT"%d\n","Song Count",rom->songc);
  fprintf(stdout,KFMT"%d\n","Data Count",rom->datac);
  
  /* Bulk data usage. */
  fprintf(stdout,KFMT"%d\n","Executable Size",rom->execc);
  fprintf(stdout,KFMT"%d\n","String Text Size",rom->textc);
  fprintf(stdout,KFMT"%d\n","Audio Config Size",rom->audoc);
  for (imag=rom->imagv,i=rom->imagc,total=0;i-->0;imag++) total+=imag->c;
  fprintf(stdout,KFMT"%d\n","Images Total Size",total);
  for (song=rom->songv,i=rom->songc,total=0;i-->0;song++) total+=song->c;
  fprintf(stdout,KFMT"%d\n","Songs Total Size",total);
  for (data=rom->datav,i=rom->datac,total=0;i-->0;data++) total+=data->c;
  fprintf(stdout,KFMT"%d\n","Data Total Size",total);
  
  /* rURL */
  for (rurl=rom->rurlv,i=rom->rurlc;i-->0;rurl++) {
    fprintf(stdout,KFMT"%.*s:%d: %.*s\n","Remote",rurl->hostc,rurl->host,rurl->port,rurl->rationalec,rurl->rationale);
  }
  
  /* sTOR */
  for (stor=rom->storv,i=rom->storc;i-->0;stor++) {
    fprintf(stdout,KFMT"id=%d (%d..%d): %.*s\n","Storage",stor->key,stor->min,stor->max,stor->rationalec,stor->rationale);
  }
  
  /* imAG */
  for (imag=rom->imagv,i=rom->imagc;i-->0;imag++) {
    fprintf(stdout,KFMT"id=%d %dx%d fmt=%d size=%d\n","Image",imag->imageid,imag->w,imag->h,imag->format,imag->c);
  }
  
  // We could dump string content, but I doubt that that's valuable.
  
  #undef KFMT
}

/* Print ROM info, main entry point.
 */
 
void gp1_print_rom_info(struct gp1_config *config,const void *src,int srcc,const char *path) {
  
  gp1_info_from_serial(config,src,srcc,path);

  uint16_t langv[16];
  int langc=gp1_get_user_languages(langv,16);
  struct gp1_rom *rom=gp1_rom_new(src,srcc,langv,langc,path);
  if (!rom) return; // gp1_rom_new() logs the error
  gp1_info_from_object(config,rom,path);
  gp1_rom_del(rom);
}
