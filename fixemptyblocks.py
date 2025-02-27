import sys
''' Write different strings to empty blocks '''
''' It writes the patched file as filenameh.a78 '''
def main():
    if len(sys.argv) < 2:
        print("Usage: python3 fixemptyblocks.py <filename>")
        return

    filename = sys.argv[1]

    try:
        with open(filename, 'rb') as f:
            data = f.read()
        ''' scan for empty pages '''
        newdata = data[:128]
        blocknr1 = 0x30
        blocknr10 = 0x30
        for i in range(int((len(data) - 128) / (16 * 1024))):
            start = 128 + i * 16 * 1024
            val = data[start]
            empty = 1
            for j in range(16 * 1024):
                if data[start + j] != val:
                    empty = 0
            if empty == 1:
                print('block',i,'is empty')
                emptyblock = bytearray()
                for k in range(2 * 1024):
                    emptyblock.append(0x45)
                    emptyblock.append(0x4D)
                    emptyblock.append(0x50)
                    emptyblock.append(0x54)
                    emptyblock.append(0x59)
                    emptyblock.append(blocknr10)
                    emptyblock.append(blocknr1)
                    emptyblock.append(0x20)
                newdata += emptyblock
            else:
                newdata += data[start:start + 16 * 1024]
            blocknr1 += 1
            if blocknr1 > 0x39:
                blocknr1 = 0x30
                blocknr10 += 1
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
