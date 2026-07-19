/* egis0576_tls.c — TLS-PSK secure channel to the EH576 (gusb + OpenSSL).
 * Ported from the standalone egis_tls.c; USB via g_usb_device_*_transfer. */
#include <string.h>
#include <openssl/hmac.h>
#include <openssl/sha.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include "egis0576_tls.h"
#include "egis_tls_init.h"

#define EP_OUT 0x01
#define EP_IN  0x82
#define IMG EGIS_TLS_IMG

static const unsigned char PSK[] = {
  0xa2,0xe6,0xd6,0x72,0x5a,0x65,0x06,0x71,0x13,0xfc,0x9b,0xe3,0xb4,0x7a,0x0f,0xf3,
  0x1a,0x9f,0x08,0x0b,0x0e,0xd9,0x7d,0x8f,0xb4,0xf8,0xf7,0x0c,0x32,0xb8 };
#define PSK_LEN (sizeof PSK)

struct EgisTls {
  GUsbDevice *usb;
  unsigned char cmac[32], smac[32], ckey[16], skey[16];
  unsigned char master[48];
  unsigned long long sseq, cseq;
  unsigned char rbuf[65536];
  int rlen;
  unsigned char gain0f;     /* cached reg 0x0f (fine gain), read once at open  */
  unsigned char gain12;     /* cached reg 0x12 (coarse gain idx), read at open */
};

/* forward decls: auto-exposure caches the gain registers at OPEN and never reads
 * them during capture (register reads interleaved with getframe drift the TLS
 * sequence counter, which the record-MAC check then rejects). */
static int  egis_readreg (EgisTls *t, int reg);
static void egis_writereg (EgisTls *t, int reg, int val);

/* reg 0x12 (coarse gain) indexes this per-step table (byte-exact from the
 * Windows driver .rdata @0x180077170). */
static const int EGIS_STEP_TABLE[16] = {
  2048, 14684, 17121, 19558, 20623, 24371, 26829, 29286,
  31744, 34202, 36454, 38912, 41370, 43827, 46285, 48742
};

/* ---- crypto (identical to egis_tls.c) ---- */
static void hmac256 (const unsigned char *k, int kl, const unsigned char *d, int dl, unsigned char *o) {
  unsigned int ol = 32; HMAC (EVP_sha256 (), k, kl, d, dl, o, &ol);
}
static void prf (const unsigned char *secret, int sl, const char *label,
                 const unsigned char *seed, int seedl, unsigned char *out, int n) {
  int ll = strlen (label);
  unsigned char ls[256]; memcpy (ls, label, ll); memcpy (ls + ll, seed, seedl); int lsl = ll + seedl;
  unsigned char a[32]; hmac256 (secret, sl, ls, lsl, a);
  int done = 0; unsigned char tmp[32 + 256];
  while (done < n) {
    memcpy (tmp, a, 32); memcpy (tmp + 32, ls, lsl);
    unsigned char blk[32]; hmac256 (secret, sl, tmp, 32 + lsl, blk);
    int take = (n - done < 32) ? n - done : 32; memcpy (out + done, blk, take); done += take;
    hmac256 (secret, sl, a, 32, a);
  }
}
static void derive_keys (EgisTls *t, const unsigned char *cr, const unsigned char *sr) {
  unsigned char z[64] = {0}; int zl = PSK_LEN;
  unsigned char pms[4 + 128]; int p = 0;
  pms[p++] = (zl >> 8) & 0xff; pms[p++] = zl & 0xff; memcpy (pms + p, z, zl); p += zl;
  pms[p++] = (PSK_LEN >> 8) & 0xff; pms[p++] = PSK_LEN & 0xff; memcpy (pms + p, PSK, PSK_LEN); p += PSK_LEN;
  unsigned char seed[64]; memcpy (seed, cr, 32); memcpy (seed + 32, sr, 32);
  prf (pms, p, "master secret", seed, 64, t->master, 48);
  unsigned char seed2[64]; memcpy (seed2, sr, 32); memcpy (seed2 + 32, cr, 32);
  unsigned char kb[96]; prf (t->master, 48, "key expansion", seed2, 64, kb, 96);
  memcpy (t->cmac, kb, 32); memcpy (t->smac, kb + 32, 32); memcpy (t->ckey, kb + 64, 16); memcpy (t->skey, kb + 80, 16);
}
static int aes_cbc (int enc, const unsigned char *key, const unsigned char *iv,
                    const unsigned char *in, int inl, unsigned char *out) {
  EVP_CIPHER_CTX *c = EVP_CIPHER_CTX_new (); int ol = 0, tl = 0;
  EVP_CipherInit_ex (c, EVP_aes_128_cbc (), NULL, key, iv, enc);
  EVP_CIPHER_CTX_set_padding (c, 0);
  EVP_CipherUpdate (c, out, &ol, in, inl); EVP_CipherFinal_ex (c, out + ol, &tl);
  EVP_CIPHER_CTX_free (c); return ol + tl;
}
static void rec_mac (const unsigned char *mk, unsigned long long seq, int ct,
                     const unsigned char *content, int cl, unsigned char *mac) {
  unsigned char h[13 + 4096]; int p = 0;
  for (int i = 7; i >= 0; i--) h[p++] = (seq >> (8 * i)) & 0xff;
  h[p++] = ct; h[p++] = 0x03; h[p++] = 0x03; h[p++] = (cl >> 8) & 0xff; h[p++] = cl & 0xff;
  memcpy (h + p, content, cl); p += cl; hmac256 (mk, 32, h, p, mac);
}
static int mac_then_encrypt (const unsigned char *mk, const unsigned char *ek, unsigned long long seq, int ct,
                             const unsigned char *content, int cl, unsigned char *out) {
  unsigned char buf[4096 + 64]; memcpy (buf, content, cl);
  unsigned char mac[32]; rec_mac (mk, seq, ct, content, cl, mac); memcpy (buf + cl, mac, 32);
  int dl = cl + 32; int pad = 15 - (dl % 16); for (int i = 0; i <= pad; i++) buf[dl + i] = pad; dl += pad + 1;
  unsigned char iv[16]; RAND_bytes (iv, 16); memcpy (out, iv, 16);
  return 16 + aes_cbc (1, ek, iv, buf, dl, out + 16);
}
/* Decrypt a TLS 1.2 CBC record (explicit IV) and VERIFY the record HMAC + padding,
 * exactly as mbedTLS ssl_decrypt_buf does (MAC-then-encrypt: plaintext =
 * content || HMAC-SHA256(seq||type||03 03||len||content) || padding). Returns the
 * content length, or -1 on any structural/padding/MAC failure. The padding and MAC
 * comparisons accumulate into one branchless diff to avoid a padding/MAC oracle. */
