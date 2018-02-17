#!/bin/sh

# Determine the platform name by any means possible
# Should result in <os>-<osversion>-<dev-type>-<arch>
#

KERNEL="`uname -s`"
MACHINE="`uname -m`"
OS="`uname -o`"

echo "$OS:$KERNEL:$MACHINE"

