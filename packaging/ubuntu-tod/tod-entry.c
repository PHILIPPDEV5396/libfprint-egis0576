/* TOD module entry point for the egis0576 driver.
 *
 * Ubuntu's libfprint (the "TOD" fork) loads a fingerprint driver as a
 * separate lib*.so from a fixed directory instead of linking it into
 * libfprint-2.so.2 itself. The loader (libfprint/tod/tod-shared-loader.c
 * in Ubuntu's libfprint source) requires exactly one exported symbol,
 * fpi_tod_shared_driver_get_type, returning the GType of an FpDevice
 * subclass. G_DEFINE_TYPE in driver/egis0576.c already emits a
 * non-static fpi_device_egis0576_get_type, so this file only forwards
 * to it. No other driver source changes for the TOD build.
 *
 * It also supplies one symbol the TOD ABI does not carry, see below.
 */
#include <glib-object.h>
#include <gmodule.h>

GType fpi_device_egis0576_get_type (void);

/* libfprint's internal fpi_device_emulation_mode_enabled() tells a driver
 * that it is running against a recorded USB session under the umockdev test
 * harness; the egis0576 driver asks it whether to drop the pacing delay
 * between frames, so that a replay is not slowed to the speed of the
 * session that produced it. libfprint-2-tod-1 on Ubuntu 24.04 does not
 * export it, and the module then fails to link.
 *
 * A TOD module is never what those tests run against -- they load the
 * in-tree driver -- so the honest answer here is "not emulating", and
 * providing it in the module keeps driver/egis0576.c byte-identical to the
 * file submitted to libfprint. The parentheses around the name are
 * deliberate: fpi-device.h defines a macro of the same name, and this is
 * how that header itself writes the definition.
 *
 * If a future TOD release exports its own, the module's definition simply
 * shadows it, with the same answer.
 */
typedef struct _FpDevice FpDevice;

gboolean
(fpi_device_emulation_mode_enabled) (FpDevice *device)
{
  (void) device;
  return FALSE;
}

G_MODULE_EXPORT GType
fpi_tod_shared_driver_get_type (void)
{
  return fpi_device_egis0576_get_type ();
}
