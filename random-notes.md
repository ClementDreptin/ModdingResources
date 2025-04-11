# Random notes

Compilation of things I learn about the Xbox 360 that could later be put in proper pages.

### Dashlaunch

Dashlaunch exports functions to get and set options. You can import them using `GetModuleHandle("launch.xex")` and `GetProcAddress`, the ordinals for getting and settings option values by name are 9 and 10 respectively. An example of how to import these functions can be found [here](https://github.com/ClementDreptin/XexUtils/blob/master/src/DashLaunch.cpp).

### Hooking

Hooking a function requires storing some instructions of the hooked function in a buffer to later jump to it. This buffer can't be stack or heap allocated because these memory pages aren't executable. Instead, it needs to be allocated as a global variable. On modded consoles, the buffer can be in the `.data` section because page permissions are bypassed and code in the `.data` section can be executed, which is not the case on retail consoles or in emulators. To make sure code in this buffer can be executed in all environments, it is safer to allocated it in the `.text` section of the binary. This can be done using the `__declspec(allocate(".text"))` attribute. Beware that explicitly declaring the section using `#pragma section(".text")` is required even if this section is automatically created by the compiler. An example can be found [here](https://github.com/ClementDreptin/XexUtils/blob/ae8a8b832315678255c00d6a9b967a9136155503/src/Detour.cpp?plain=1#L10-L15).

When using the debug runtime (`<RuntimeLibrary>MultiThreadedDebug</RuntimeLibrary>` in `.vcxproj` file or the `/MTd` compiler flag), and hooking a title function from a system DLL, you can't call the original function. In this scenario, the jump instructions used to jump back to the original function are allocated by a system thread but used by a title thread. This is fine when using the regular runtime library but the debug one has extra checks that trigger a kernel exception (`stop code 0xf4: CRITICAL_OBJECT_TERMINATION`) and prints "This heap cannot be used by this thread." to the debug output. Those types of exceptions trigger a blue screen on Windows but on Xbox 360 the console just shuts down.

### Hypervisor

The hypervisor exposes functions which allow the kernel and titles to run privileged tasks. These functions can be called using the syscall instructions (`sc` in PowerPC). Modded consoles and consoles exploited with [BadUpdate](https://github.com/grimdoomer/Xbox360BadUpdate) and [FreeMyXe](https://github.com/FreeMyXe/FreeMyXe) have a backdoor installed in `HvxGetVersions` (syscall 0) which allows them to use it as a `memcpy` primitive. An example use of this backdoor can be found [here](https://github.com/ClementDreptin/XexUtils/blob/ae8a8b832315678255c00d6a9b967a9136155503/src/Hypervisor.cpp#L48).

Addresses given to hypervisor functions need to point to memory allocated using `XPhysicalAlloc` and not just `malloc`. Despite its name, `XPhysicalAlloc` still returns a virtual address, which needs to be converted to a physical address using `MmGetPhysicalAddress` and be bitwise OR'd with 0x8000000000000000. A helper function that converts a virtual address to a physical address can be found [here](https://github.com/ClementDreptin/XexUtils/blob/ae8a8b832315678255c00d6a9b967a9136155503/src/Hypervisor.cpp#L14).
