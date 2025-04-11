# Reading assembly language (PowerPC)

The Xbox 360 has a PowerPC CPU so all of the examples shown in this page will then be in PowerPC.

## What is assembly?

Assembly is a textual representation of the CPU instructions contained in an executable file. It's the version of the code that's the closest to what the CPU executes while remaining (somewhat) human-readable.

## Why is it important?

Being able to read assembly language is important because this is the only way of looking at "the game's code" (unless the game's code is public, of course). Most companies don't make the source code of their games public for obvious reasons so the only way to look at how the game works is by "disassembling" it (converting the executable file into assembly language).

## How to convert an executable file to assembly?

You can do this by using a disassembler, the most common one is [IDA](https://hex-rays.com/ida-pro/). IDA is a paid software but there are plenty of resources online to get it for free that I won't link here. As an alternative, you can use [Ghidra](https://ghidra-sre.org/), which is free and open-source, but I don't personally don't use it, so the examples will be in IDA.
In order to open Xbox 360 executable files (XEX) in IDA, you'll need the [idaxex](https://github.com/emoose/idaxex) plugin. If you use Ghidra, you'll need the [XEXLoaderWV](https://github.com/zeroKilo/XEXLoaderWV) plugin.
Once everything is installed and set up, you can simply drag your XEX file in IDA and a prompt like this should appear.

<img src="./Images/ida-xex-prompt.png" alt="IDA Load XEX"/>

The options selected by default should be good, if not, set them as above and click OK.

And voilà! Your XEX file is converted to assembly!

<img src="./Images/ida.png" alt="IDA"/>

## Ok, what the hell is this?

PowerPC is pretty straight forward to learn because there are not that many instructions. Here's a list of references that I learned from:

-   [IBM assembler language reference](https://www.ibm.com/docs/en/aix/7.2?topic=aix-assembler-language-reference)<br>
    IBM is one of the companies that created PowerPC, their website is, therefore, one of the best resources.
-   [PPC - Basics Tutorial](https://www.se7ensins.com/forums/threads/ppc-basics-tutorial.927634/) from Const<br>
    Reading the IBM documentation can be a little overwhelming so Const made a concise tutorial with the most common instructions you need to understand to get started.

With IDA >7.5, you can decompile PowerPC assembly, so generate pseudo C code, which is much easier to read than assembly. You can decompile a function by pressing F5 when the cursor is in the function.

<br/><br/>

&rarr; [Next: Installing the development environment](install-env.md)
