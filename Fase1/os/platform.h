// platform.h
#ifdef PLATFORM_QEMU
    #include "qemu/addr.h"
    #include "qemu/timer.h"
    #include "qemu/watchdog.h"
#else
    #include "beagle/addr.h"
    #include "beagle/timer.h"
    #include "beagle/watchdog.h"
#endif