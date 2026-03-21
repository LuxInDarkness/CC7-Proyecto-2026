// platform.h
#ifdef PLATFORM_QEMU
    #include "qemu/addr.h"
    #include "qemu/timer.h"
#else
    #include "beagle/addr.h"
    #include "beagle/timer.h"
#endif