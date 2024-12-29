import sys
''' Add activation for 16k RAM and YM2152 sound chip in binary '''
def main():
    if len(sys.argv) != 2:
        print("Usage: python3 build256k.py <filename>")
        return

    filename = sys.argv[1]

    try:
        with open(filename, 'rb') as f:
            data = f.read()
        if len(data) != 256 * 1024 + 128:
            print("Size should be", 256 * 1024 + 128, "not", len(data))
            quit()
        data = bytearray(data)
        ''' Point reset vector to 8 bytes before signature area '''
        '''
        FF80    a5 e4
        FF82    8d 70 04
        FF85    4c 4a fe

        FF78    a5 e4
        FF7A    8d 70 04
        FF7D    4c 4a fe
        '''
        index = 256 * 1024 + 128 - 128 - 8
        data[index] = 0xa5
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
        data[index] = data[256 * 1024 + 128 - 4]
        index += 1
        data[index] = data[256 * 1024 + 128 - 3]
        index = 256 * 1024 + 128 - 4
        data[index] = 0x78
        index += 1
        data[index] = 0xff
        filename = filename.rsplit( ".", 1)[0] + ".bin"
        g = open(filename, 'wb')
        g.write(data[128:])
    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
