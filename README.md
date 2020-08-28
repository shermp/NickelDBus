# NickelDBus :: Monitor and control aspects of Kobo's Nickel application

A library to provide monitoring and control of the Nickel application over d-bus.

Built with help from code from NickelMenu.

## Installing

Grab the latest release from the releases page, and copy the the KoboRoot.tgz to the `.kobo` directory on your Kobo. Eject/disconnect your Kobo, and it should automatically reboot.

No configuration is required.

Note, to check what version of NickelDBus you have installed, open the file `.adds/nickeldbus` in a text editor.

## Uninstalling

To uninstall NickelDBus, simply delete the file called `nickeldbus` in the `.adds` directory, and reboot your Kobo.

## Quickstart

NickelDBus is designed to give application developers a way of interacting with Kobo's nickel from a script or program. It can perform many of the same actions that NickelMenu can, and also provides a limited number of signals that can be monitored. For example, if you need to know when the content import process has completed, you can listen/wait for the `pfmDoneProcessing` signal.

NickelDBus exports the following interface:
```com.github.shermp.nickeldbus```
And can be found at the following path:
```/nickeldbus```

You can use the included `qndb` command line tool to get started with.

Documentation can be found [here](https://shermp.github.io/nickeldbus)

## Compiling

This library was designed to be compiled with [NickelTC](https://github.com/geek1011/NickelTC).

A KoboRoot.tgz that can be installed on your Kobo can be generated with `make koboroot`.

To start developing with NickelDBus, you will first need to generate the dbus adapter and proxy headers. You can run `make interface` to do this. Alternatively, `make` will also do this as part of the compile process. Note, this requires the `qdbuscpp2xml` and `qdbusxml2cpp` programs from Qt, which are included with NickelTC.

To compile `qndb`, run `make cli`. Note, you will need to run this before `make koboroot`.