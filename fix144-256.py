import sys

def main():
    if len(sys.argv) != 2:
        print("Usage: python fix144-256.py <filename>")
        return

    filename = sys.argv[1]

    try:
        with open(filename, 'rb') as file:
            data = file.read()
            data = data[128:]
            bank4000 = data[:16 * 1024]

            rom = bytearray([255] * 256 * 1024)  # Initialize rom with 256 KB of 255

            # copy banked rom starting at 0
            rom[0:128 * 1024] = data[16 * 1024:]
            # copy banked rom starting at block 128
            rom[128 * 1024:] = data[16 * 1024:]
            # insert bank for 4000 at top - 1
            rom[-32 * 1024:-16 * 1024] = bank4000[:]

        with open('rom.bin', 'wb') as g:
            g.write(rom)
    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()

