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
        ''' scan for empty pages '''
        emptyblock = bytearray()
        for k in range(2 * 1024):
            emptyblock.append(0x45)
            emptyblock.append(0x4D)
            emptyblock.append(0x50)
            emptyblock.append(0x54)
            emptyblock.append(0x59)
            emptyblock.append(0x2D)
            emptyblock.append(0x2D)
            emptyblock.append(0x20)
        newdata = data[:128]
        for i in range(int((len(data) - 128) / (16 * 1024))):
            start = 128 + i * 16 * 1024
            val = data[start]
            empty = 1
            for j in range(16 * 1024):
                if data[start + j] != val:
                    empty = 0
            if empty == 1:
                print('block',i,'is empty')
                newdata += emptyblock
            else:
                newdata += data[start:start + 16 * 1024]
        data = newdata
        ''' this was needed as 2600+ barfed on empty pages '''
        filename = filename.rsplit( ".", 1)[0] + ".a78"
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
