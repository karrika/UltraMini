objects= \
	palwiz \
	74LS02_16k.jed \
	74LS02_32k.jed \
	74LS02_48k.jed \
	SUPER.jed \
	SUPER_EXFIX.jed \
	SUPER_EXROM.jed \
	Asteroids3D.bin \
	breakout.bin \
	Easter.bin \
	Cracked.bin \
	AB.bin \
	1942.bin \

all: $(objects)

palwiz: palwiz.c
	gcc -o palwiz palwiz.c

74LS02_16k.jed: 74LS02_16k.pal
	./palwiz 74LS02_16k.pal | tee 74LS02_16k.jed

74LS02_32k.jed: 74LS02_32k.pal
	./palwiz 74LS02_32k.pal | tee 74LS02_32k.jed

74LS02_48k.jed: 74LS02_48k.pal
	./palwiz 74LS02_48k.pal | tee 74LS02_48k.jed

SUPER.jed: SUPER.pal
	./palwiz SUPER.pal | tee SUPER.jed

SUPER_EXFIX.jed: SUPER_EXFIX.pal
	./palwiz SUPER_EXFIX.pal | tee SUPER_EXFIX.jed

SUPER_EXROM.jed: SUPER_EXROM.pal
	./palwiz SUPER_EXROM.pal | tee SUPER_EXROM.jed

# Test binary for 16k
Asteroids3D.bin: Asteroids3D.a78
	python3 build16k.py Asteroids3D.a78

# Test binary for 32k
breakout.bin: breakout.a78
	python3 build32k.py breakout.a78

# Test binary for 48k
Easter.bin: Easter.a78
	python3 build48k.py Easter.a78

# Test binary for 128k
Cracked.bin: Cracked.a78
	python3 build128k.py Cracked.a78

# Test binary for 144k
AB.bin: AB.a78
	python3 build144k.py AB.a78

# Test binary for 256k
1942.bin: 1942.a78
	python3 build256k.py 1942.a78

clean:
	rm -f $(objects) *.c~ Easter.bin breakout.bin Asteroids3D.bin AB.bin