static int decrypt_verify (const unsigned char *mk, const unsigned char *ek, unsigned long long seq, int ct,
                           const unsigned char *rec, int rl, unsigned char *content) {
  unsigned char pt[4096], mac[32];
  int ptl, pad, cl, i, diff = 0;

  if (rl < 16 || ((rl - 16) & 0x0f) != 0)     /* need explicit IV + block-aligned CT */
    return -1;
  ptl = aes_cbc (0, ek, rec, rec + 16, rl - 16, pt);
  if (ptl < 33 || (ptl & 0x0f) != 0)          /* >= 1 pad byte + 32-byte MAC, block-aligned */
    return -1;
  pad = pt[ptl - 1];
  if (pad + 1 > ptl)
    return -1;
  for (i = 0; i <= pad; i++)                   /* every one of (pad+1) trailing bytes must equal pad */
    diff |= pt[ptl - 1 - i] ^ pad;
  cl = ptl - (pad + 1) - 32;
  if (cl < 0)
    return -1;
  rec_mac (mk, seq, ct, pt, cl, mac);          /* recompute HMAC over the content */
  for (i = 0; i < 32; i++)
    diff |= mac[i] ^ pt[cl + i];
  if (diff != 0)                               /* padding or MAC mismatch -> reject */
    return -1;
  memcpy (content, pt, cl);
  return cl;
}

