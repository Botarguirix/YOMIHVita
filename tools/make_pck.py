"""Empaqueta una carpeta como .pck de Godot 3.x (formato v1, sin cifrar).

Uso: python make_pck.py <carpeta> <salida.pck> [version, p. ej. 3.5.1]
"""
import hashlib
import os
import struct
import sys


def make_pck(src, out, version=(3, 5, 1)):
    files = []
    for root, _, names in os.walk(src):
        for name in sorted(names):
            full = os.path.join(root, name)
            rel = os.path.relpath(full, src).replace(os.sep, "/")
            with open(full, "rb") as f:
                files.append(("res://" + rel, f.read()))

    header = b"GDPC" + struct.pack("<4I", 1, *version) + b"\0" * 64
    header += struct.pack("<I", len(files))
    dir_size = sum(4 + len(p.encode()) + 8 + 8 + 16 for p, _ in files)
    offset = len(header) + dir_size

    directory = b""
    for path, data in files:
        p = path.encode()
        directory += struct.pack("<I", len(p)) + p
        directory += struct.pack("<QQ", offset, len(data)) + hashlib.md5(data).digest()
        offset += len(data)

    with open(out, "wb") as f:
        f.write(header + directory)
        for _, data in files:
            f.write(data)
    print(f"{len(files)} archivos -> {out}")


if __name__ == "__main__":
    ver = tuple(int(x) for x in sys.argv[3].split(".")) if len(sys.argv) > 3 else (3, 5, 1)
    make_pck(sys.argv[1], sys.argv[2], ver)
