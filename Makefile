objects= \
	palwiz \
	74LS02_48k.jed \
	Easter.bin \

all: $(objects)

palwiz: palwiz.c
	gcc -o palwiz palwiz.c

74LS02_48k.jed: 74LS02_48k.pal
	./palwiz 74LS02_48k.pal | tee 74LS02_48k.jed

Easter.bin: Easter.a78
	python3 build48k.py Easter.a78

clean:
	rm -f $(objects) *.c~ Easter.bin
