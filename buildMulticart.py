import sys

class lyx:

    def __init__(self, fname):
        self.f = open(fname + '.LYX', 'wb')
        self.empty = bytearray(16384)
        for i in range(16384):
            self.empty[i] = 255

    def write_empty(self):
        self.f.write(self.empty)

    def write_emptydata(self):
        self.write_empty()
        self.write_empty()
        self.write_empty()
        self.write_empty()

    def write_data(self, data):
        self.f.write(data)

    def close(self):
        self.f.close()


p = lyx('ULTRA')

fname='Easter.a78'
with open(fname, 'rb') as f:
    data = f.read()
p.write_empty()
p.write_data(data[128:])

fname='Asteroids3D_INV1_16K.a78'
with open(fname, 'rb') as f:
    data = f.read()
p.write_empty()
p.write_empty()
p.write_empty()
p.write_data(data[128:])

fname='Galaga (PAL) (Atari) (1987) (5990A9F8).a78'
with open(fname, 'rb') as f:
    data = f.read()
p.write_empty()
p.write_data(data[128:])

fname='breakout.a78'
with open(fname, 'rb') as f:
    data = f.read()
p.write_empty()
p.write_empty()
p.write_data(data[128:])
p.close()
