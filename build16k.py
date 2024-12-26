import sys
def main():
    if len(sys.argv) != 2:
        print("Usage: python3 build16k.py <filename>")
        return

    filename = sys.argv[1]

    try:
        with open(filename, 'rb') as f:
            data = f.read()
        if len(data) != 16 * 1024 + 128:
            print("Size should be", 16 * 1024 + 128, "not", len(data))
            quit()
        filename = filename.rsplit( ".", 1)[0] + ".bin"
        g = open(filename, 'wb')
        g.write(data[128:])
    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
