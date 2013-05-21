
all : patch-qboot

patch-qboot : patch_qboot.c
	gcc -o patch-qboot patch_qboot.c

clean:
	-rm -f patch-qboot.o
	-rm -f patch-qboot
