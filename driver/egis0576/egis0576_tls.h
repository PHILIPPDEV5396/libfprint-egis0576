/* egis0576_tls.h — TLS-PSK secure channel to the EH576 for the libfprint driver.
 * The sensor is the TLS client, the host (us) the TLS server; after the handshake
 * the EGIS/SIGE command protocol (incl. GetFrame image capture) runs encrypted.
 * Uses gusb (synchronous bulk transfers) + OpenSSL libcrypto. */
#ifndef EGIS0576_TLS_H
#define EGIS0576_TLS_H

#include <glib.h>
#include <gusb.h>

#define EGIS_TLS_IMG 3990

typedef struct EgisTls EgisTls;

/* Create a TLS session bound to an already-claimed GUsbDevice. */
EgisTls *egis_tls_new (GUsbDevice *usb);
void     egis_tls_free (EgisTls *t);

/* ctrl 0x21/9 TLS-mode trigger + full TLS-PSK handshake + replay the Windows
 * init/calibration. Blocks (~1s). Returns TRUE on success. */
gboolean egis_tls_open (EgisTls *t, GError **error);

/* Capture one 3990-byte frame over the encrypted channel (sends the streaming
 * triggers + GetFrame, reassembles the decrypted app-data records). Blocks
 * ~0.1s. img must hold EGIS_TLS_IMG bytes. */
gboolean egis_tls_getframe (EgisTls *t, guint8 *img, GError **error);

/* Per-device gain calibration (Windows calibrate_gain / FUN_180066470): one-time
 * binary search over the fine-gain register at open so the no-finger exposure is
 * the same on any EH576 unit. Runs before capture (no getframe interleaving) and
 * once per boot (gain persists in the sensor). Templates must be re-enrolled after
 * it changes the gain. Returns FALSE (keeping the baked gain) on capture error. */
gboolean egis_tls_calibrate_gain (EgisTls *t, GError **error);

#endif
