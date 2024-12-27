import sys
def main():
    if len(sys.argv) != 2:
        print("Usage: python3 build4kFill.py <filename>")
        return

    filename = sys.argv[1]

    try:
        with open(filename, 'rb') as f:
            data = f.read()
        if len(data) != 4 * 1024:
            print("Size should be", 4 * 1024, "not", len(data))
            quit()
        filename = filename.rsplit( ".", 1)[0] + ".bin"
        g = open(filename, 'wb')
        for i in range(64):
            g.write(data)
    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
