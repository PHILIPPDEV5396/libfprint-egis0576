# The capture worker and gusb — and the lost wakeup of 2026-09-13

**TL;DR.** Every capture runs on a worker thread (`egis0576-capture`); only
the one-time bring-up in `open()` runs on the fprintd main thread. Since
v0.4.3 the transport talks to gusb through its **async** API on a **private
`GMainContext`** that the calling thread iterates itself. It used to call
gusb's *synchronous* wrappers, which quietly depend on whoever owns the
default main context — the fprintd main thread — and that produced
a hang where a verify never answered and every later claim was refused with
`"Device was already claimed"` until fprintd was restarted. The rule that falls
out of it: **never call the gusb sync API from a thread that does not own the
`GUsbContext`'s main context.**

---

## How the worker talks to the sensor

`driver/egis0576.c` starts one `GThread` per capture (`start_capture`) and joins
it before the action completes. Everything a capture does that touches USB —
the re-init replay when the sensor needs it, and `GetFrame` — happens on that
thread, in `driver/egis0576/egis0576_proto.c`, and results go
back to the main thread with `g_idle_add`. The one exception is the `open()`
vfunc: libfprint dispatches it inline on the main thread, and this driver runs
the bring-up there synchronously — readiness poll, `ForceReset` control
request, vendor replay (~0.3 s on a healthy sensor) and the exposure
calibration — without spawning a thread. So the main thread does block on USB
once per device open; it never does while a finger is on the sensor.

Each transfer is *blocking from the caller's point of view*: `usb_out` and
`usb_in` do not return until the transfer completes or its timeout fires
(3000 ms for a bulk OUT, 300 or 800 ms for a bulk IN plus a 30 ms drain read
in `flush_in`, 500 ms for the control request). Cancellation is a flag /
`GCancellable` that the worker **polls
between transfers**; it is never handed to gusb, because aborting an in-flight
URB was measured to wedge the sensor at USB level (see the comment above
`usb_out` in `egis0576_proto.c`). That rule is unchanged.

What changed in v0.4.3 is *how* a transfer blocks. Before, the transport
called `g_usb_device_bulk_transfer()` and
`g_usb_device_control_transfer()`, gusb's synchronous helpers. Now it calls
`g_usb_device_bulk_transfer_async()` / `g_usb_device_control_transfer_async()`
and waits for the completion callback on a `GMainLoop` that runs on a fresh
`GMainContext` created for that one call and pushed as the thread-default
context for its duration (`egis_bulk` / `egis_control`).

## The incident

Environment: fprintd 1.94.5, gusb 0.4.9, glib 2.88.3, driver v0.4.2, on the
reference Yoga 7 14ARB7. A verify was cancelled by the client going away; the
verify never completed; from then on every `Claim` was refused with
`"Device was already claimed"` until systemd eventually had to `SIGABRT`
fprintd.

The refusal text is the same one the upstream stale-claim-across-suspend bug
produces (see [`suspend-resume.md`](suspend-resume.md)); the difference is that
here a verify was in flight and never returned, whereas in the suspend case no
driver call happens at all.

Attaching gdb to the stuck fprintd showed the worker thread parked inside
`g_main_loop_run`, in `g_main_context_wait_internal`, called from
`g_usb_device_bulk_transfer` — the sync wrapper — for the 7-byte bulk OUT
`EGIS 62 67 03`, one of the per-frame preamble commands. The wrapper's helper
struct already held `ret = 7`: the transfer *had completed* and its finish
function *had run*. libusb had no transfer in flight. The loop's `is_running`
flag was set. The default `GMainContext`'s owner was the main thread, which
held it three levels deep: fprintd was sitting in
`_fprint_device_client_vanished` → `g_main_context_iteration(NULL, TRUE)`,
waiting for the cancelled verify to finish — i.e. waiting for the worker,
which was waiting for the main thread.

## Why the sync wrapper can lose the wakeup

gusb's sync helper (gusb-device.c, `g_usb_device_bulk_transfer`) does this:

1. `helper.loop = g_main_loop_new (g_usb_context_get_main_context (ctx), FALSE)`
   — that context is the **default** `GMainContext`, the one fprintd's main
   thread owns for the lifetime of its `g_main_loop_run`.
2. `g_usb_device_bulk_transfer_async (..., sync_cb, &helper)` — creates a
   `GTask` with `g_task_new`, which captures the *calling thread's* thread-
   default context. On the worker nothing has been pushed, so that is again
   the default context. Then `libusb_submit_transfer`.
