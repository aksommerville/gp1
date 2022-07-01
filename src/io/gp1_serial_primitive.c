#include "gp1_serial.h"
#include <limits.h>
#include <stdint.h>
#include <string.h>

/* Evaluate integer.
 */
 
int gp1_int_eval(int *dst,const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (srcc<1) return -1;
  int srcp=0,base=10,positive=1;
  
  if (src[srcp]=='-') {
    positive=0;
    if (++srcp>=srcc) return -1;
  } else if (src[srcp]=='+') {
    if (++srcp>=srcc) return -1;
  }
  
  if ((srcp<srcc-2)&&(src[srcp]=='0')) switch (src[srcp+1]) {
    case 'x': case 'X': base=16; srcp+=2; break;
    case 'd': case 'D': base=10; srcp+=2; break;
    case 'o': case 'O': base=8; srcp+=2; break;
    case 'b': case 'B': base=2; srcp+=2; break;
  }
  
  int limit,overflow=0;
  if (positive) limit=UINT_MAX/base;
  else limit=INT_MIN/base;
  
  *dst=0;
  while (srcp<srcc) {
    int digit=gp1_digit_eval(src[srcp++]);
    if ((digit<0)||(digit>=base)) return -1;
    if (positive) {
      if ((unsigned int)(*dst)>limit) overflow=1;
      (*dst)=((unsigned int)(*dst))*base;
      if ((unsigned int)(*dst)>UINT_MAX-digit) overflow=1;
      (*dst)=((unsigned int)(*dst))+digit;
    } else {
      if (*dst<limit) overflow=1;
      (*dst)*=base;
      if (*dst<INT_MIN+digit) overflow=1;
      (*dst)-=digit;
    }
  }
  if (overflow) return 0;
  if (positive&&(*dst<0)) return 1;
  return 2;
}

/* Represent signed decimal integer.
 */
 
