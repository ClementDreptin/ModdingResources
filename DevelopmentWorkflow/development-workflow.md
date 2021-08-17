# Development workflow
This section will show you ONE way to make your development experience a little less frustating. This is what I personally do and is by no means the most efficient way of programming for the Xbox 360.

## Why?
You will soon realize that it's important to have an efficient workflow when you will have to entirely reboot your console just to add or remove a print statement. If you followed the previous sections, you set your DLL as a Dashlaunch plugin, which gets loaded on start-up. This means that, if you want to reload your DLL, you need to reboot your console. This is extremely wasteful and time consumming so I'll show a few ways to ease your pain.

## Loading/Unloading modules on demand
What can save you a lot of time is being able to load a DLL, see what it does, make some changes then unload it and load it back to apply the changes. This can be achieved in various ways, I'm going to show you one.

I use [ModuleLoader](https://github.com/ClementDreptin/ModuleLoader) to load and unload modules remotely from my computer, it's the one I master the most for one simple reason: I made it. ModuleLoader is a command line tool so there is no graphical interface. If you only used Windows you probably rarely ever used the command prompt but trust me, it's worth learning!

I'll let you read the README of the repository to download it and install what's needed. The documentation is also in the README.

### The limits
The biggest downside is that it becomes very tricky to unload a DLL if this DLL created a thread. Cleaning up threads is a very complicated topic, especially when you're stuck using an old version of the C++ standard library that does not support the `std::thread` API. I searched for a very long time throughout the internet and could never find a way to properly clean up a thread before unloading a DLL. My knowledge of the Win32 C threading API is also very limited so, if anyone knows how to solve this problem PLEASE TELL ME!
This is a problem because Dashlaunch does not create a thread for you when loading your DLL. This means that, if your DLL is built with an update loop (it's usually the case), you absolutely need to create a thread otherwise your DLL is going to heavily slow the main thread.

## Using a debug console (Xbox Watson)
The XDK comes with a debug console called Xbox Watson, it's not that good and very buggy but that's all we have.

**TODO: explain how to open and set up Xbox Watson**

In order to print a message in the Xbox Watson console you're going to need to use the `DbgPrint` function from `xboxkrnl.lib`. You can do so by adding the following code somewhere in your project:
```C++
extern "C" VOID DbgPrint(LPCSTR s, ...);
```
`DbgPrint` is a variadic function that works like `printf`, an example usage could be:
```C++
DbgPrint("value: %i", 3); // Prints "value: 3"
```

### Disclaimer
Xbox Watson is very buggy, especially when exceptions occur (that's the case when you just have an RGH and not an actual devkit at least). When Xbox Watson detects that an exception occured on your console, it's gonna tell you so and clicking OK or cancel will just make the pop-up appear again. At this point you're basically stuck, you can't close Xbox Watson by clicking the cross in the top-right corner so the only way to stop it is by killing its process in your task manager.
One thing that's worth knowing, you can't use ModuleLoader if you have a connection open in Xbox Watson, I don't know if the bug comes from Xbox Watson or ModuleLoader, I haven't tested with any other remote tool. So, if you want to use ModuleLoader, be sure to close the Xbox Watson connection first.
