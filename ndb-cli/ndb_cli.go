package main

import (
	"flag"
	"fmt"
	"math"
	"os"
	"strconv"
	"strings"
	"time"

	"github.com/godbus/dbus/v5"
	"github.com/godbus/dbus/v5/introspect"
)

type signals map[string]struct{}

func (s signals) String() string {
	return ""
}

func (s signals) Set(sigName string) error {
	s[sigName] = struct{}{}
	return nil
}

func (s signals) Exists(signal string) bool {
	_, ok := s[signal]
	return ok
}

type dbusCLI struct {
	conn       *dbus.Conn
	node       *introspect.Node
	dbusPath   dbus.ObjectPath
	dbusIface  string
	ifaceIndex int
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
func (d *dbusCLI) callMethod(methodName string, methodArgs []string, sigNames signals, sigTimeout int) error {
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
	serr := make(chan error)
	if len(sigNames) > 0 {
		go d.waitForSignal(sigNames, sigTimeout, serr)
	}
	call := obj.Call(fmt.Sprintf("%s.%s", d.dbusIface, methodName), 0, convArgs...)
	if call.Err != nil {
		return fmt.Errorf("error calling %s : %w", methodName, call.Err)
	}
	if len(call.Body) > 0 {
		for _, r := range call.Body {
			fmt.Printf("%v", r)
		}
		fmt.Printf("\n")
	}
	if len(sigNames) > 0 {
		if err := <-serr; err != nil {
			return fmt.Errorf("error waiting for signal after method call: %w", err)
		}
	}
	return nil
}
func (d *dbusCLI) waitForSignal(sigNames signals, sigTimeout int, err chan<- error) {
	var serr error
	sigFound := 0
	for _, s := range d.node.Interfaces[d.ifaceIndex].Signals {
		if sigNames.Exists(s.Name) {
			sigFound++
		}
	}
	if sigFound != len(sigNames) {
		err <- fmt.Errorf("one or more signals not found")
		return
	}
	if serr = d.conn.AddMatchSignal(
		dbus.WithMatchObjectPath(d.dbusPath),
		dbus.WithMatchInterface(d.dbusIface),
	); serr != nil {
		err <- fmt.Errorf("error adding match signal: %w", serr)
		return
	}
	c := make(chan *dbus.Signal)
	d.conn.Signal(c)
	timeout := time.Duration(sigTimeout) * time.Second
	if timeout == 0 {
		timeout = time.Duration(math.MaxInt64)
	}
	for {
		select {
		case v := <-c:
			name := v.Name[strings.LastIndex(v.Name, ".")+1:]
			if sigNames.Exists(name) {
				printSignal(v.Body, name)
				err <- nil
				return
			}
		case <-time.After(timeout):
			err <- fmt.Errorf("timeout after %ds", sigTimeout)
			return
		}
	}
}

func printSignal(val []interface{}, name string) {
	fmt.Printf("%s ", name)
	for _, s := range val {
		fmt.Printf("%v ", s)
	}
	fmt.Printf("\n")
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
	d.dbusIface = "com.github.shermp.nickeldbus"

	methodFlags := flag.NewFlagSet("method", flag.ExitOnError)
	// methodWaitForSig := d.methodFlags.String("wait-for-signal", "", "Wait for named signal after calling method.")
	// methodSigTimeout := d.methodFlags.Int("signal-timeout", 0, "When used with '--wait-for-signal', sets the time to wait for signal before timing out. 0 (default) disables timeout.")
	signalFlags := flag.NewFlagSet("signal", flag.ExitOnError)

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
		sigNames := make(signals)
		methodFlags.Var(&sigNames, "signal", "Wait for named signal after calling method.")
		sigTimeout := methodFlags.Int("signal-timeout", 0, "When used with '--signal', sets the time to wait for signal before timing out. 0 (default) disables timeout.")
		methodFlags.Parse(os.Args[2:])
		if methodFlags.NArg() < 1 {
			fmt.Printf("Expected method name\n")
			os.Exit(1)
		}
		methodName := methodFlags.Arg(0)
		methodArgs := methodFlags.Args()[1:]
		if err = d.callMethod(methodName, methodArgs, sigNames, *sigTimeout); err != nil {
			fmt.Printf("Error calling method: %s\n", err.Error())
			os.Exit(1)
		}
	case "signal":
		sigTimeout := signalFlags.Int("timeout", 0, "Sets the time to wait for signal before timing out. 0 (default) disables timeout.")
		signalFlags.Parse(os.Args[2:])
		if signalFlags.NArg() < 1 {
			fmt.Printf("Expected signal name\n")
		}
		sigNames := make(signals)
		for i := 0; i < signalFlags.NArg(); i++ {
			sigNames[signalFlags.Arg(i)] = struct{}{}
		}
		serr := make(chan error)
		go d.waitForSignal(sigNames, *sigTimeout, serr)
		if err = <-serr; err != nil {
			fmt.Printf("Error waiting for signal: %s\n", err.Error())
			os.Exit(1)
		}
	default:
		fmt.Printf("Unknown command '%s'!\n", os.Args[1])
		os.Exit(1)
	}
	os.Exit(0)
}
