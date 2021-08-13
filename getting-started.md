# Getting started
They are plenty of ways to execute your own code on your console. In this section I'm going to show you the most flexible one as it applies to any game.

Dashlaunch is able to load up to 5 dynamic libraries (plugins) on start-up. We are going to learn how to create a plugin that Dashlaunch will load for us when the console boots.

## Project setup
Open Visual Studio and create a new Xbox 360 project.

<img src="./Resources/Screenshots/vs-create-project.png" alt="Visual Studio Xbox 360 Project"/>

**TODO: Add paragraph and screenshot about the project type**

Before writing any code, you'll need to set up a few things in Visual Studio to be able to compile and deploy to your console successfully. In the `Solution Explorer`, right-click on your project and click on `Properties` and make sure you are in the `Configuration Properties` tab (you should be by default).

- Go to `C/C++ > General` and set your application type is to Dynamic Library (DLL).
- Some properties about the XEX file can be set with an XML configuration file. Create an XML file that looks like this:
    ```XML
    <?xml version="1.0"?>
    <xex>
      <baseaddr addr="0x91D00000"/>
      <sysdll/>
      <format>
        <compressed/>
      </format>
      <mediatypes>
        <default/>
        <allpackages/>
      </mediatypes>
      <gameregion>
        <all/>
      </gameregion>
    </xex>
    ```
    The base address doesn't need to be `0x91D00000` but the it must not conflict with the base address of any other loaded plugin and must be greater `0x82000000` + the size of the application currently running. I recommended setting the base address as anything greater than `0x90000000`.
    Now set this file as your config file in `C/C++ > Configuration > File` (TO CHANGE!!!)
