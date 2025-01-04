#include <string.h>

#include "../crypto.hh"
#include "../string.hh"
#include "../bytes.hh"

using namespace ssc::runtime::string;
using namespace ssc::runtime::bytes;

#define SHA1_ROTL(x,n) (((x) << (n)) | ((x) >> (32-(n))))
#define SHA1_CH(x,y,z) (((x) & (y)) ^ ((~(x)) & (z)))
#define SHA1_PARITY(x,y,z) ((x) ^ (y) ^ (z))
#define SHA1_MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))

namespace ssc::runtime::crypto {
  static void sha1_transform (
    uint32_t state[5],
    const unsigned char buffer[64]
  ) {
    uint32_t a, b, c, d, e;
    uint32_t w[80];
    size_t t;

    for (t = 0; t < 16; t++) {
      w[t] = (uint32_t)buffer[t * 4] << 24;
      w[t] |= (uint32_t)buffer[t * 4 + 1] << 16;
      w[t] |= (uint32_t)buffer[t * 4 + 2] << 8;
      w[t] |= (uint32_t)buffer[t * 4 + 3];
    }

    for (t = 16; t < 80; t++) {
      w[t] = SHA1_ROTL((w[t-3] ^ w[t-8] ^ w[t-14] ^ w[t-16]), 1);
    }

    a = state[0];
    b = state[1];
    c = state[2];
    d = state[3];
    e = state[4];

    // round 1
    for (t = 0; t < 20; t++) {
      uint32_t temp = SHA1_ROTL(a,5) + SHA1_CH(b,c,d) + e + w[t] + 0x5A827999;
      e = d; d = c; c = SHA1_ROTL(b,30); b = a; a = temp;
    }

    // round 2
    for (; t < 40; t++) {
      uint32_t temp = SHA1_ROTL(a,5) + SHA1_PARITY(b,c,d) + e + w[t] + 0x6ED9EBA1;
      e = d; d = c; c = SHA1_ROTL(b,30); b = a; a = temp;
    }

    // round 3
    for (; t < 60; t++) {
      uint32_t temp = SHA1_ROTL(a,5) + SHA1_MAJ(b,c,d) + e + w[t] + 0x8F1BBCDC;
      e = d; d = c; c = SHA1_ROTL(b,30); b = a; a = temp;
    }

    // round 4
    for (; t < 80; t++) {
      uint32_t temp = SHA1_ROTL(a,5) + SHA1_PARITY(b,c,d) + e + w[t] + 0xCA62C1D6;
      e = d; d = c; c = SHA1_ROTL(b,30); b = a; a = temp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
  }

  void sha1_init (SHA1Context *ctx) {
    ctx->bitCount = 0;
    ctx->state[0] = 0x67452301UL;
    ctx->state[1] = 0xEFCDAB89UL;
    ctx->state[2] = 0x98BADCFEUL;
    ctx->state[3] = 0x10325476UL;
    ctx->state[4] = 0xC3D2E1F0UL;
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
  }

  void sha1_update(SHA1Context *ctx, const unsigned char *data, size_t size) {
    size_t i = 0;
    size_t index = (size_t)((ctx->bitCount >> 3) & 0x3F);
    ctx->bitCount += (uint64_t)size << 3;

    size_t space = 64 - index;

    if (size >= space) {
      memcpy(&ctx->buffer[index], data, space);
      sha1_transform(ctx->state, ctx->buffer);
      for (i = space; i + 63 < size; i += 64) {
        sha1_transform(ctx->state, &data[i]);
      }
      index = 0;
    } else {
      i = 0;
    }
    memcpy(&ctx->buffer[index], &data[i], size - i);
  }

  void sha1_final(SHA1Context *ctx, unsigned char out[SHA1_DIGEST_SIZE]) {
    static const unsigned char pad[64] = { 0x80 };
    unsigned char bits[8];
    size_t index = (size_t)((ctx->bitCount >> 3) & 0x3F);
    size_t padSize;
    uint64_t bitCount_be = (ctx->bitCount);

    /* convert bit count to big-endian */
    bits[0] = (unsigned char)((bitCount_be >> 56) & 0xFF);
    bits[1] = (unsigned char)((bitCount_be >> 48) & 0xFF);
    bits[2] = (unsigned char)((bitCount_be >> 40) & 0xFF);
    bits[3] = (unsigned char)((bitCount_be >> 32) & 0xFF);
    bits[4] = (unsigned char)((bitCount_be >> 24) & 0xFF);
    bits[5] = (unsigned char)((bitCount_be >> 16) & 0xFF);
    bits[6] = (unsigned char)((bitCount_be >> 8) & 0xFF);
    bits[7] = (unsigned char)(bitCount_be & 0xFF);

    padSize = (index < 56) ? (56 - index) : (120 - index);
    sha1_update(ctx, pad, padSize);
    sha1_update(ctx, bits, 8);

    for (int i = 0; i < 5; i++) {
      out[i*4]     = (unsigned char)((ctx->state[i] >> 24) & 0xFF);
      out[i*4 + 1] = (unsigned char)((ctx->state[i] >> 16) & 0xFF);
      out[i*4 + 2] = (unsigned char)((ctx->state[i] >> 8) & 0xFF);
      out[i*4 + 3] = (unsigned char)(ctx->state[i] & 0xFF);
    }

    /* wipe context */
    memset(ctx, 0, sizeof(*ctx));
  }

  void sha1 (
    const unsigned char *data,
    size_t size,
    unsigned char out[SHA1_DIGEST_SIZE]
  ) {
    memset(out, 0, SHA1_DIGEST_SIZE);
    SHA1Context ctx;
    sha1_init(&ctx);
    sha1_update(&ctx, data, size);
    sha1_final(&ctx, out);
  }

  SHA1::SHA1 () {
    sha1_init(&this->context);
  }

  SHA1::SHA1 (const String& data) : SHA1() {
    this->update(data);
  }

  SHA1::SHA1 (
    const SharedPointer<unsigned char[]>& data,
    size_t size
  ) : SHA1() {
    this->update(
      reinterpret_cast<const unsigned char*>(data.get()),
      size
    );
  }

  SHA1::SHA1 (
    const unsigned char* data,
    size_t size
  ) : SHA1() {
    this->update(data, size);
  }

  SHA1& SHA1::update (
    const unsigned char* data,
    size_t size
  ) {
    sha1_update(&this->context, data, size);
    return *this;
  }

  SHA1& SHA1::update (const String& string) {
    return this->update(
      reinterpret_cast<const unsigned char*>(string.data()),
      string.size()
    );
  }

  bool SHA1::finalized () const {
    return this->isFinalized;
  }

  Vector<uint8_t> SHA1::finalize () {
    if (this->isFinalized == false) {
      sha1_final(&this->context, this->output);
      this->isFinalized = true;
    }

    return Vector<uint8_t>(
      this->output,
      this->output + SHA1_DIGEST_SIZE
    );
  }

  String SHA1::str () {
    return toUpperCase(encodeHexString(this->finalize()));
  }

  const String sha1 (const String& input) {
    return SHA1(input).str();
  }

  const String sha1 (const SharedPointer<unsigned char[]>& input, size_t size) {
    return SHA1(input, size).str();
  }

  const String sha1 (const unsigned char * input, size_t size) {
    return SHA1(input, size).str();
  }

}