int gp1_decsint_repr(char *dst,int dsta,int src) {
  if (!dst||(dsta<0)) dsta=0;
  int dstc;
  if (src<0) {
    dstc=2;
    int limit=-10;
    while (src<=limit) { dstc++; if (limit<INT_MIN/10) break; limit*=10; }
    if (dstc>dsta) return dstc;
    int i=dstc;
    for (;i-->0;src/=10) dst[i]='0'-src%10;
    dst[0]='-';
  } else {
    dstc=1;
    int limit=1;
    while (src>=limit) { dstc++; if (limit>INT_MAX/10) break; limit*=10; }
    if (dstc>dsta) return dstc;
    int i=dstc;
    for (;i-->0;src/=10) dst[i]='0'+src%10;
  }
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

/* Represent unsigned decimal integer with padding and truncation.
 */
 
int gp1_decuint_repr(char *dst,int dsta,int src,int digitc) {
  int dstc=1;
  if (digitc<0) { // explicit length, pad or truncate. no need to measure
    dstc=-digitc;
  } else {
    unsigned int limit=10;
    while ((unsigned int)src>=limit) { dstc++; if (limit>UINT_MAX/10) break; limit*=10; }
    if (dstc<digitc) dstc=digitc;
  }
  if (dstc>dsta) return dstc;
  int i=dstc;
  for (;i-->0;src=(unsigned int)src/10) dst[i]='0'+(unsigned int)src%10;
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

/* Represent unsigned hexadecimal integer with padding and truncation.
 */
 
int gp1_hexuint_repr(char *dst,int dsta,int src,int digitc,int prefix) {
  int dstc=1;
  if (digitc<0) {
    dstc=-digitc;
  } else {
    unsigned int limit=0x10;
    while ((unsigned int)src>=limit) { dstc++; if (limit&0xf0000000) break; limit<<=4; }
    if (dstc<digitc) dstc=digitc;
  }
  if (prefix) {
    dstc+=2;
    if (dstc>dsta) return dstc;
    int i=dstc;
    for (;i-->2;src=(unsigned int)src>>4) dst[i]=gp1_hexdigit_repr(src);
    dst[0]='0';
    dst[1]='x';
  } else {
    if (dstc>dsta) return dstc;
    int i=dstc;
    for (;i-->0;src=(unsigned int)src>>4) dst[i]=gp1_hexdigit_repr(src);
  }
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

/* Evaluate hexadecimal integer, digits only.
 */

int gp1_hexuint_eval(int *dst,const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  *dst=0;
  int shift=0;
  while (srcc-->0) {
    int digit=gp1_digit_eval(src[srcc]);
    if ((digit<0)||(digit>15)) return -1;
    (*dst)|=digit<<shift;
    shift+=4;
  }
  if (!shift) return -1;
  if (shift>32) return 0;
  if (*dst<0) return 1;
  return 2;
}

/* Measure number token.
 */

int gp1_number_measure(const char *src,int srcc,int *flags) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int _flags; if (!flags) flags=&_flags;
  *flags=0;
  int srcp=0;
  
  if (srcp>=srcc) return -1;
  if ((src[srcp]=='-')||(src[srcp]=='+')) {
    if (++srcp>=srcc) return -1;
  }
  
  if ((srcp<srcc-2)&&(src[srcp]=='0')) switch (src[srcp+1]) {
    case 'x': case 'X': srcp+=2; (*flags)|=GP1_NUMBER_INTEGER; break;
    case 'd': case 'D': srcp+=2; (*flags)|=GP1_NUMBER_INTEGER; break;
    case 'o': case 'O': srcp+=2; (*flags)|=GP1_NUMBER_INTEGER; break;
    case 'b': case 'B': srcp+=2; (*flags)|=GP1_NUMBER_INTEGER; break;
  }
  
  while (srcp<srcc) {
    if (gp1_digit_eval(src[srcp])<0) break;
    srcp++;
  }
  
  if ((srcp<srcc)&&(src[srcp]=='.')) {
    if (++srcp>=srcc) return -1;
    (*flags)|=GP1_NUMBER_FLOAT;
    while ((srcp<srcc)&&(src[srcp]>='0')&&(src[srcp]<='9')) srcp++;
  }
  if ((srcp<srcc)&&((src[srcp]=='e')||(src[srcp]=='E'))) {
    if (++srcp>=srcc) return -1;
    if ((src[srcp]=='+')||(src[srcp]=='-')) {
      if (++srcp>=srcc) return -1;
    }
    (*flags)|=GP1_NUMBER_FLOAT;
    while ((srcp<srcc)&&(src[srcp]>='0')&&(src[srcp]<='9')) srcp++;
  }
  
  if (srcp>=srcc) return srcp;
  if (gp1_digit_eval(src[srcp])>=0) return -1;
  return srcp;
}

/* Measure string token.
 */
 
int gp1_string_measure(const char *src,int srcc,int *flags) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (srcc<2) return -1;
  if ((src[0]!='"')&&(src[0]!='\'')&&(src[0]!='`')) return -1;
  if (flags) *flags=GP1_STRING_SIMPLE;
  int srcp=1;
  while (1) {
    if (srcp>=srcc) return -1;
    if (src[srcp]==src[0]) return srcp+1;
    if (src[srcp]=='\\') {
      if (flags) *flags=0;
      if (++srcp>=srcc) return -1;
    }
    // High bytes are allowed (eg UTF-8). But anything below 0x20 is an error.
    if ((unsigned char)src[srcp]<0x20) return -1;
    srcp++;
  }
}

/* Measure JSON token.
 * We allow trailing commas, though JSON strictly doesn't.
 */
 
int gp1_json_measure(const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (srcc<1) return -1;
  
  if (src[0]=='{') {
    int srcp=1,err;
    while (1) {
      if (srcp>=srcc) return -1;
      if (src[srcp]=='}') return srcp+1;
      if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
      if ((err=gp1_string_measure(src+srcp,srcc-srcp,0))<1) return -1;
      srcp+=err;
      while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
      if ((srcp>=srcc)||(src[srcp++]!=':')) return -1;
      while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
      if ((err=gp1_json_measure(src+srcp,srcc-srcp))<1) return -1;
      srcp+=err;
      while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
      if (srcp>=srcc) return -1;
      if (src[srcp]=='}') return srcp++;
      if (src[srcp++]!=',') return -1;
    }
    
  } else if (src[0]=='[') {
    int srcp=1,err;
    while (1) {
      if (srcp>=srcc) return -1;
      if (src[srcp]==']') return srcp+1;
      if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
      if ((err=gp1_json_measure(src+srcp,srcc-srcp))<1) return -1;
      srcp+=err;
      if ((unsigned char)src[srcp]<=0x20) { srcp++; continue; }
      if (srcp>=srcc) return -1;
      if (src[srcp]==']') return srcp+1;
      if (src[srcp++]!=',') return -1;
    }
    
  } else if (src[0]=='"') {
    return gp1_string_measure(src,srcc,0);
    
  } else if ((src[0]=='-')||(src[0]=='+')||((src[0]>='0')&&(src[0]<='9'))) {
    return gp1_number_measure(src,srcc,0);
    
  } else {
    // The only things left are the three identifiers: "null", "true", "false".
    // It should suffice just to count lower case letters.
    int srcp=0;
    while ((srcp<srcc)&&(src[srcp]>='a')&&(src[srcp]<='z')) srcp++;
    return srcp;
  }
}
      

/* Evaluate string.
 */

int gp1_string_eval(char *dst,int dsta,const char *src,int srcc) {
  if (!dst||(dsta<0)) dsta=0;
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if ((srcc<2)||(src[0]!=src[srcc-1])) return -1;
  if ((src[0]!='"')&&(src[0]!='\'')&&(src[0]!='`')) return -1;
  src++; srcc-=2;
  int dstc=0,srcp=0;
  while (srcp<srcc) {
    if (src[srcp]=='\\') {
      if (++srcp>=srcc) return -1;
      switch (src[srcp++]) {
      
        case '"': case '\'': case '`': case '\\': if (dstc<dsta) dst[dstc]=src[srcp-1]; dstc++; break;
      
        case '0': if (dstc<dsta) dst[dstc]=0x00; dstc++; break;
        case 'b': if (dstc<dsta) dst[dstc]=0x08; dstc++; break;
        case 't': if (dstc<dsta) dst[dstc]=0x09; dstc++; break;
        case 'n': if (dstc<dsta) dst[dstc]=0x0a; dstc++; break;
        case 'v': if (dstc<dsta) dst[dstc]=0x0b; dstc++; break;
        case 'f': if (dstc<dsta) dst[dstc]=0x0c; dstc++; break;
        case 'r': if (dstc<dsta) dst[dstc]=0x0d; dstc++; break;
        case 'e': if (dstc<dsta) dst[dstc]=0x1b; dstc++; break;
        
        case 'x': { // 'x' = single byte, allowed to break encoding
            if (srcp>srcc-2) return -1;
            int hi=gp1_digit_eval(src[srcp++]);
            int lo=gp1_digit_eval(src[srcp++]);
            if ((hi<0)||(hi>15)||(lo<0)||(lo>15)) return -1;
            if (dstc<dsta) dst[dstc]=(hi<<4)|lo;
            dstc++;
          } break;
          
        case 'u': { // 'u' = JSON-style Unicode, always 4 digits, above BMP must be manually surrogated, yuck.
            if (srcp>srcc-4) return -1;
            int i=4,ch=0;
            for (;i-->0;srcp++) {
              int digit=gp1_digit_eval(src[srcp++]);
              if ((digit<0)||(digit>15)) return -1;
              ch<<=4;
              ch|=digit;
            }
            if ((ch>=0xd800)&&(ch<0xdc00)&&(srcp<=srcc-6)&&(src[srcp]=='\\')&&(src[srcp+1]=='u')) {
              int lo=0,p=2;
              for (;p<6;p++) {
                int digit=gp1_digit_eval(src[srcp+p]);
                if ((digit<0)||(digit>15)) { lo=0; break; }
                lo<<=4;
                lo|=digit;
              }
              if ((lo>=0xdc00)&&(lo<0xe000)) {
                ch=0x10000+((ch&0x3ff)<<10)+(lo&0x3ff);
                srcp+=6;
              }
            }
            dstc+=gp1_utf8_encode(dst+dstc,dsta-dstc,ch);
          } break;
          
        case 'U': { // 'U' = my-style Unicode, 1..6 digits, optional ';' terminator
            int digitc=0,ch=0;
            while ((srcp<srcc)&&(digitc<6)) {
              int digit=gp1_digit_eval(src[srcp]);
              if ((digit<0)||(digit>15)) break;
              ch<<=4;
              ch|=digit;
              digitc++;
            }
            if (!digitc) return -1;
            if (ch>=0x110000) return -1;
            if ((srcp<srcc)&&(src[srcp]==';')) srcp++;
            dstc+=gp1_utf8_encode(dst+dstc,dsta-dstc,ch);
          } break;
        
        default: return -1;
      }
    } else {
      if (dstc<dsta) dst[dstc]=src[srcp];
      dstc++;
      srcp++;
    }
  }
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

/* Represent string.
 * Don't use '\x' or '\U', we want to stay JSON-legal.
 */
 
int gp1_string_repr(char *dst,int dsta,const char *src,int srcc) {
  if (!dst||(dsta<0)) dsta=0;
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int dstc=0,srcp=0;
  while (srcp<srcc) {
    
    if ((src[srcp]=='"')||(src[srcp]=='\\')) {
      if (dstc<=dsta-2) {
        dst[dstc++]='\\';
        dst[dstc++]=src[srcp];
      } else dstc+=2;
      srcp++;
      continue;
    }
    
    if ((src[srcp]>=0x20)&&(src[srcp]<=0x7e)) {
      if (dstc<dsta) dst[dstc]=src[srcp];
      dstc++;
      srcp++;
      continue;
    }
    
    int ch,seqlen;
    if ((seqlen=gp1_utf8_decode(&ch,src+srcp,srcc-srcp))<1) {
      ch=(unsigned char)src[srcp];
      seqlen=1;
    }
    if (ch>=0x10000) {
      if (dstc<=dsta-12) {
        ch-=0x10000;
        int hi=0xd800|(ch>>10);
        int lo=0xdc00|(ch&0x3ff);
        dst[dstc++]='\\';
        dst[dstc++]='u';
        dst[dstc++]=gp1_hexdigit_repr(hi>>12);
        dst[dstc++]=gp1_hexdigit_repr(hi>>8);
        dst[dstc++]=gp1_hexdigit_repr(hi>>4);
        dst[dstc++]=gp1_hexdigit_repr(hi);
        dst[dstc++]='\\';
        dst[dstc++]='u';
        dst[dstc++]=gp1_hexdigit_repr(lo>>12);
        dst[dstc++]=gp1_hexdigit_repr(lo>>8);
        dst[dstc++]=gp1_hexdigit_repr(lo>>4);
        dst[dstc++]=gp1_hexdigit_repr(lo);
      } else dstc+=12;
    } else {
      if (dstc<=dsta-6) {
        dst[dstc++]='\\';
        dst[dstc++]='u';
        dst[dstc++]=gp1_hexdigit_repr(ch>>12);
        dst[dstc++]=gp1_hexdigit_repr(ch>>8);
        dst[dstc++]=gp1_hexdigit_repr(ch>>4);
        dst[dstc++]=gp1_hexdigit_repr(ch);
      } else dstc+=6;
    }
    srcp+=seqlen;
  }
  if (dstc<dsta) dst[dstc]=0;
  return dstc;
}

/* UTF-8.
 */

int gp1_utf8_encode(void *dst,int dsta,int src) {
  uint8_t *DST=dst;
  if (src<0) return -1;
  if (src<0x80) {
    if (dsta>=1) {
      DST[0]=src;
    }
    return 1;
  }
  if (src<0x800) {
    if (dsta>=2) {
      DST[0]=0xc0|(src>>6);
      DST[1]=0x80|(src&0x3f);
    }
    return 2;
  }
  if (src<0x10000) {
    if (dsta>=3) {
      DST[0]=0xe0|(src>>12);
      DST[1]=0x80|((src>>6)&0x3f);
      DST[2]=0x80|(src&0x3f);
    }
    return 3;
  }
  if (src<0x110000) {
    if (dsta>=4) {
      DST[0]=0xf0|(src>>18);
      DST[1]=0x80|((src>>12)&0x3f);
      DST[2]=0x80|((src>>6)&0x3f);
      DST[3]=0x80|(src&0x3f);
    }
    return 4;
  }
  return -1;
}

int gp1_utf8_decode(int *dst,const void *src,int srcc) {
  if (!src||(srcc<1)) return -1;
  const uint8_t *SRC=src;
  if (!(SRC[0]&0x80)) {
    *dst=SRC[0];
    return 1;
  }
  if (!(SRC[0]&0x40)) return -1;
  if (!(SRC[0]&0x20)) {
    if (srcc<2) return -1;
    if ((SRC[1]&0xc0)!=0x80) return -1;
    *dst=((SRC[0]&0x1f)<<6)|(SRC[1]&0x3f);
    return 2;
  }
  if (!(SRC[0]&0x10)) {
    if (srcc<3) return -1;
    if ((SRC[1]&0xc0)!=0x80) return -1;
    if ((SRC[2]&0xc0)!=0x80) return -1;
    *dst=((SRC[0]&0x0f)<<12)|((SRC[1]&0x3f)<<6)|(SRC[2]&0x3f);
    return 3;
  }
  if (!(SRC[0]&0x08)) {
    if (srcc<4) return -1;
    if ((SRC[1]&0xc0)!=0x80) return -1;
    if ((SRC[2]&0xc0)!=0x80) return -1;
    if ((SRC[3]&0xc0)!=0x80) return -1;
    *dst=((SRC[0]&0x07)<<18)|((SRC[1]&0x3f)<<12)|((SRC[2]&0x3f)<<6)|(SRC[3]&0x3f);
    return 4;
  }
  return -1;
}

/* JSON primitive as int.
 */
 
int gp1_json_as_int(int *dst,const char *src,int srcc) {
  if (!src) return -1;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  if (srcc<1) return -1;
  
  if (gp1_int_eval(dst,src,srcc)>=1) return 0; // >=1 means allow sign-bit overflow
  
  // null, true, or false? 0 or 1
  if ((srcc==4)&&!memcmp(src,"null",4)) { *dst=0; return 0; }
  if ((srcc==4)&&!memcmp(src,"true",4)) { *dst=1; return 0; }
  if ((srcc==5)&&!memcmp(src,"false",5)) { *dst=0; return 0; }
  
  // If it's a string, evaluate it, then retry evaluating as int.
  if (src[0]=='"') {
    char tmp[32];
    int tmpc=gp1_string_eval(tmp,sizeof(tmp),src,srcc);
    if ((tmpc>0)&&(tmpc<=sizeof(tmp))) {
      if (gp1_int_eval(dst,tmp,tmpc)>=1) return 0;
    }
    // The empty string, we'll call that zero.
    if (!tmpc) { *dst=0; return 0; }
  }
  
  //TODO floats -- truncate
  
  // The only other legal things are array and object. Those will not be readable as int.
  return -1;
}

/* Count newlines.
 */
 
int gp1_count_newlines(const char *src,int srcc) {
  if (!src) return 0;
  if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  int nlc=0;
  for (;srcc-->0;src++) if (*src==0x0a) nlc++;
  return nlc;
}

/* Text or binary?
 */
 
int gp1_is_text(const char *src,int srcc) {
  // UTF-8 is extremely unlikely to contain U+0..U+8.
  // Binary data in general is likely to contain them, esp zero.
  for (;srcc-->0;src++) {
    if ((*src>=0)&&(*src<=0x08)) return 0;
  }
  return 1;
}

/* Match text pattern.
 */
 
static int gp1_pattern_match_1(const char *pat,int patc,const char *src,int srcc) {
  int patp=0,srcp=0,score=1;
  while (1) {
  
    // Check completion of (pat).
    if (patp>=patc) {
      if (srcp<srcc) return 0;
      return score;
    }
    
    // Check for wildcard. (src) may be complete but still match.
    if (pat[patp]=='*') {
      patp++;
      while ((patp<patc)&&(pat[patp]=='*')) patp++; // adjacent stars redundant
      if (patp>=patc) return score; // trailing star matches all
      while (srcp<srcc) {
        int sub=gp1_pattern_match_1(pat+patp,patc-patp,src+srcp,srcc-srcp);
        if (sub>0) return score+sub;
        srcp++;
      }
      return 0; // portion after star didn't match
    }
    
    // With wildcards out of the way, completion of (src) is a mismatch.
    if (srcp>=srcc) return 0;
    
    // Literals.
    if (pat[patp]=='\\') {
      if (++patp>=patc) return 0; // malformed pattern
      if (pat[patp]!=src[srcp]) return 0;
      patp++;
      srcp++;
      score++;
      continue;
    }
    
    // Numeric wildcard.
    if (pat[patp]=='#') {
      if ((src[srcp]<'0')||(src[srcp]>'9')) return 0;
      while ((srcp<srcc)&&(src[srcp]>='0')&&(src[srcp]<='9')) srcp++;
      patp++;
      score++; // the whole construction is worth just one point
      continue;
    }
    
    // Collapse inner whitespace.
    if ((unsigned char)pat[patp]<=0x20) {
      if ((unsigned char)src[srcp]>0x20) return 0;
      while ((patp<patc)&&((unsigned char)pat[patp]<=0x20)) patp++;
      while ((srcp<srcc)&&((unsigned char)src[srcp]<=0x20)) srcp++;
      score++; // one point for the whole construction
      continue;
    }
    
    // Case-insensitive letters, or verbatim all else.
    int cha=pat[patp++]; if ((cha>='A')&&(cha<='Z')) cha+=0x20;
    int chb=src[srcp++]; if ((chb>='A')&&(chb<='Z')) chb+=0x20;
    if (cha!=chb) return 0;
    score++;
  }
}
 
int gp1_pattern_match(const char *pat,int patc,const char *src,int srcc) {
  if (!pat) patc=0; else if (patc<0) { patc=0; while (pat[patc]) patc++; }
  if (!src) srcc=0; else if (srcc<0) { srcc=0; while (src[srcc]) srcc++; }
  while (patc&&((unsigned char)pat[patc-1]<=0x20)) patc--;
  while (patc&&((unsigned char)pat[0]<=0x20)) { patc--; pat++; }
  while (srcc&&((unsigned char)src[srcc-1]<=0x20)) srcc--;
  while (srcc&&((unsigned char)src[0]<=0x20)) { srcc--; src++; }
  return gp1_pattern_match_1(pat,patc,src,srcc);
}
