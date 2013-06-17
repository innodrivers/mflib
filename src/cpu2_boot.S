	.text

	.global cpu2boot_data
cpu2boot_data:
	.incbin "qboot.bin"

	.global cpu2boot_data_end
cpu2boot_data_end:
