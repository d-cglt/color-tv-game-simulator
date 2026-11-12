import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
import zipfile

GENERATOR = Path(sys.argv[1]).resolve()
ROOT = Path(sys.argv[2]).resolve()
BUILD = Path(sys.argv[3]).resolve()
del sys.argv[1:]
PACKAGER = ROOT / "scripts/package-selectors.py"
README = ROOT / "docs/SELECTORS_README.md"


class SelectorPackageTests(unittest.TestCase):
    def test_payload_and_reproducibility(self):
        with tempfile.TemporaryDirectory(prefix="selector-test-", dir=BUILD) as d:
            root = Path(d)
            staging = root / "system with spaces"
            subprocess.run([str(GENERATOR), str(staging)], check=True)
            generated = staging / "Nintendo Color TV-Game"
            expected = {p.name: p.read_bytes() for p in generated.iterdir()}
            self.assertEqual(len(expected), 16)
            self.assertEqual({b[4] for b in expected.values()}, set(range(16)))
            for data in expected.values():
                self.assertEqual(len(data), 5)
                self.assertEqual(data[:4], b"CTG1")
            self.assertEqual(expected["Racing 112 (Color TV Game Racing 112).ctg"], b"CTG1\x0f")
            outputs = [root / "first archive.zip", root / "second.zip"]
            for output, tz in zip(outputs, ("UTC", "Pacific/Honolulu")):
                subprocess.run([sys.executable, str(PACKAGER), str(GENERATOR), str(README), str(output)],
                               check=True, env={**os.environ, "TZ": tz})
            self.assertEqual(outputs[0].read_bytes(), outputs[1].read_bytes())
            with zipfile.ZipFile(outputs[0]) as z:
                self.assertEqual(z.namelist(), ["README.md"] + sorted(expected))
                self.assertEqual(z.read("README.md"), README.read_bytes())
                for name, data in expected.items():
                    self.assertEqual(z.read(name), data)
                for info in z.infolist():
                    self.assertEqual(info.date_time, (1980, 1, 1, 0, 0, 0))
                    self.assertEqual(info.compress_type, zipfile.ZIP_STORED)
                    self.assertEqual(info.external_attr >> 16, 0o100644)

    def test_failure_preserves_existing_archive(self):
        with tempfile.TemporaryDirectory(prefix="selector-test-", dir=BUILD) as d:
            root = Path(d)
            output = root / "existing.zip"
            output.write_bytes(b"keep me")
            result = subprocess.run([sys.executable, str(PACKAGER), str(GENERATOR),
                                     str(root / "missing-readme"), str(output)],
                                    stdout=subprocess.PIPE, stderr=subprocess.PIPE)
            self.assertNotEqual(result.returncode, 0)
            self.assertEqual(output.read_bytes(), b"keep me")
            self.assertEqual(list(root.iterdir()), [output])


if __name__ == "__main__":
    unittest.main()
