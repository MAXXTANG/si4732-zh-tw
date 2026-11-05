#!/usr/bin/env python3
"""Package a clean 0.2.1 build and check its flash layout before distribution."""
import argparse
from hashlib import sha256
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile, ZipInfo

ROOT = Path(__file__).resolve().parents[1]
NAME = "Si4732Traditional.ino"
PARTS = {
    f"{NAME}.bootloader.bin": 0x0000,
    f"{NAME}.partitions.bin": 0x8000,
    "boot_app0.bin": 0xE000,
    f"{NAME}.bin": 0x10000,
}
MERGED = f"{NAME}.merged.bin"
ARCHIVE = "si4732-zh-tw-0.2.1-firmware.zip"
NOTICES = [
    "THIRD_PARTY_NOTICES.md",
    "licenses/Arduino-ESP32-LICENSE.md",
    "licenses/ESP-IDF-LICENSE.txt",
    "licenses/Noto-CJK-OFL.txt",
    "vendor/TFT_eSPI/license.txt",
    "vendor/PU2CLR_SI4735/license.txt",
]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / ".dev/build")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "dist")
    args = parser.parse_args()
    images = {name: (args.build_dir / name).read_bytes() for name in [*PARTS, MERGED]}
    expected = bytearray(b"\xff" * (16 * 1024 * 1024))
    for name, offset in PARTS.items():
        data = images[name]
        expected[offset:offset + len(data)] = data
    if expected != images[MERGED]:
        raise ValueError("Merged image must contain only build artifacts and erased space.")
    if any(marker in expected for marker in [b"/Users/", b"/home/", b"\\Users\\"]):
        raise ValueError("Personal build paths found. Rebuild using build.sh before publishing.")

    files = dict(images)
    files["README.md"] = (ROOT / "FIRMWARE_README.md").read_bytes()
    files["SHA256SUMS"] = "".join(f"{sha256(data).hexdigest()}  {name}\n"
                                  for name, data in images.items()).encode()
    files["flash_args"] = (args.build_dir / "flash_args").read_bytes()
    files.update({name: (ROOT / name).read_bytes() for name in NOTICES})
    args.output_dir.mkdir(parents=True, exist_ok=True)
    archive = args.output_dir / ARCHIVE
    with ZipFile(archive, "w") as package:
        for name, data in files.items():
            entry = ZipInfo(name)  # fixed timestamp for identical input
            entry.external_attr = 0o100644 << 16
            package.writestr(entry, data, compress_type=ZIP_DEFLATED)
    with ZipFile(archive) as package:
        if package.testzip() or any(package.read(n) != content for n, content in files.items()):
            raise ValueError("ZIP verification failed.")
    digest = sha256(archive.read_bytes()).hexdigest()
    (archive.parent / (ARCHIVE + ".sha256")).write_text(f"{digest}  {ARCHIVE}\n")
    print(f"{digest}  {archive.name}")


if __name__ == "__main__":
    main()
