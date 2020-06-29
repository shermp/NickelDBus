package main

import (
	"flag"
	"fmt"
	"os"
	"strconv"

	"github.com/godbus/dbus/v5"
	"github.com/godbus/dbus/v5/introspect"
)

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
	dbusPath := dbus.ObjectPath("/nickeldbus")
	dbusIface := "local.shermp.nickeldbus"

	methodCmd := flag.NewFlagSet("method", flag.ExitOnError)
	signalCmd := flag.NewFlagSet("signal", flag.ExitOnError)

	if len(os.Args) < 2 {
		fmt.Printf("Expected 'method' or 'signal' subcommands\n")
		os.Exit(1)
	}
	conn, err := dbus.SystemBus()
	if err != nil {
		fmt.Printf("Error connecting to system bus: %s\n", err.Error())
		os.Exit(1)
	}
	defer conn.Close()
	node, err := introspect.Call(conn.Object(dbusIface, dbusPath))
	if err != nil {
		fmt.Printf("Error introspecting NickelDBus: %s\n", err.Error())
		os.Exit(1)
	}
	ifaceIndex := -1
	for i, iface := range node.Interfaces {
		if iface.Name == dbusIface {
			ifaceIndex = i
		}
	}
	if ifaceIndex < 0 {
		fmt.Printf("%s not in list of available interfaces\n", dbusIface)
		os.Exit(1)
	}
	switch os.Args[1] {
	case "method":
		methodCmd.Parse(os.Args[2:])
		if methodCmd.NArg() < 1 {
			fmt.Printf("Expected method name\n")
			os.Exit(1)
		}
		methodName := methodCmd.Arg(0)
		methodArgs := methodCmd.Args()[1:]
		convArgs := make([]interface{}, 0)
		methodFound := false
		for _, m := range node.Interfaces[ifaceIndex].Methods {
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
								os.Exit(1)
							}
							convArgs = append(convArgs, v)
							i++
						}
					}
					methodFound = true
				}
			}
		}
		if !methodFound {
			fmt.Printf("Method '%s' not found\n", methodName)
			os.Exit(1)
		}

		obj := conn.Object(dbusIface, dbusPath)
		call := obj.Call(dbusIface+"."+methodName, 0, convArgs...)
		if call.Err != nil {
			fmt.Printf("Error calling %s : %s\n", methodName, call.Err.Error())
			os.Exit(1)
		}
		for _, r := range call.Body {
			fmt.Printf("%v", r)
		}
		fmt.Printf("\n")
	case "signal":
		signalCmd.Parse(os.Args[2:])
		if signalCmd.NArg() < 1 {
			fmt.Printf("Expected signal name\n")
			os.Exit(1)
		}
		sigName := signalCmd.Arg(0)
		sigFound := false
		for _, s := range node.Interfaces[ifaceIndex].Signals {
			if s.Name == sigName {
				sigFound = true
			}
		}
		if !sigFound {
			fmt.Printf("Signal '%s' not found\n", sigName)
			os.Exit(1)
		}
		if err = conn.AddMatchSignal(
			dbus.WithMatchObjectPath(dbusPath),
			dbus.WithMatchInterface(dbusIface),
			dbus.WithMatchMember(sigName),
		); err != nil {
			fmt.Printf("Error adding match signal: %s\n", err.Error())
			os.Exit(1)
		}
		c := make(chan *dbus.Signal, 10)
		conn.Signal(c)
		for v := range c {
			for _, s := range v.Body {
				fmt.Printf("%v", s)
			}
			fmt.Printf("\n")
			break
		}
	}
	os.Exit(0)
}
