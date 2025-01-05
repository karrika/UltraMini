objects= \
	palwiz \
	74LS02_16k.jed \
	74LS02_32k.jed \
	74LS02_48k.jed \
	SUPER.jed \
	SUPER_22V10.jed \
	SUPER_EXFIX.jed \
	SUPER_EXFIX_22V10.jed \
	SUPER_EXROM.jed \
	SUPER_EXROM_22V10.jed \
	Asteroids3D.bin \
	breakout.bin \
	Easter.bin \
	Cracked.bin \
	Crackedexrom.bin \
	AB.bin \
	1942xm.bin \
	RType2xm.bin \
	Warlords.bin \

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

SUPER_22V10.jed: SUPER_22V10.pal
	./palwiz SUPER_22V10.pal | tee SUPER_22V10.jed

SUPER_EXFIX.jed: SUPER_EXFIX.pal
	./palwiz SUPER_EXFIX.pal | tee SUPER_EXFIX.jed

SUPER_EXFIX_22V10.jed: SUPER_EXFIX_22V10.pal
	./palwiz SUPER_EXFIX_22V10.pal | tee SUPER_EXFIX_22V10.jed

SUPER_EXROM.jed: SUPER_EXROM.pal
	./palwiz SUPER_EXROM.pal | tee SUPER_EXROM.jed

SUPER_EXROM_22V10.jed: SUPER_EXROM_22V10.pal
	./palwiz SUPER_EXROM_22V10.pal | tee SUPER_EXROM_22V10.jed

# Test binary for 16k use 74LS02_16k.jed
Asteroids3D.bin: Asteroids3D.a78
	python3 build16k.py Asteroids3D.a78

# Test binary for 32k use 74LS02_32k.jed
breakout.bin: breakout.a78
	python3 build32k.py breakout.a78

# Test binary for 48k use 74LS02_48k.jed
Easter.bin: Easter.a78
	python3 build48k.py Easter.a78

# Test binary for 128k use SUPER_EXROM.jed
# This will create a 256k ROM
Crackedexrom.bin: Cracked.a78
	sign7800 -w Cracked.a78
	python3 exfix2exrom.py Cracked.a78
	python3 build144k.py Crackedexrom.a78

# Test binary for 128k use SUPER_EXFIX.jed
Cracked.bin: Cracked.a78
	sign7800 -w Cracked.a78
	python3 build128k.py Cracked.a78
	python3 filltosize.py Cracked.bin 256

# Test binary for 144k use SUPER_EXROM.jed
AB.bin: AB.a78
	sign7800 -w AB.a78
	python3 build144k.py AB.a78

# Test binary for 128k with 2600+ extend to 256k ROM use SUPER.jed
RType2xm.bin: RType2.a78
	python3 addxm.py RType2.a78 ff10
	python3 fixemptyblocks.py RType2xm.a78
	sign7800 -w RType2xm.a78
	python3 stripheader.py RType2xm.a78
	python3 filltosize.py RType2xm.bin 256

# Test binary for 256k with 2600+ hack use SUPER.jed
1942xm.bin: 1942.a78
	python3 addxm.py 1942.a78
	python3 fixemptyblocks.py 1942xm.a78
	sign7800 -w 1942xm.a78
	python3 stripheader.py 1942xm.a78

# Test binary for 4k 2600 game use 74LS02_16k.jed
Warlords.bin: Warlords.a26
	python3 build4kFill.py Warlords.a26

clean:
	rm -f $(objects) *.c~ Easter.bin breakout.bin Asteroids3D.bin AB.bin
	rm -f 1942.bin Warlords.bin
