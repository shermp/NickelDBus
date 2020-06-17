# NickelDBus :: Monitor and control aspects of Kobo's Nickel application

An experimental library to provide monitoring and control of the Nickel application over d-bus.

Built with help from code from NickelMenu.

## Compiling

This library was designed to be compiled with [NickelTC](https://github.com/geek1011/NickelTC).

To start developing with NickelDBus, you will first need to generate the dbus adapter headers. You can run `make adapter` to do this. Alternatively, `make all` will also do this as part of the compile process. Note, this requires the `qdbuscpp2xml` and `qdbusxml2cpp` programs from Qt, which are included with NickelTC. 