/* ---- USB via gusb (synchronous) ---- */
static gboolean usb_write (EgisTls *t, const unsigned char *d, int n) {
  gsize act = 0;
  return g_usb_device_bulk_transfer (t->usb, EP_OUT, (guint8 *) d, n, &act, 3000, NULL, NULL);
}
static int usb_fill (EgisTls *t, int timeout) {
  guint8 tmp[4096]; gsize act = 0;
  if (!g_usb_device_bulk_transfer (t->usb, EP_IN, tmp, sizeof tmp, &act, timeout, NULL, NULL)) return -1;
  if (act > 0 && t->rlen + (int) act <= (int) sizeof t->rbuf) { memcpy (t->rbuf + t->rlen, tmp, act); t->rlen += act; }
  return (int) act;
}
static int read_record (EgisTls *t, unsigned char *rec, int *rt, int timeout) {
  while (t->rlen < 5) if (usb_fill (t, timeout) < 0) return -1;
  int rl = (t->rbuf[3] << 8) | t->rbuf[4];
  while (t->rlen < 5 + rl) if (usb_fill (t, timeout) < 0) return -1;
  *rt = t->rbuf[0]; memcpy (rec, t->rbuf + 5, rl);
  memmove (t->rbuf, t->rbuf + 5 + rl, t->rlen - (5 + rl)); t->rlen -= 5 + rl; return rl;
}
static void write_record (EgisTls *t, int rt, const unsigned char *body, int bl) {
  unsigned char p[4096]; p[0] = rt; p[1] = 0x03; p[2] = 0x03; p[3] = (bl >> 8) & 0xff; p[4] = bl & 0xff;
  memcpy (p + 5, body, bl); usb_write (t, p, 5 + bl);
}
static void send_app (EgisTls *t, const unsigned char *d, int n) {
  unsigned char enc[4096]; int el = mac_then_encrypt (t->smac, t->skey, t->sseq++, 23, d, n, enc);
  write_record (t, 23, enc, el);
}
static int recv_app (EgisTls *t, unsigned char *out, int timeout) {
  unsigned char rec[4096]; int rt; int rl = read_record (t, rec, &rt, timeout);
  if (rl < 0) return -1;
  return decrypt_verify (t->cmac, t->ckey, t->cseq++, rt, rec, rl, out);
}
/* Read and discard a command's response record(s). Waits up to `timeout` ms for
 * the FIRST record (so a slow response is never cut off, which would leave stale
 * bytes to corrupt the following frame read), then flushes any back-to-back
 * followers with a short timeout instead of paying the full timeout as dead wait
 * after every command. This is the dominant per-frame latency saver: getframe
 * issues 6 commands, so the old "full 80ms tail per command" cost ~0.5s/frame. */
static void drain (EgisTls *t, int timeout) {
  unsigned char o[4096];
  if (recv_app (t, o, timeout) < 0) return;   /* no response within the window */
  while (recv_app (t, o, 12) >= 0) {}          /* flush immediate followers only */
}

/* ---- handshake + init ---- */
gboolean egis_tls_open (EgisTls *t, GError **error) {
  g_usb_device_control_transfer (t->usb, G_USB_DEVICE_DIRECTION_HOST_TO_DEVICE,
                                 G_USB_DEVICE_REQUEST_TYPE_CLASS, G_USB_DEVICE_RECIPIENT_INTERFACE,
                                 9, 0, 0, NULL, 0, NULL, 2000, NULL, NULL);
  unsigned char transcript[8192]; int tl = 0;
  unsigned char rec[4096]; int rt;
  int rl = read_record (t, rec, &rt, 3000);
  if (rl < 0 || rt != 22 || rec[0] != 1) {
    g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "EH576: no TLS ClientHello (rt=%d)", rt);
    return FALSE;
  }
  memcpy (transcript + tl, rec, rl); tl += rl;
  unsigned char cr[32]; memcpy (cr, rec + 6, 32);
  unsigned char sr[32], sid[32]; RAND_bytes (sr, 32); RAND_bytes (sid, 32);
  unsigned char body[128]; int b = 0;
  body[b++] = 0x03; body[b++] = 0x03; memcpy (body + b, sr, 32); b += 32;
  body[b++] = 32; memcpy (body + b, sid, 32); b += 32;
  body[b++] = 0x00; body[b++] = 0xae; body[b++] = 0x00;
  unsigned char ext[] = {0x00, 0x05, 0xff, 0x01, 0x00, 0x01, 0x00}; memcpy (body + b, ext, 7); b += 7;
  unsigned char sh[140]; sh[0] = 0x02; sh[1] = 0; sh[2] = (b >> 8) & 0xff; sh[3] = b & 0xff; memcpy (sh + 4, body, b);
  unsigned char shd[] = {0x0e, 0x00, 0x00, 0x00};
  memcpy (transcript + tl, sh, 4 + b); tl += 4 + b; memcpy (transcript + tl, shd, 4); tl += 4;
  write_record (t, 22, sh, 4 + b); write_record (t, 22, shd, 4);
  gboolean have_keys = FALSE;
  for (int guard = 0; guard < 8; guard++) {
    rl = read_record (t, rec, &rt, 3000);
    if (rl < 0) break;
    if (rt == 22 && rec[0] == 16) { memcpy (transcript + tl, rec, rl); tl += rl; derive_keys (t, cr, sr); have_keys = TRUE; }
    else if (rt == 20) { /* CCS */ }
    else if (rt == 22) {
      unsigned char content[64]; int cl = decrypt_verify (t->cmac, t->ckey, t->cseq++, 22, rec, rl, content);
      if (cl > 0) { memcpy (transcript + tl, content, cl); tl += cl; }
      break;
    }
  }
  if (!have_keys) { g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "EH576: TLS key exchange failed"); return FALSE; }
  unsigned char one = 0x01; write_record (t, 20, &one, 1);
  unsigned char th[32]; SHA256 (transcript, tl, th);
  unsigned char sfin[12]; prf (t->master, 48, "server finished", th, 32, sfin, 12);
  unsigned char fin[16] = {0x14, 0x00, 0x00, 0x0c}; memcpy (fin + 4, sfin, 12);
  unsigned char enc[128]; int el = mac_then_encrypt (t->smac, t->skey, t->sseq++, 22, fin, 16, enc);
  write_record (t, 22, enc, el);

  /* replay the exact Windows init + calibration */
  for (int i = 0; i < EGIS_INIT_RECORD_COUNT; i++) {
    send_app (t, egis_init_records[i].data, egis_init_records[i].len);
    drain (t, 80);
  }
  t->gain0f = 0xff;   /* auto-exposure disabled (see egis_tls_autoexpose note) */
  return TRUE;
}

