# NickelDBus :: Monitor and control aspects of Kobo's Nickel application

A library to provide monitoring and control of the Nickel application over d-bus.

Built with help from code from NickelMenu.

## Installing

Grab the latest release from the releases page, and copy the the KoboRoot.tgz to the `.kobo` directory on your Kobo. Eject/disconnect your Kobo, and it should automatically reboot.

No configuration is required.

## Usage

NickelDBus is designed to give application developers a way of interacting with Kobo's nickel from a script or program. It can perform many of the same actions that NickelMenu can, and also provides a limited number of signals that can be monitored. For example, if you need to know when the content import process has completed, you can listen/wait for the `pfmDoneProcessing` signal.

NickelDBus exports the following interface:
```com.github.shermp.nickeldbus```
And can be found at the following path:
```/nickeldbus```

### Command line/shell script

Kobo devices provide the standard `dbus-send` and `dbus-monitor` tools by default. NickelDBus is fully compatible with these tools if you wish to use them (hint, they are a PITA to use...).

Alternatively, a CLI tool written Qt C++ has been created, called `qndb`. Usage is very simple:

Call a method
```
qndb -m <method_name> <method_args>
```
Wait indefinitely for a signal. The signal name and any of its outputs will be printed to stdout (space delimited)
```
qndb -s <signal_name>
```
Wait for a signal, with a 10s timeout (timeout is expressed in milliseconds)
```
qndb -t 10000 -s <signal_name>
```
Call a method, then wait for a signal, and timeout after 10s
```
qndb -s <signal_name> -t 10000 -m <method_name> <method_args>
```
It's possible to listen for multiple signals. The first signal that is received that matches one of the desired signals is output.
```
qndb -s <signal_name1> -s <signal_name2> ...
```
And yes, this also works for waiting for signal in method calls
```
qndb -s <signal_name1> -s <signal_name2> -m <method_args>
```

`qndb` returns 0 on success, or 1 otherwise.

### Language bindings

Bindings are available for most programming languages. For example, `qndb` and NickelDBus uses [QtDbus](https://doc.qt.io/qt-5/qtdbus-index.html).

## Compiling

This library was designed to be compiled with [NickelTC](https://github.com/geek1011/NickelTC).

A KoboRoot.tgz that can be installed on your Kobo can be generated with `make all koboroot`.

To start developing with NickelDBus, you will first need to generate the dbus adapter and proxy headers. You can run `make interface` to do this. Alternatively, `make all` will also do this as part of the compile process. Note, this requires the `qdbuscpp2xml` and `qdbusxml2cpp` programs from Qt, which are included with NickelTC.

To compile `qndb`, currently you need to change into the `src/cli` directory, and run `make cli`. Note, the interface files must have been created in the root directory first.