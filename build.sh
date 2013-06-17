#!/bin/sh

[ -z ${BOARD} ] && {
	echo "please specifies BOARD=xxx. e.g."
	echo "BOARD=p4abu ./build.sh"
	exit 1
}

PATCH_QBOOT_TOOL=tools/patch-qboot/patch-qboot
QBOOT_BIN=board/${BOARD}/qboot.bin

#MACHID=3300
#ATAG_ADDR=0x46000100
#KERNEL_ADDR=0x46008000
CMDLINE="console=ttyS1,115200 mem=32M cpu1_mem=96M@0x40000000 mbox_mem=2M@0x41800000 "
CMDLINE=${CMDLINE}"init=/etc/preinit "
#CMDLINE=${CMDLINE}"init=/linuxrc "
CMDLINE=${CMDLINE}"root=/dev/mtdblock1 rw rootfstype=squashfs "
#CMDLINE=${CMDLINE}"root=nfs rw nfsroot=172.16.8.97:/home/drivers/workspace/nfs_share/rootfs,nolock ip=dhcp"

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