static const char *REP[] = { "632c020057", "602d00", "626703", "600f00", "632c020013" };
static void hx (const char *h, unsigned char *o, int *n) { *n = strlen (h) / 2; for (int i = 0; i < *n; i++) sscanf (h + 2 * i, "%2hhx", o + i); }
static void egis_cmd (EgisTls *t, const char *tail) {
  unsigned char c[16] = {0x45, 0x47, 0x49, 0x53}; int n; hx (tail, c + 4, &n); send_app (t, c, 4 + n); drain (t, 80);
}

/* ReadRegister(reg): "EGIS" 60 reg 00 -> "SIGE" reg <value> <status>; value@[5] */
static int egis_readreg (EgisTls *t, int reg) {
  unsigned char c[8] = {0x45, 0x47, 0x49, 0x53, 0x60, (unsigned char) reg, 0x00};
  unsigned char o[4096];
  int n;
  send_app (t, c, 7);
  n = recv_app (t, o, 400);
  drain (t, 50);
  return (n >= 6 && o[0] == 'S') ? o[5] : -1;
}
static void egis_writereg (EgisTls *t, int reg, int val) {
  unsigned char c[8] = {0x45, 0x47, 0x49, 0x53, 0x61, (unsigned char) reg, (unsigned char) val};
  send_app (t, c, 7);
  drain (t, 50);
}

/* One auto-exposure step (et5xx_fetch_dynamic_intensity, FUN_180065f70): nudge
 * the sensor gain (reg 0x0f fine / 0x12 coarse) from the frame min/max so every
 * frame spans 0..0xff. This adapts the exposure PER DEVICE — the baked init gain
 * is one unit's value; on a different EH576 the frame may be over/under-exposed
 * and this corrects it. Returns 1 if it changed the gain, 0 if the frame was
 * already well-exposed (no change). A well-exposed sensor (mn>0 && mx<0xff) hits
 * the `else` no-op, so on the author's unit this never fires. */
static int auto_expose_mm (EgisTls *t, int mn, int mx) {
  int step, v;

  if (mn > 0 && mx < 0xff)            /* already well-exposed: no read, no write */
    return 0;
  if (mn == 0 && mx == 0xff)          /* full span: back off coarse gain */
    {
      if (t->gain12 > 1)
        {
          egis_writereg (t, 0x12, t->gain12 - 1);
          t->gain12--;
          return 1;
        }
      return 0;
    }
  step = EGIS_STEP_TABLE[t->gain12 & 0x0f];   /* cached coarse gain, no read */
  v = t->gain0f;                              /* cached fine gain, no read   */
  if (mx == 0xff)                    /* too bright: lower fine gain toward 0 */
    {
      int target = mn << 10;
      while (v > 0 && target >= step) { target -= step; v--; }
    }
  else /* mn == 0 */                 /* too dark: raise fine gain, cap 0x40 */
    {
      int target = (0xff - mx) << 10;
      while (v < 0x40 && target >= step) { v++; target -= step; }
    }
  if ((unsigned char) v != t->gain0f)
    {
      egis_writereg (t, 0x0f, v);
      t->gain0f = (unsigned char) v;
      return 1;
    }
  return 0;
}

/* Public per-device auto-exposure step. The driver passes the current frame's
 * min/max; returns TRUE iff the sensor gain was changed (so the caller can
 * re-capture its flat-field baseline at the new gain). */
