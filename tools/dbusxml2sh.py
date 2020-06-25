#!/usr/bin/env python3

# A quick and dirty script to create shell script wrapper functions around dbus-send & dbus-monitor, 
# because those two tools are a pain to use.
#
# It uses the d-bus introspection XML file to create the wrappers

import argparse
import os
import xml.etree.ElementTree as ET

def typecode_to_str(code):
    if code == 'y':
        return 'byte'
    elif code == 'b':
        return 'boolean'
    elif code == 'n':
        return 'int16'
    elif code == 'q':
        return 'uint16'
    elif code == 'i':
        return 'int32'
    elif code == 'u':
        return 'uint32'
    elif code == 'x':
        return 'int64'
    elif code == 't':
        return 'uint64'
    elif code == 'd':
        return 'double'
    elif code == 'h':
        return 'unix_fd'
    elif code == 's':
        return 'string'

def process_signal(signal, signals, bus, iface, object):
    pass


# Note, the methods dict is as follows
# {
#     'common_cmd': '...',
#     'methods': 
#     {
#         'method_name': 
#         {
#             'cmd': '...',
#             'args':[
#               ['arg1', 'arg2', ...]
#             ]
#         }
#     }
# }
def process_method(method, methods, iface, obj):
    method_name = method.attrib['name']
    # Overloaded methods are possible
    if method_name in methods['methods']:
        methods['methods'][method_name]['args'].append([])
        index = 1
        for arg in method:
            if arg.attrib['direction'] == 'in':
                arg_str = f'{typecode_to_str(arg.attrib["type"])}:"${index}"'
                methods['methods'][method_name]['args'][-1].append(arg_str)
                index += 1
    else:
        methods['methods'][method_name] = {}
        methods['methods'][method_name]['cmd'] = f'--dest={iface} {obj} {iface}.{method_name}'
        methods['methods'][method_name]['args'] = []
        methods['methods'][method_name]['args'].append([])
        index = 1
        for arg in method:
            if arg.attrib['direction'] == 'in':
                arg_str = f'{typecode_to_str(arg.attrib["type"])}:"${index}"'
                methods['methods'][method_name]['args'][0].append(arg_str)
                index += 1
    return
def create_shell_str(methods, signals):
    script = '#!/bin/sh\n\n# Shell function wrappers for dbus-send and dbus-monitor\n\n'
    funcs = []
    for name, method in methods['methods'].items():
        for i, arg_list in enumerate(method['args']):
            fn_name = f"ndb_{name}"
            if i > 0:
                fn_name += "_{i}"
            cmd = f'{methods["common_cmd"]} {method["cmd"]}'
            for arg in arg_list:
                cmd += f' {arg}'
            funcs.append(f'{fn_name}()\n{{\n    {cmd}\n    return 0\n}}\n\n')
            # TODO: parse the return output of dbus-send and return with an appropriate value
    for func in funcs:
        script += func
    return script

def main():
    parser = argparse.ArgumentParser(description='Create shell function wrappers for dbus-send & dbus-monitor')
    parser.add_argument('introspection_xml', help='The d-bus introspection XML file to parse')
    parser.add_argument('object_path', help='The object path on the bus')
    parser.add_argument('output_file', help='The output shell file to write')
    parser.add_argument('--bus_name', '-b', default='system', help='Which bus to connect to. Defaults to the system bus')
    args = parser.parse_args()
    if args.bus_name not in ('system', 'session'):
        print('"bus_name" must be one of "system" or "session"!')
        return
    try:
        tree = ET.parse(args.introspection_xml)
    except Exception as e:
        print(f'Failed to parse d-bus introspection XML with error {e}')
        return
    node = tree.getroot()
    if node.tag != 'node':
        print(f'Unexpected xml tag. Expected "node", got "{node.tag}"')
        return
    methods = {
        'common_cmd': f'dbus-send --{args.bus_name} --print-reply --type="method_call"', 
        'methods':{}
        }
    signals = {}
    for iface in node:
        if iface.tag != 'interface':
            print(f'Unexpected xml tag. Expected "interface", got "{iface.tag}"')
            return
        iface_name = iface.attrib['name']
        for fn in iface:
            if fn.tag == 'signal':
                pass
            elif fn.tag == 'method':
                process_method(fn, methods, iface_name, args.object_path)
            else:
                print('Unsupported tag')
    with open(args.output_file, 'wb') as o:
        o.write(create_shell_str(methods, signals).encode('utf-8'))

if __name__ == '__main__':
    main()