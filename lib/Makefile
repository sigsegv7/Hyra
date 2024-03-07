mlibc/build/:
	cd mlibc; ln -s ../../cross/bin/ crossbin
	cd mlibc; meson setup -Ddisable_crypt_option=true -Ddisable_iconv_option=true \
		-Ddisable_intl_option=true -Ddisable_libgcc_dependency=true \
		-Ddisable_linux_option=true -Ddisable_libgcc_dependency=false \
		-Dmlibc_no_headers=true \
		--cross-file=cross_file.txt build; cd build/; ninja

.PHONY: clean
clean:
	rm -rf mlibc/build; rm -f mlibc/crossbin
