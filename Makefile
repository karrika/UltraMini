objects= \
	palwiz \
	74LS02_16k.jed \
	74LS02_32k.jed \
	74LS02_48k.jed \
	Asteroids3D_INV1_16K.bin \
	breakout.bin \
	Easter.bin \

all: $(objects)

palwiz: palwiz.c
	gcc -o palwiz palwiz.c

74LS02_16k.jed: 74LS02_16k.pal
	./palwiz 74LS02_16k.pal | tee 74LS02_16k.jed

74LS02_32k.jed: 74LS02_32k.pal
	./palwiz 74LS02_32k.pal | tee 74LS02_32k.jed

74LS02_48k.jed: 74LS02_48k.pal
	./palwiz 74LS02_48k.pal | tee 74LS02_48k.jed

# Test binary for 16k
Asteroids3D_INV1_16K.bin: Asteroids3D_INV1_16K.a78
	python3 build16k.py Asteroids3D_INV1_16K.a78

# Test binary for 32k
breakout.bin: breakout.a78
	python3 build32k.py breakout.a78

# Test binary for 48k
Easter.bin: Easter.a78
	python3 build48k.py Easter.a78

clean:
	rm -f $(objects) *.c~ Easter.bin
