#!/bin/sh

PATCH_QBOOT_TOOL=tools/patch-qboot/patch-qboot
QBOOT_BIN=src/qboot.bin

#MACHID=3300
#ATAG_ADDR=0x46000100
#KERNEL_ADDR=0x46008000
CMDLINE="console=ttyS1,115200 init=/linuxrc mem=32M cpu1_mem=64M@0x42000000 mbox_mem=2M@0x41800000 "

#CMDLINE=${CMDLINE}"root=nfs rw nfsroot=172.16.8.233:/home/drivers/workspace/nfs_share/p4a_root,nolock ip=dhcp"

#if patch-qboot not exists, then build it
[ -f ${PATCH_QBOOT_TOOL} ] || {
	echo "build patch-qboot tool..."
	local patch_qboot_dir=`dirname ${PATCH_QBOOT_TOOL}`
	cd ${patch_qboot_dir}
	make

	echo "tool build done."
	echo ""
	-cd -
}

#if patch-qboot still not available, then exit
[ -x ${PATCH_QBOOT_TOOL} ] ||  {
	echo "no patch-qboot available!"
	exit 1
}

echo "patch the qboot.bin ..."
${PATCH_QBOOT_TOOL} ${QBOOT_BIN} ${CMDLINE:+-c "${CMDLINE}"} ${MACHID:+-m ${MACHID}} ${ATAG_ADDR:+-a ${ATAG_ADDR}} ${KERNEL_ADDR:+-k ${KERNEL_ADDR}}
echo "patch done."
echo ""

echo "start to build mflib ..."
make 1>/dev/null
[ $? = 0 ] || {
	echo "build mflib failed!"
	exit 1
}

echo "mflib build done."
