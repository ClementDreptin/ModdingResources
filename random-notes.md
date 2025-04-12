# Random notes

Compilation of things I learn about the Xbox 360 that could later be put in proper pages.

### Dashlaunch

Dashlaunch exports functions to get and set options. You can import them using `GetModuleHandle("launch.xex")` and `GetProcAddress`, the ordinals for getting and settings option values by name are 9 and 10 respectively. An example of how to import these functions can be found [here](https://github.com/ClementDreptin/XexUtils/blob/master/src/DashLaunch.cpp).

### Hooking

Hooking a function requires storing some instructions of the hooked function in a buffer to later jump to it. This buffer can't be stack or heap allocated because these memory pages aren't executable. Instead, it needs to be allocated as a global variable. On modded consoles, the buffer can be in the `.data` section because page permissions are bypassed and code in the `.data` section can be executed, which is not the case on retail consoles or in emulators. To make sure code in this buffer can be executed in all environments, it is safer to allocated it in the `.text` section of the binary. This can be done using the `__declspec(allocate(".text"))` attribute. Beware that explicitly declaring the section using `#pragma section(".text")` is required even if this section is automatically created by the compiler. An example can be found [here](https://github.com/ClementDreptin/XexUtils/blob/ae8a8b832315678255c00d6a9b967a9136155503/src/Detour.cpp#L10-L15).

When using the debug runtime (`<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>` in `.vcxproj` file or the `/MTd` compiler flag), and hooking a title function from a system DLL, you can't call the original function. In this scenario, the jump instructions used to jump back to the original function are allocated by a system thread but used by a title thread. This is fine when using the regular runtime library but the debug one has extra checks that trigger a kernel exception (`stop code 0xf4: CRITICAL_OBJECT_TERMINATION`) and prints "This heap cannot be used by this thread." to the debug output. Those types of exceptions trigger a blue screen on Windows but on Xbox 360 the console just shuts down.

### Hypervisor

The hypervisor exposes functions which allow the kernel and titles to run privileged tasks. These functions can be called using the syscall instructions (`sc` in PowerPC). Modded consoles and consoles exploited with [BadUpdate](https://github.com/grimdoomer/Xbox360BadUpdate) and [FreeMyXe](https://github.com/FreeMyXe/FreeMyXe) have a backdoor installed in `HvxGetVersions` (syscall 0) which allows them to use it as a `memcpy` primitive. An example use of this backdoor can be found [here](https://github.com/ClementDreptin/XexUtils/blob/ae8a8b832315678255c00d6a9b967a9136155503/src/Hypervisor.cpp#L48).

Addresses given to hypervisor functions need to point to memory allocated using `XPhysicalAlloc` and not just `malloc`. Despite its name, `XPhysicalAlloc` still returns a virtual address, which needs to be converted to a physical address using `MmGetPhysicalAddress` and be bitwise OR'd with 0x8000000000000000. A helper function that converts a virtual address to a physical address can be found [here](https://github.com/ClementDreptin/XexUtils/blob/ae8a8b832315678255c00d6a9b967a9136155503/src/Hypervisor.cpp#L14).

### Threads

The [`CreateThread` function](https://learn.microsoft.com/fr-fr/windows/win32/api/processthreadsapi/nf-processthreadsapi-createthread) from the public Win32 API only allows you to create regular threads, and regular threads are attached to the currently running title, which means the thread ends whenever the current title changes.<br>
This is a problem when you create a thread from a system DLL and don't want it to end when you switch games or go back to the dashboard. In order to create a thread that isn't attached to the currently running title, you need to use the `ExCreateThread` function and pass `2` to the `creationFlags` argument. An example of creating a system thread can be found [here](https://github.com/ClementDreptin/Hayzen/blob/2560f7a57434fa73c853fe1f7c87b69caccbd81f/src/Core/Plugin.cpp#L34).<br>
It is also worth noting that creating system threads on the same hardware thread as the title is not possible. This is not a problem when writing a system DLL loaded by Dashlaunch because the Dashlaunch code that loads your DLL already runs on a different hardware thread. But when writing a title, you need to create the system thread in a paused state, assign it to a different hardware thread, and then resume it. An example of this entire process can be found [here](https://github.com/ClementDreptin/X360PluginManager/blob/2e85a7c33b0b0364c1e8ad31348fab9622b606c7/src/main.cpp#L157-L165).

### SMC

The SMC (System Management Controller) is the chip responsible for controlling power on the motherboard among other things, a more detailed explanation can be found [here](https://free60.org/Hardware/Console/SMC/). For example, you can set the power LED colors using the `HalSendSMCMessage` function, a helper function that does exactly this can be found [here](https://github.com/ClementDreptin/XexUtils/blob/ae8a8b832315678255c00d6a9b967a9136155503/src/SMC.cpp#L22).

### Sockets

Using sockets on the Xbox 360 without being connected to Xbox Live is pretty complicated. You can't simply use the WinSock API the same way you would on Windows and expect it to work. Anytime you want to create sockets from a system DLL, you need to use internal `NetDll_*` API instead of the `WSA_` functions and the regular socket functions from the C standard library. In general, the `NetDll_*` functions have the same signature as their public version except they take one more argument, the first one, which is the `xnCaller`. An enum with all the possible `xnCaller` values can be found [here](https://github.com/ClementDreptin/XexUtils/blob/ae8a8b832315678255c00d6a9b967a9136155503/src/Kernel.h#L92). For example, you can't use `WSAStartup` as is, you need to use `NetDll_WSAStartupEx` and pass `XNCALLER_SYSAPP` (`2`) to the first argument. To make my life easier, I made a set of macros which allow me to use the `NetDll_*` functions just like the public WinSock API, and these macros can be found [here](https://github.com/ClementDreptin/XexUtils/blob/ae8a8b832315678255c00d6a9b967a9136155503/src/Socket.cpp#L6).

Communication over sockets is encrypted by default on Xbox 360, and encrypting and decrypting requires an active Xbox Live connection, which means encryption needs to be disabled when creating the socket for the communcation to be possible without Xbox Live. Disabling encryption can be set just like any other socket option using `setsockopt` but the flag (`0x5801`) isn't defined in any header. An example of disabling encryption on a socket can be found [here](https://github.com/ClementDreptin/XexUtils/blob/ae8a8b832315678255c00d6a9b967a9136155503/src/Socket.cpp#L71).
