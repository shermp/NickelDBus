package main

import (
	"flag"
	"fmt"
	"os"
	"strconv"

	"github.com/godbus/dbus/v5"
	"github.com/godbus/dbus/v5/introspect"
)

type dbusCLI struct {
	conn                     *dbus.Conn
	node                     *introspect.Node
	dbusPath                 dbus.ObjectPath
	dbusIface                string
	methodFlags, signalFlags *flag.FlagSet
	signalTimeout            int
	ifaceIndex               int
}

func (d *dbusCLI) initDBus() error {
	var err error
	d.conn, err = dbus.SystemBus()
	if err != nil {
		return fmt.Errorf("Error connecting to system bus: %w", err)
	}
	d.node, err = introspect.Call(d.conn.Object(d.dbusIface, d.dbusPath))
	if err != nil {
		d.conn.Close()
		return fmt.Errorf("Error introspecting NickelDBus: %w", err)
	}
	d.ifaceIndex = -1
	for i, iface := range d.node.Interfaces {
		if iface.Name == d.dbusIface {
			d.ifaceIndex = i
		}
	}
	if d.ifaceIndex < 0 {
		d.conn.Close()
		return fmt.Errorf("%s not in list of available interfaces", d.dbusIface)
	}
	return nil
}
func (d *dbusCLI) callMethod() error {
	d.methodFlags.Parse(os.Args[2:])
	if d.methodFlags.NArg() < 1 {
		return fmt.Errorf("expected method name")
	}
	methodName := d.methodFlags.Arg(0)
	methodArgs := d.methodFlags.Args()[1:]
	var convArgs []interface{}
	methodFound := false
	invalidArgs := false
	for _, m := range d.node.Interfaces[d.ifaceIndex].Methods {
		if m.Name == methodName {
			argCount := 0
			for _, a := range m.Args {
				if a.Direction == "in" {
					argCount++
				}
			}
			if len(methodArgs) == argCount {
				i := 0
				for _, a := range m.Args {
					if a.Direction == "in" {
						//fmt.Printf("Converting %s, which is of type %s\n", methodArgs[i], a.Type)
						v, err := strToDBusType(a.Type, methodArgs[i])
						if err != nil {
							fmt.Printf("Could not convert argument '%s' to type %s\n", methodArgs[i], a.Type)
							invalidArgs = true
							convArgs = nil
							break
						}
						convArgs = append(convArgs, v)
						i++
					}
				}
				if i == argCount {
					invalidArgs = false
				}
				if invalidArgs {
					continue
				}
				methodFound = true
			}
		}
	}
	if invalidArgs {
		return fmt.Errorf("invalid arguments")
	}
	if !methodFound {
		return fmt.Errorf("method '%s' not found", methodName)
	}

	obj := d.conn.Object(d.dbusIface, d.dbusPath)
	call := obj.Call(fmt.Sprintf("%s.%s", d.dbusIface, methodName), 0, convArgs...)
	if call.Err != nil {
		return fmt.Errorf("error calling %s : %w", methodName, call.Err)
	}
	for _, r := range call.Body {
		fmt.Printf("%v", r)
	}
	fmt.Printf("\n")
	return nil
}
func (d *dbusCLI) waitForSignal() error {
	var err error
	d.signalFlags.Parse(os.Args[2:])
	if d.signalFlags.NArg() < 1 {
		return fmt.Errorf("expected signal name")
	}
	sigName := d.signalFlags.Arg(0)
	sigFound := false
	for _, s := range d.node.Interfaces[d.ifaceIndex].Signals {
		if s.Name == sigName {
			sigFound = true
		}
	}
	if !sigFound {
		return fmt.Errorf("signal '%s' not found", sigName)
	}
	if err = d.conn.AddMatchSignal(
		dbus.WithMatchObjectPath(d.dbusPath),
		dbus.WithMatchInterface(d.dbusIface),
		dbus.WithMatchMember(sigName),
	); err != nil {
		return fmt.Errorf("error adding match signal: %w", err)
	}
	c := make(chan *dbus.Signal, 10)
	d.conn.Signal(c)
	for v := range c {
		for _, s := range v.Body {
			fmt.Printf("%v", s)
		}
		fmt.Printf("\n")
		break
	}
	return nil
}

func strToDBusType(typ string, val string) (interface{}, error) {
	var err error
	var b bool
	var i int64
	var ui uint64
	var f float64
	switch typ {
	case "y":
		if ui, err = strconv.ParseUint(val, 10, 8); err == nil {
			return byte(ui), nil
		}
	case "b":
		if b, err = strconv.ParseBool(val); err == nil {
			return b, nil
		}
	case "n":
		if i, err = strconv.ParseInt(val, 10, 16); err == nil {
			return int16(i), nil
		}
	case "q":
		if ui, err = strconv.ParseUint(val, 10, 16); err == nil {
			return uint16(ui), nil
		}
	case "i":
		if i, err = strconv.ParseInt(val, 10, 32); err == nil {
			return int32(i), nil
		}
	case "u":
		if ui, err = strconv.ParseUint(val, 10, 32); err == nil {
			return uint32(ui), nil
		}
	case "x":
		if i, err = strconv.ParseInt(val, 10, 64); err == nil {
			return int64(i), nil
		}
	case "t":
		if ui, err = strconv.ParseUint(val, 10, 64); err == nil {
			return uint64(ui), nil
		}
	case "h":
		if f, err = strconv.ParseFloat(val, 64); err == nil {
			return f, nil
		}
	case "s":
		return val, nil
	default:
		return nil, fmt.Errorf("unsupported dbus type '%s'", typ)
	}
	return nil, err
}

func main() {
	var err error
	d := dbusCLI{}
	d.dbusPath = dbus.ObjectPath("/nickeldbus")
	d.dbusIface = "local.shermp.nickeldbus"

	d.methodFlags = flag.NewFlagSet("method", flag.ExitOnError)
	// methodWaitForSig := d.methodFlags.String("wait-for-signal", "", "Wait for named signal after calling method.")
	// methodSigTimeout := d.methodFlags.Int("signal-timeout", 0, "When used with '--wait-for-signal', sets the time to wait for signal before timing out. 0 (default) disables timeout.")
	d.signalFlags = flag.NewFlagSet("signal", flag.ExitOnError)

	if len(os.Args) < 2 {
		fmt.Printf("Expected 'method' or 'signal' subcommands\n")
		os.Exit(1)
	}
	if err = d.initDBus(); err != nil {
		fmt.Printf("Error initializing dbus connection: %s\n", err.Error())
		os.Exit(1)
	}
	defer d.conn.Close()
	switch os.Args[1] {
	case "method":
		if err = d.callMethod(); err != nil {
			fmt.Printf("Error calling method: %s\n", err.Error())
			os.Exit(1)
		}
	case "signal":
		if err = d.waitForSignal(); err != nil {
			fmt.Printf("Error waiting for signal: %s\n", err.Error())
			os.Exit(1)
		}
	default:
		fmt.Printf("Unknown command '%s'!\n", os.Args[1])
		os.Exit(1)
	}
	os.Exit(0)
}
