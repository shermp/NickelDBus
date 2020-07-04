# NickelDBus :: Monitor and control aspects of Kobo's Nickel application

A library to provide monitoring and control of the Nickel application over d-bus.

Built with help from code from NickelMenu.

## Installing

Grab the latest release from the releases page, and copy the the KoboRoot.tgz to the `.kobo` directory on your Kobo. Eject/disconnect your Kobo, and it should automatically reboot.

No configuration is required.

## Usage

NickelDBus is designed to give application developers a way of interacting with Kobo's nickel from a script or program. It can perform many of the same actions that NickelMenu can, and also provides a limited number of signals that can be monitored. For example, if you need to know when the content import process has completed, you can listen/wait for the `pfmDoneProcessing` signal.

NickelDBus exports the following interface:
```local.shermp.nickeldbus```
And can be found at the following path:
```/nickeldbus```

### Command line/shell script

Kobo devices provide the standard `dbus-send` and `dbus-monitor` tools by default. NickelDBus is fully compatible with these tools if you wish to use them (hint, they are a PITA to use...).

Alternatively, a CLI tool written in Go has been created, called `ndb-cli`. Usage is very simple:

Call a method
```
ndb-cli method <method_name> <method_args>
```
Wait indefinitely for a signal
```
ndb-cli signal <signal_name>
```
Wait for a signal, with a 10s timeout
```
ndb-cli signal --timeout 10 <signal_name>
```
Call a method, then wait for a signal, and timeout after 10s
```
ndb-cli method --wait-for-signal <signal_name> --signal-timeout 10 <method_name> <method_args>
```

`ndb-cli` returns 0 on success, or 1 otherwise.

### Language bindings

Bindings are available for most programming languages. For example, `ndb-cli` uses [godbus](https://github.com/godbus/dbus) and NickelDBus itself uses [QtDbus](https://doc.qt.io/qt-5/qtdbus-index.html).

## Compiling

This library was designed to be compiled with [NickelTC](https://github.com/geek1011/NickelTC).

A KoboRoot.tgz that can be installed on your Kobo can be generated with `make all koboroot`.

To start developing with NickelDBus, you will first need to generate the dbus adapter headers. You can run `make adapter` to do this. Alternatively, `make all` will also do this as part of the compile process. Note, this requires the `qdbuscpp2xml` and `qdbusxml2cpp` programs from Qt, which are included with NickelTC. 