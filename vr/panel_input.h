#pragma once
namespace panelinput {
void Install(); // timer thread; retries while native code is still encrypted
void OwnPointer(); // before native menu pointer injection; no game-thread calls
bool BlocksWorld(); // includes queued release after the menu closes
bool ControllerReady(); // UI thread: request native controller mode, report when committed
bool ControllerAction(void (*action)(void*),void*); // synchronous native binding operation, controller bank only
}
