/* gp1_serial.h
 * Helpers for encoding and decoding simple data.
 */
 
#ifndef GP1_SERIAL_H
#define GP1_SERIAL_H

struct gp1_encoder;
struct gp1_decoder;

/* Primitives.
 ***************************************************************/

static inline char gp1_hexdigit_repr(int src) {
  return "0123456789abcdef"[src&15];
}

static inline int gp1_digit_eval(char src) {
  if ((src>='0')&&(src<='9')) return src-'0';
  if ((src>='a')&&(src<='z')) return src-'a'+10;
  if ((src>='A')&&(src<='Z')) return src-'A'+10;
  return -1;
}
 
// (digitc>0) to pad only; (digitc<0) to pad or truncate.
int gp1_int_eval(int *dst,const char *src,int srcc);
int gp1_decsint_repr(char *dst,int dsta,int src);
int gp1_decuint_repr(char *dst,int dsta,int src,int digitc);
int gp1_hexuint_repr(char *dst,int dsta,int src,int digitc,int prefix);

#define GP1_NUMBER_INTEGER      0x0001
#define GP1_NUMBER_FLOAT        0x0002
#define GP1_STRING_SIMPLE       0x0004
int gp1_number_measure(const char *src,int srcc,int *flags);
int gp1_string_measure(const char *src,int srcc,int *flags);
int gp1_json_measure(const char *src,int srcc);

int gp1_string_eval(char *dst,int dsta,const char *src,int srcc);
int gp1_string_repr(char *dst,int dsta,const char *src,int srcc);

int gp1_utf8_encode(void *dst,int dsta,int src);
int gp1_utf8_decode(int *dst,const void *src,int srcc);

int gp1_json_as_int(int *dst,const char *src,int srcc);

int gp1_count_newlines(const char *src,int srcc);
int gp1_is_text(const char *src,int srcc); // NB imperfect

/* Decoder.
 *****************************************************************/
 
struct gp1_decoder {
  const void *src;
  int srcc;
  int srcp;
  int jsonctx;
};

static inline int gp1_decoder_remaining(const struct gp1_decoder *decoder) {
  return decoder->srcc-decoder->srcp;
}

int gp1_decode_int32be(int *dst,struct gp1_decoder *decoder);

int gp1_decode_raw(void *dstpp,struct gp1_decoder *decoder,int c);
int gp1_decode_int32belen(void *dstpp,struct gp1_decoder *decoder);

int gp1_decode_line(void *dstpp,struct gp1_decoder *decoder);

// Skip one JSON expression and return its content verbatim (points into src).
int gp1_decode_json_raw(void *dstpp,struct gp1_decoder *decoder);

/* Decode JSON array or object in one call, with a callback per member.
 * Your callback must consume the next expression. Hint: gp1_decode_json_raw(0,decoder) to skip it.
 * Object keys have the quotes stripped if we can do so trivially.
 * If a key is empty or contains any escapes, (k,kc) is the original token with quotes.
 */
int gp1_decode_json_array(
  struct gp1_decoder *decoder,
  int (*cb)(struct gp1_decoder *decoder,int p,void *userdata),
  void *userdata
);
int gp1_decode_json_object(
  struct gp1_decoder *decoder,
  int (*cb)(struct gp1_decoder *decoder,const char *k,int kc,void *userdata),
  void *userdata
);

/* Decode JSON array or object sequentially.
 * Prefer the callback-driven functions above, they work out easier.
 * 'start' returns a token (jsonctx) which you must return at 'end'.
 */
int gp1_decode_json_array_start(struct gp1_decoder *decoder);
int gp1_decode_json_object_start(struct gp1_decoder *decoder);
int gp1_decode_json_array_end(struct gp1_decoder *decoder,int jsonctx);
int gp1_decode_json_object_end(struct gp1_decoder *decoder,int jsonctx);
int gp1_decode_json_next(void *kpp,struct gp1_decoder *decoder);

/* Decode JSON primitives.
 * We make a reasonable attempt to convert the input to the value you request.
 * Everything can convert to string (even structures), that's usually equivalent to "raw".
 * null becomes the empty string (not the string "null").
 * gp1_decode_json_string() advances the decoder only if the output length <= (dsta).
 * gp1_decode_json_string_to_encoder() returns the amount appended on success.
 * TODO floats, 64-bit integers
 */
int gp1_decode_json_int(int *dst,struct gp1_decoder *decoder);
int gp1_decode_json_string(char *dst,int dsta,struct gp1_decoder *decoder);
int gp1_decode_json_string_to_encoder(struct gp1_encoder *dst,struct gp1_decoder *decoder);

/* What kind of object is next:
 *  [ array
 *  { object
 *  + number
 *  " string
 *  n null
 *  t true
 *  f false
 *  ! error
 * Doesn't consume that object, but may advance state across whitespace.
 */
char gp1_decode_json_peek(struct gp1_decoder *decoder);

/* Encoder.
 ***************************************************************/
 
struct gp1_encoder {
  char *v;
  int c,a;
  int jsonctx;
};

// Frees (ctx->v) if set. You may yoink it first.
void gp1_encoder_cleanup(struct gp1_encoder *encoder);

/* (src) for replace may be null to insert zeroes.
 */
int gp1_encoder_require(struct gp1_encoder *encoder,int addc);
int gp1_encoder_replace(struct gp1_encoder *encoder,int p,int c,const void *src,int srcc);

int gp1_encode_int8(struct gp1_encoder *encoder,int src);
int gp1_encode_int16be(struct gp1_encoder *encoder,int src);
int gp1_encode_int32be(struct gp1_encoder *encoder,int src);

int gp1_encode_raw(struct gp1_encoder *encoder,const void *src,int srcc);
int gp1_encode_int32belen(struct gp1_encoder *encoder,int lenp); // insert length to end at (lenp)

//TODO encode json

#endif
