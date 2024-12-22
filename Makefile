objects= \
	palwiz \
	SUPERCART22V10.jed \
	SUPERCART20V8.jed \

all: $(objects)

palwiz: palwiz.c
	gcc -o palwiz palwiz.c

SUPERCART22V10.jed: SUPERCART22V10.pal
	./palwiz SUPERCART22V10.pal | tee SUPERCART22V10.jed
	cp SUPERCART22V10.jed ~/dtop

SUPERCART20V8.jed: SUPERCART20V8.pal
	./palwiz SUPERCART20V8.pal | tee SUPERCART20V8.jed
	cp SUPERCART20V8.jed ~/dtop

clean:
	rm -f $(objects) *.c~
