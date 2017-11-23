default: build/rom.bin

build/rom.bin: main.c
	mkdir -p build
	docker run -v `pwd`:/src -v `pwd`/build:/src/out beardedfoo/gendev:0.3.0

run-megaedx7: build/rom.bin
	megaedx7-run build/rom.bin
