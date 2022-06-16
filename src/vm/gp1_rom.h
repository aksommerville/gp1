/* gp1_rom.h
 * ROM file, expanded in memory.
 */
 
#ifndef GP1_ROM_H
#define GP1_ROM_H

#include <stdint.h>

struct gp1_rom {
  int refc;
  
  // gpHD
  uint32_t game_version;
  uint32_t gp1_version_min;
  uint32_t gp1_version_target;
  uint32_t pub_date;
  uint16_t fbw,fbh;
  uint8_t playerc_min,playerc_max;
  
  // exEC
  void *exec;
  int execc;
  
  // hTXT
  char *ht_title;
  char *ht_author;
  char *ht_copyrite;
  char *ht_website;
  char *ht_license;
  char *ht_variant;
  char *ht_desc;
  
  // teXT
  struct gp1_string {
    uint16_t stringid;
    uint16_t c;
    const char *v; // WEAK, points into (text)
  } *stringv;
  int stringc,stringa;
  char *text;
  int textc,texta;
  
  // rURL,sTOR
  struct gp1_rurl {
    char *host;
    int hostc;
    int port;
    char *rationale;
    int rationalec;
  } *rurlv;
  int rurlc,rurla;
  struct gp1_stor {
    uint32_t key;
    uint32_t min;
    uint32_t max;
    char *rationale;
    int rationalec;
  } *storv;
  int storc,stora;
  
  // imAG
  struct gp1_imag {
    uint32_t imageid;
    uint16_t w,h;
    uint8_t format;
    void *v; // Filtered and compressed.
    int c;
  } *imagv;
  int imagc,imaga;
  
  // auDO
  void *audo;
  int audoc;
  
  // soNG
  struct gp1_song {
    uint32_t songid;
    void *v; // MIDI file
    int c;
  } *songv;
  int songc,songa;
  
  // daTA
  struct gp1_data {
    uint32_t dataid;
    void *v;
    int c;
  } *datav;
  int datac,dataa;
};

void gp1_rom_del(struct gp1_rom *rom);
int gp1_rom_ref(struct gp1_rom *rom);

/* Decode a ROM file into a new object.
 * Give us the user's preferred languages in order, empty if you don't know.
 * We will commit to a language during decode, and other languages will not be retrievable after.
 * If (refname) not null, we log errors to stderr.
 */
struct gp1_rom *gp1_rom_new(
  const void *src,int srcc,
  const uint16_t *langv,int langc,
  const char *refname
);

/* Returns a weak reference to our internal storage, never terminated.
 */
int gp1_rom_string_get(void *dstpp,const struct gp1_rom *rom,uint16_t stringid);
int gp1_rom_data_get(void *dstpp,const struct gp1_rom *rom,uint32_t dataid);

/* Decode an image resource and return the raw pixels.
 * On success, (*dstpp) will contain a fresh buffer which the caller must eventually free.
 * This can be very expensive. Try to cache to output somewhere.
 */
int gp1_rom_image_decode(
  void *dstpp,
  uint16_t *w,uint16_t *h,uint8_t *fmt,
  const struct gp1_rom *rom,
  uint32_t imageid
);

int gp1_rom_imag_search(const struct gp1_rom *rom,uint32_t imageid);
struct gp1_imag *gp1_rom_imag_insert(struct gp1_rom *rom,int p,uint32_t imageid,const void *src,int srcc);

int gp1_rom_song_search(const struct gp1_rom *rom,uint32_t songid);
struct gp1_song *gp1_rom_song_insert(struct gp1_rom *rom,int p,uint32_t songid,const void *src,int srcc);

int gp1_rom_data_search(const struct gp1_rom *rom,uint32_t dataid);
struct gp1_data *gp1_rom_data_insert(struct gp1_rom *rom,int p,uint32_t dataid,const void *src,int srcc);

#endif
