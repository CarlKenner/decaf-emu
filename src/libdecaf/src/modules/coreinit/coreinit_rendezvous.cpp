#include <algorithm>
#include "coreinit.h"
#include "coreinit_core.h"
#include "coreinit_rendezvous.h"

namespace coreinit
{


/**
 * Initialise a rendezvous structure.
 */
void
OSInitRendezvous(OSRendezvous *rendezvous)
{
   rendezvous->core[0].store(0, std::memory_order_release);
   rendezvous->core[1].store(0, std::memory_order_release);
   rendezvous->core[2].store(0, std::memory_order_release);
}


/**
 * Wait on a rendezvous with infinite timeout.
 */
BOOL
OSWaitRendezvous(OSRendezvous *rendezvous,
                 uint32_t coreMask)
{
   return OSWaitRendezvousWithTimeout(rendezvous, coreMask, -1);
}


/**
 * Wait on a rendezvous with a timeout.
 *
 * This will wait with a timeout until all cores matching coreMask have
 * reached the rendezvous point.
 *
 * \return Returns TRUE on success, FALSE on timeout.
 */
BOOL
OSWaitRendezvousWithTimeout(OSRendezvous *rendezvous,
                            uint32_t coreMask,
                            OSTime timeoutNS)
{
   auto core = OSGetCoreId();
   auto success = FALSE;
   auto endTime = OSGetTime() + internal::nanosToTicks(timeoutNS);

   auto waitCore0 = (coreMask & (1 << 0)) != 0;
   auto waitCore1 = (coreMask & (1 << 1)) != 0;
   auto waitCore2 = (coreMask & (1 << 2)) != 0;

   // Set our core flag
   rendezvous->core[core].store(1, std::memory_order_release);

   do {
      if (waitCore0 && rendezvous->core[0].load(std::memory_order_acquire)) {
         waitCore0 = false;
      }

      if (waitCore1 && rendezvous->core[1].load(std::memory_order_acquire)) {
         waitCore1 = false;
      }

      if (waitCore2 && rendezvous->core[2].load(std::memory_order_acquire)) {
         waitCore2 = false;
      }

      if (!waitCore0 && !waitCore1 && !waitCore2) {
         success = TRUE;
         break;
      }

      if (timeoutNS != -1 && OSGetTime() >= endTime) {
         break;
      }

      // We must manually check for interrupts here, as we are busy-looping.
      //  Note that this is only safe as no locks are held during the wait.
      cpu::this_core::checkInterrupts();
   } while (true);

   return success;
}


void
Module::registerRendezvousFunctions()
{
   RegisterKernelFunction(OSInitRendezvous);
   RegisterKernelFunction(OSWaitRendezvous);
   RegisterKernelFunction(OSWaitRendezvousWithTimeout);
}

} // namespace coreinit
