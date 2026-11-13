
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
            "cheats": "false",
            "input_descriptors": "true",
            "memory_descriptors": "false",
            "libretro_saves": "false",
            "core_options": "true",
            "load_subsystem": "false",
            "hw_render": "false",
            "disk_control": "false",
        }
        for key, value in expected.items():
            with self.subTest(field=key):
                self.assertEqual(fields.get(key), value)
        description = fields.get("description", "")
        for phrase in ("15 playable Color TV-Game 6/15 variants", "Racing 112 prototype",
                       "not all 112 combinations", "not verified original hardware"):
            with self.subTest(description=phrase):
                self.assertIn(phrase, description)
        notes = fields.get("notes", "")
        for phrase in ("No content is required", "game selectors, not ROMs", "XRGB8888",
                       "libretro memory saves are not supported", "frontend save directory",
                       "when writable", "remain in memory for the current session"):
            with self.subTest(notes=phrase):
                self.assertIn(phrase, notes)


if __name__ == "__main__":
    unittest.main()
