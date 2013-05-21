#!/bin/sh

PATCH_QBOOT_TOOL=tools/patch-qboot/patch-qboot
QBOOT_BIN=src/qboot.bin

#MACHID=3300
#ATAG_ADDR=0x46000100
#KERNEL_ADDR=0x46008000
CMDLINE="console=ttyS1,115200 init=/linuxrc mem=32M cpu1_mem=64M@0x42000000 mbox_mem=2M@0x41800000 "

#CMDLINE=${CMDLINE}"root=nfs rw nfsroot=172.16.8.233:/home/drivers/workspace/nfs_share/p4a_root,nolock ip=dhcp"

[ -f tools/patch-qboot/patch-qboot ] || {
	cd tools/patch-qboot/
	make
}

[ -x tools/patch-qboot/patch-qboot ] ||  exit 1

${PATCH_QBOOT_TOOL} ${QBOOT_BIN} ${CMDLINE:+-c "${CMDLINE}"} ${MACHID:+-m ${MACHID}} ${ATAG_ADDR:+-a ${ATAG_ADDR}} ${KERNEL_ADDR:+-k ${KERNEL_ADDR}}

make
