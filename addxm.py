import sys
''' Add activation for 16k RAM and YM2152 sound chip in binary '''
def main():
    if len(sys.argv) != 2:
        print("Usage: python3 addxm.py <filename>")
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
        data[index] = 0x78
        index += 1
        data[index] = 0xff
        filename = filename.rsplit( ".", 1)[0] + "p.a78"
        g = open(filename, 'wb')
        g.write(data)
    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
