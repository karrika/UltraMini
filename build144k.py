import sys
def main():
    if len(sys.argv) != 2:
        print("Usage: python3 build144k.py <filename>")
        return

    filename = sys.argv[1]

    try:
        with open(filename, 'rb') as f:
            data = f.read()
        if len(data) != (128 + 16) * 1024 + 128:
            print("Size should be", (128 + 16) * 1024 + 128, "not", len(data))
            quit()
        filename = filename.rsplit( ".", 1)[0] + ".bin"
        print(filename)
        g = open(filename, 'wb')
        g.write(data[128 + 16 * 1024:])
        for i in range(8):
            g.write(data[128:128 + 16 * 1024])
    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
