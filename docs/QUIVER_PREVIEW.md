# Nocked skill-arrow preview

This follow-up to #118 enables the selected arrow skill's native model and
particles on the bowstring (`[bow] quiver_preview=1`, now the default).
An explicit saved value of `0` retains the basic nocked model. Casting, native
mana costs, physical flight and hit effects continue to use #118's implementation.

## Crash investigation

The contributor reported `BC_ASSERT: m_registryIsMutable` immediately after
selecting a skill with the preview enabled, followed by unit/instance assertions
on unloading. That crash has not yet been reproduced with this follow-up.

Code inspection found that `PresentPreview`, a queued UI callback, directly
called the native client-missile destructor when swapping/recovering a preview.
The renderer may be using its HD entity registry in that phase. Additionally,
the destruction guard only accepted missile 0, so it detached skill previews
without deleting them. These are distinct lifecycle problems; attributing the
reported assertion to the callback phase remains a hypothesis until headset
testing.

The UI now requests retirement using the native pointer, unit ID and actual
missile type. The existing native `ClientTick` hook performs destruction, as it
already does for finished physical arrows. Replacement creation waits until
destruction returns. Native `ClientKill` notifications also complete pending
retirement, while late callbacks and recycled addresses cannot delete another
unit. Unwanted client creations use the same deferred cleanup. No registry
assertion or engine protection is disabled.

Pending previews stay owned and hidden until cleanup. Rapid selection changes
coalesce into the latest selection. The next preview is created only with a
current visible bow pose, so opening a menu cannot spawn a stale replacement.
Binding records the actual missile type during construction, including early
HD callbacks. Normal native teardown continues to own old-session objects.

## Verification

`quiver-test` covers basic and all seven supported missile types, duplicate
requests, replacement blocking through destruction, stale completion after
session reset, reused pointers/IDs/types, and multiple pending objects. These
fixtures do not run the injected game routines or prove registry-phase safety.

Headset checklist (offline Amazon):

- Load, draw, fire, unload and switch Magic/Fire/Cold Arrow repeatedly.
- Switch while drawn; check selected appearance, arrow placement and effects.
- Check the other supported arrow skills and confirm mana is spent only on firing.
- Walk/turn with a nocked arrow, open/close menus and switch weapons.
- Change areas, save/exit and re-enter; check for stale arrows or missing previews.

`Frame bow preview:` logs identify queued retirement, native-tick completion and
newly created missile IDs. Collect the active mod's `d2r-vr-frame.log` and native
loader/crash logs if an assertion remains. The regression is not considered
headset-verified until the owner reports the result.
