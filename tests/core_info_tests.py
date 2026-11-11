
"""Check the standalone libretro metadata; no frontend is required."""

import pathlib
import re
import unittest


class CoreInfoTests(unittest.TestCase):
    def test_metadata(self):
        info = pathlib.Path(__file__).resolve().parent.parent / "colortvgame_libretro.info"
        fields = {}
        for number, line in enumerate(info.read_text(encoding="utf-8").splitlines(), 1):
            if not line.strip() or line.lstrip().startswith("#"):
                continue
            match = re.fullmatch(r'\s*([a-z_][a-z_0-9]*)\s*=\s*"([^"\r\n]*)"\s*', line)
            if match is None:
                self.fail(f"Invalid metadata on line {number}: {line}")
            key, value = match.groups()
            self.assertNotIn(key, fields, f"Duplicate metadata field: {key}")
            fields[key] = value

        expected = {
            "display_name": "Color TV-Game Simulator",
            "license": "MIT",
            "supported_extensions": "ctg",
            "supports_no_game": "true",
            "needs_fullpath": "false",
            "savestate": "false",
            "core_options": "true",
        }
        for key, value in expected.items():
            with self.subTest(field=key):
                self.assertEqual(fields.get(key), value)
        self.assertTrue(fields.get("description"))
        self.assertTrue(fields.get("notes"))


if __name__ == "__main__":
    unittest.main()
