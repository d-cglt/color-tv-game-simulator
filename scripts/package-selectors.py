#!/usr/bin/env python3
"""Build a reproducible selector archive using the core's generator."""
import argparse
from pathlib import Path
import subprocess
import tempfile
import zipfile


def build(generator, readme, output):
    output = Path(output).resolve()
    # Keep all temporary files alongside the output, and replace only on success.
    with tempfile.TemporaryDirectory(prefix=".ctg-package-", dir=output.parent) as tmp:
        tmp = Path(tmp)
        subprocess.run([str(Path(generator).resolve()), str(tmp)], check=True)
        selectors = tmp / "Nintendo Color TV-Game"
        files = sorted(selectors.iterdir(), key=lambda path: path.name)
        if len(files) != 16 or any(not p.is_file() or p.suffix != ".ctg" for p in files):
            raise ValueError("generator did not produce exactly 16 selectors")
        archive = tmp / "selectors.zip"
        with zipfile.ZipFile(archive, "w", compression=zipfile.ZIP_STORED) as z:
            for name, data in [("README.md", Path(readme).read_bytes())] + [
                (p.name, p.read_bytes()) for p in files
            ]:
                info = zipfile.ZipInfo(name, (1980, 1, 1, 0, 0, 0))
                info.create_system = 3
                info.external_attr = 0o100644 << 16
                info.compress_type = zipfile.ZIP_STORED
                z.writestr(info, data)
        archive.replace(output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("generator", help="path to the generate_selectors executable")
    parser.add_argument("readme", help="path to the package README")
    parser.add_argument("output", help="output ZIP (parent directory must exist)")
    args = parser.parse_args()
    build(args.generator, args.readme, args.output)


if __name__ == "__main__":
    main()
