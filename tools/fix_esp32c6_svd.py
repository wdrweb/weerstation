"""Fix Espressif ESP32-C6 SVD addressBlock sizes for the VS Code peripheral viewer.

Espressif's esp32c6.svd declares addressBlock sizes smaller than its own
registers for most peripherals (e.g. GPIO declares 0x39C but has registers up
to 0x700). mcu-debug/peripheral-viewer sizes its read buffer from the block,
so every register past the end fails with
"Internal error: Failed to update peripheral X after memory reads" even
though the GDB reads themselves succeed.

Usage:  python3 tools/fix_esp32c6_svd.py <path-to-esp32c6.svd>

Enlarges each addressBlock to cover its registers (dim-array aware) and keeps
a .orig backup. Re-running on an already-fixed file changes nothing.
"""

import shutil
import sys
import xml.etree.ElementTree as ET


def parse_int(text):
    return int(text, 0) if text is not None else None


def main(path):
    backup = path + ".orig"
    shutil.copy2(path, backup)
    print("backup:", backup)

    tree = ET.parse(path)
    root = tree.getroot()
    patched = 0
    for peripheral in root.findall("peripherals/peripheral"):
        name = peripheral.findtext("name")
        need = 0
        for reg in peripheral.iter("register"):
            offset = parse_int(reg.findtext("addressOffset"))
            size_text = reg.findtext("size")
            size = (int(size_text, 0) // 8) if size_text else 4
            dim = parse_int(reg.findtext("dim")) or 1
            inc_text = reg.findtext("dimIncrement")
            increment = int(inc_text, 0) if inc_text else size
            if offset is not None:
                need = max(need, offset + increment * (dim - 1) + size)
        if need == 0:
            print(f"{name}: no registers found, skipped (inherits via derivedFrom?)")
            continue
        for block in peripheral.findall("addressBlock"):
            offset = parse_int(block.findtext("offset"))
            size = parse_int(block.findtext("size"))
            if offset is None or size is None:
                continue
            if offset + size < need:
                old = block.findtext("size")
                block.find("size").text = hex(need - offset)
                patched += 1
                print(f"{name}: addressBlock size {old} -> {hex(need - offset)}")

    tree.write(path, encoding="utf-8", xml_declaration=True)
    print("patched blocks:", patched)


if __name__ == "__main__":
    if len(sys.argv) != 2:
        sys.exit("usage: fix_esp32c6_svd.py <path-to-esp32c6.svd>")
    main(sys.argv[1])
