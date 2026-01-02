import sys
import struct
import os

class FileLump:
    def __init__(self, filepos, size, name_high, name_low):
        self.filepos = filepos
        self.size = size
        self.name_high = name_high
        self.name_low = name_low

def main():
    if len(sys.argv) != 2:
        print("Usage: python wad2cc.py <wadfile>")
        sys.exit(1)

    wadfile = sys.argv[1]
    base = os.path.splitext(os.path.basename(wadfile))[0]
    ccfile = f"{base}_lumps.cc"
    hfile = f"{base}_lumps.h"
    guard = f"_{base.upper()}_LUMPS_H_"

    with open(wadfile, "rb") as f:
        header = f.read(12)
        if len(header) < 12:
            print("Invalid WAD file: header too short")
            sys.exit(1)
        ident, numlumps, infotableofs = struct.unpack('<4sii', header)
        f.seek(infotableofs)
        lumps = []
        for _ in range(numlumps):
            lumpdata = f.read(16)
            if len(lumpdata) < 16:
                print("Invalid WAD file: lump directory too short")
                sys.exit(1)
            filepos, size, name_low, name_high = struct.unpack('<iiII', lumpdata)
            lumps.append(FileLump(filepos, size, name_high, name_low))

    # Write header
    with open(hfile, "w") as h:
        h.write(f"#ifndef {guard}\n")
        h.write(f"#define {guard}\n\n")
        h.write(f"#include <stdint.h>\n\n")
        h.write(f"#define WADLUMPS {numlumps}\n\n")
        h.write(f"extern int32_t filepos[WADLUMPS];\n")
        h.write(f"extern int32_t lumpsize[WADLUMPS];\n")
        h.write(f"extern uint32_t lumpname_high[WADLUMPS];\n")
        h.write(f"extern uint32_t lumpname_low[WADLUMPS];\n\n")
        h.write(f"#endif // {guard}\n")

    # Write .cc file
    with open(ccfile, "w") as cc:
        cc.write(f'#include "{hfile}"\n')
        cc.write('#include "annotations.h"\n\n')
        cc.write(f"int32_t CONSTMEM filepos[WADLUMPS] = {{\n")
        for lump in lumps:
            cc.write(f"    {lump.filepos},\n")
        cc.write("};\n\n")
        cc.write(f"int32_t CONSTMEM lumpsize[WADLUMPS] = {{\n")
        for lump in lumps:
            cc.write(f"    {lump.size},\n")
        cc.write("};\n\n")
        cc.write(f"uint32_t CONSTMEM lumpname_high[WADLUMPS] = {{\n")
        for lump in lumps:
            cc.write(f"    0x{lump.name_high:08x},\n")
        cc.write("};\n\n")
        cc.write(f"uint32_t CONSTMEM lumpname_low[WADLUMPS] = {{\n")
        for lump in lumps:
            cc.write(f"    0x{lump.name_low:08x},\n")
        cc.write("};\n")

if __name__ == "__main__":
    main()
