#!/usr/bin/env python3
"""Emit a C++ source defining a byte array from a binary file, so it can be embedded in the executable.

Usage: embed_file.py <input> <output.cpp> <SYMBOL>
"""
import sys


def main() -> int:
    if len(sys.argv) != 4:
        sys.stderr.write("usage: embed_file.py <input> <output> <symbol>\n")
        return 2
    input_path, output_path, symbol = sys.argv[1], sys.argv[2], sys.argv[3]
    with open(input_path, "rb") as handle:
        data = handle.read()
    with open(output_path, "w", encoding="ascii") as out:
        out.write("// Generated from %s. Do not edit.\n" % input_path)
        out.write("extern const unsigned char %s[];\n" % symbol)
        out.write("extern const unsigned int %s_size;\n" % symbol)
        out.write("const unsigned char %s[] = {" % symbol)
        out.write(",".join(str(b) for b in data))
        out.write("};\n")
        out.write("const unsigned int %s_size = %du;\n" % (symbol, len(data)))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