gboolean egis_tls_autoexpose (EgisTls *t, int frame_min, int frame_max) {
  return auto_expose_mm (t, frame_min, frame_max) ? TRUE : FALSE;
}

/* NOTE: auto_expose_mm() (above) is the RE'd Windows auto-exposure; it is
 * currently NOT wired into getframe because reading it back every frame slowed
 * capture too much AND it did not resolve the cross-session match. Kept for a
 * future, cheaper integration (e.g. run it only every N frames / off the poll). */
gboolean egis_tls_getframe (EgisTls *t, guint8 *img, GError **error) {
  int got = 0;
  unsigned char o[4096];

  for (int i = 0; i < 5; i++)
    egis_cmd (t, REP[i]);
  egis_cmd (t, "600000");
  {
    unsigned char req[] = {0x45, 0x47, 0x49, 0x53, 0x64, 0x0f, 0x96};
    send_app (t, req, 7);
  }
  while (got < IMG) {
    int n = recv_app (t, o, 800);
    if (n < 0) break;
    int take = (IMG - got < n) ? IMG - got : n; memcpy (img + got, o, take); got += take;
  }
  if (got < IMG) { g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED, "EH576: short frame (%d)", got); return FALSE; }
  return TRUE;
}

/* Per-device gain calibration — a faithful port of the Windows calibrate_gain /
 * FUN_180066470 path. A 6-iteration BINARY SEARCH over the fine-gain register
 * (0x0f, range 0..0x3f): each step writes a candidate gain, captures a NO-FINGER
 * frame, and measures its mean, converging to the gain whose mean is closest to
 * the target 0x40 (=64). This makes the sensor's exposure the SAME on any EH576
 * unit (the baked init gain is one unit's). It runs ONCE per boot at open, BEFORE
 * any capture, so the gain register writes never interleave with a getframe (which
 * would drift the TLS sequence counter). The gain persists in the sensor across
 * opens (init does not touch 0x0f), so later opens skip it. Because it can change
 * the gain, templates must be (re-)enrolled after it is in place — templates are
 * gain-specific, exactly like the per-boot flat-field. Assumes no finger at open. */
#define EGIS_GAIN_TARGET 0x58   /* target no-finger mean; Windows uses 0x40 + per-finger AE, but we have
                                 * no per-finger AE, so we anchor to the reference unit's well-matching
                                 * fixed-gain no-finger level (~0x58=88) — a no-op on that unit, adaptive on others */
static gboolean g_gain_calibrated = FALSE;

gboolean egis_tls_calibrate_gain (EgisTls *t, GError **error) {
  unsigned char img[IMG];
  /* Full fine-gain range [0,0x3f], no unit-specific cap: the binary search finds
   * each unit's own operating point. Saturating gains (mean 0xff) are handled
   * naturally — a saturated frame reads as "too bright" and the search backs off,
   * and capturing one is harmless (the read_record reassembly is exact, so the TLS
   * sequence never drifts). best_gain tracks the closest-to-target mean seen, so a
   * unit whose saturation begins early still converges to a sane gain. */
  int lo = 0, hi = 0x3f, best_gain = 0x1f, best_diff = 0x100, best_level = 0;

  if (g_gain_calibrated)
    return TRUE;                         /* once per boot; gain persists in the sensor */
  for (int it = 0; it < 6; it++)
    {
      int mid = (lo + hi) >> 1, level, diff;
      long sum = 0;
      egis_writereg (t, 0x0f, mid);
      t->gain0f = (unsigned char) mid;
      if (!egis_tls_getframe (t, img, error))
        return FALSE;                    /* keep the baked gain on failure */
      for (int i = 0; i < IMG; i++) sum += img[i];
      level = (int) (sum / IMG);         /* mean of the no-finger frame */
      diff = level > EGIS_GAIN_TARGET ? level - EGIS_GAIN_TARGET : EGIS_GAIN_TARGET - level;
      if (diff < best_diff) { best_diff = diff; best_gain = mid; best_level = level; }
      if (level > EGIS_GAIN_TARGET) hi = mid;   /* too bright: lower the gain */
      else                          lo = mid;   /* too dark:   raise the gain */
    }
  egis_writereg (t, 0x0f, best_gain);
  t->gain0f = (unsigned char) best_gain;
  g_gain_calibrated = TRUE;
  return TRUE;
}

EgisTls *egis_tls_new (GUsbDevice *usb) { EgisTls *t = g_new0 (EgisTls, 1); t->usb = usb; return t; }
void egis_tls_free (EgisTls *t) { g_free (t); }
