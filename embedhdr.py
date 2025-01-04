import sys
''' Ember header in the signature space '''
def main():
    if len(sys.argv) < 2:
        print("Usage: python3 embedhdr.py <filename>")
        return

    filename = sys.argv[1]

    try:
        with open(filename, 'rb') as f:
            data = f.read()
        ''' embed header into signature space '''
        hdr = data[:120]
        vectors = data[-8:]
        newdata = data[:-128]
        newdata += hdr
        newdata += vectors
        data = newdata
        filename = filename.rsplit( ".", 1)[0] + "h.a78"
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
