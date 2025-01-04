import sys

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 filltosize.py <filename.bin> 256")
        return

    filename = sys.argv[1]

    try:
        with open(filename, 'rb') as f:
            data = f.read()
        if (len(data) == 0) or (len(data) % 1024 != 0):
            print('File is not a multiple os 1024')
            quit()
        g = open(filename, 'wb')
        for i in range(int(int(sys.argv[2]) * 1024 / len(data))):
            g.write(data)
    except FileNotFoundError:
        print(f"Error: The file '{filename}' was not found.")
    except Exception as e:
        print(f"An error occurred: {e}")

if __name__ == "__main__":
    main()
