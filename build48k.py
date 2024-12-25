import sys
def main():
    if len(sys.argv) != 2:
        print("Usage: python3 build48k.py <filename>")
        return

    filename = sys.argv[1]

    try:
        with open(filename, 'rb') as file:
            file.read(128)
            b0 = file.read(16 * 1024)
            b1 = file.read(16 * 1024)
            b2 = file.read(16 * 1024)

        rom = bytearray([255] * 16 * 1024)
        rom = rom + b0 + b1 + b2
        filename = filename.rsplit( ".", 1)[0] + ".bin"
        with open(filename, 'wb') as g:
            g.write(rom)
    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
