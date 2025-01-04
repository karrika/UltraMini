import sys
''' Add activation for 16k RAM and YM2152 sound chip in binary '''
def main():
    if len(sys.argv) < 2:
        print("Usage: python3 addxm.py <filename> [xmpos] [bin]")
        return

    filename = sys.argv[1]

    try:
        with open(filename, 'rb') as f:
            data = f.read()
        data = bytearray(data)
        ''' Point reset vector to 8 bytes before signature area '''
        '''
        FF78    a9 e4
        FF7A    8d 70 04
        FF7D    4c 4a fe
        '''
        if len(sys.argv) > 2:
            xmpos = sys.argv[2]
            index = len(data) - (int('10000',16) - int(xmpos,16))
        else:
            xmpos = 'ff78'
            index = len(data) - 128 - 8
        data[index] = 0xa9
        index += 1
        data[index] = 0xe4
        index += 1
        data[index] = 0x8d
        index += 1
        data[index] = 0x70
        index += 1
        data[index] = 0x04
        index += 1
        data[index] = 0x4c
        index += 1
        data[index] = data[len(data) - 4]
        index += 1
        data[index] = data[len(data) - 3]
        index = len(data) - 4
        data[index] = int(xmpos[2:], 16)
        index += 1
        data[index] = int(xmpos[0:2], 16)
        filename = filename.rsplit( ".", 1)[0] + "xm.a78"
        g = open(filename, 'wb')
        if len(sys.argv) > 3:
            g.write(data[128:])
        else:
            g.write(data)
    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