3. `g_main_loop_run (helper.loop)`.

When the transfer completes, gusb's libusb event thread calls
`g_task_return_int`, which queues the completion as an idle on the task's
context — the default context — so the completion, and with it
`g_main_loop_quit (helper.loop)`, is **dispatched by the main thread**, not by
the worker. If that quit runs *after* step 2 has submitted the transfer but
*before* step 3 has entered `g_main_loop_run`, its effect is lost.
`g_main_loop_quit` does not check whether the loop is running: it clears
`is_running` (already clear), signals the context's wakeup and broadcasts the
context's condition variable — but nobody is waiting on that condition yet, so
the broadcast has no taker. `g_main_loop_run` then fails
`g_main_context_acquire` (the main thread owns the context), only *then* sets
`is_running = TRUE` — overwriting the quit — and waits on that condition
variable in `g_main_context_wait_internal` for a broadcast that has already
happened (glib 2.88.3 `gmain.c`, `g_main_loop_run` / `g_main_loop_quit`).
The window is a few instructions wide, and a preemption between
steps 2 and 3 widens it; a fast sensor answering a 7-byte OUT makes it
reachable in practice.

So the old transport had two hidden dependencies on the main thread: it needed
the main thread to be iterating the default context at all (or nothing would
ever complete), and it needed that dispatch to not race its own
`g_main_loop_run`. The first is why fprintd's client-vanished handler, which
*does* iterate, kept things moving most of the time; the second is what
finally bit.

## Why the private context fixes it

`egis_bulk` creates a `GMainContext` of its own, pushes it as the thread-
default, and only then calls `g_usb_device_bulk_transfer_async`. Verified in
gusb 0.4.9's `gusb-device.c`: both `_bulk_transfer_async` and
`_control_transfer_async` create their `GTask` with `g_task_new` after entry
(for a real device, directly before `libusb_submit_transfer`) and neither
pushes or pops a thread-default context of its own. `g_task_new` therefore
captures *our* context. When the transfer completes, gusb's internal libusb
event thread (`GUsbEventThread`) calls `g_task_return_int`; that thread is not
inside a source dispatch, so `GTask` queues the completion as an idle on the
task's context — ours. The only thread that ever iterates it is the caller,
inside `g_main_loop_run`, after `is_running` has been set. No other thread can
quit the loop; a completion cannot be dispatched before the loop runs; and the
worker no longer needs the main thread to make progress at all. The context is
popped and dropped when the call returns.

The same helper serves the main-thread bring-up in `open()`, and it is safe
there for the same reason: the fresh context is unowned, so `g_main_loop_run`
acquires it, and the completion lands on it. One behavioural difference from
the sync wrapper is worth knowing: before, the wrapper nested a loop on the
*default* context, so fprintd's own sources (D-Bus and the rest) were
dispatched re-entrantly from inside `open()`; now they are simply not
dispatched for the duration of the bring-up, which is what a blocking `open()`
is expected to look like.

`usb_out` / `usb_in` keep their exact semantics — `usb_out` is TRUE iff the
finish function returned something other than -1 (what the sync wrapper's
`helper.ret != -1` used to report), `usb_in` returns the byte count or -1 —
and the timeouts are unchanged. The cancellable passed to gusb is still always
NULL.

One consequence for the `close()` backstop in `egis0576.c`: joining the worker
from the main thread while a transfer is in flight used to be a guaranteed
deadlock (the completion needed the very thread that was blocked in the join).
Now it waits until the worker reaches its next cancellation point — the end of
the current getframe sequence in the capture loop (~2.6 s worst case on a
silent sensor), or, if the worker is inside `worker_reinit`, the end of the
vendor replay, which is deliberately uncancellable as a unit (up to ~25
records × 800 ms if the sensor has stopped answering). Bounded, not
deadlocked. The path is still meant to be unreachable and still warns.

## The rule

**Never call the gusb sync API (`g_usb_device_bulk_transfer`,
`g_usb_device_control_transfer`, `g_usb_device_interrupt_transfer`) from a
thread that does not own the `GUsbContext`'s main context.** In a libfprint
driver that context is the default one and the daemon's main thread owns it;
any other thread that calls the sync API is borrowing the main thread's loop
without saying so, and will hang the moment the main thread blocks on the
caller — or, as here, the moment the main thread's dispatch races the caller's
`g_main_loop_run`. Drive the async API on a context you iterate yourself
instead, as `egis_bulk` does.
