#!/usr/bin/env python3
import sys
import struct
import gdbremote
import argparse
import bits.bootargs

arg = argparse.ArgumentParser()
arg.add_argument('--host', default='localhost', help='Host for OpenOCD and console terminal server')
arg.add_argument('--gdb-port', default=3333, type=int, help='GDB remote protocol port')
arg.add_argument('--timeout', type=int, default=3600, help='Timeout in seconds for app to complete')
arg.add_argument('image', help='Image in .elf format to be loaded and run')

bits.bootargs.BootArguments.add_arguments(arg)

opt = arg.parse_args(sys.argv[1:])

gdb  = gdbremote.GDBclient()
try:
    gdb.connect(opt.host, opt.gdb_port)
except Exception as e:
    print("Failed to connect to OpenOCD on {}\nReason: {}".format(opt.host, str(e)))
    sys.exit(1)

# Inhibit reset of M3 debug block
gdb.write16(0xfc0118, 8)

# Remap console after boot workaround
gdb.write32(0xfc0d30, 17)

gdb.monitor_cmd("reset halt")
entry = gdb.load(opt.image)

args = bits.bootargs.BootArguments.factory(options=opt)
argc, argv = gdb.bootargs(args)
#argc = 0
#argv = 0

gdb.set_reg(15, entry)
gdb.set_reg(0, argc)
gdb.set_reg(1, argv)

gdb.cont()

sys.exit(0)
