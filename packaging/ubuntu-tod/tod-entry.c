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
 */
#include <glib-object.h>
#include <gmodule.h>

GType fpi_device_egis0576_get_type (void);

G_MODULE_EXPORT GType
fpi_tod_shared_driver_get_type (void)
{
  return fpi_device_egis0576_get_type ();
}
