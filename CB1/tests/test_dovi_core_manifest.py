# SPDX-License-Identifier: MIT
"""The dovi_core port must stay registered in every manifest that gates a build."""
import hashlib
import json
import pathlib
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
REPO = ROOT.parent
CORE = ROOT / "src/dovi_core"
PATCH = REPO / "projects/Generic/patches/kodi/kodi-9999-zzzz-dovi-mode.patch"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


class DoviCoreManifest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.owned = json.loads((ROOT / "config/owned-sources.json").read_text())
        cls.lock = json.loads((REPO / "config/cb1-source-lock.json").read_text())

    def test_every_source_is_owned_and_staged(self):
        files = sorted(p.name for p in CORE.iterdir() if p.is_file())
        self.assertGreaterEqual(len(files), 120)
        for name in files:
            key = "src/dovi_core/" + name
            self.assertIn(key, self.owned, key)
            self.assertEqual(self.owned[key], sha(CORE / name), key)
            dest = "packages/mediacenter/kodi/cb1/src/dovi_core/" + name
            self.assertIn(dest, self.lock["staging"], dest)
            record = self.lock["staging"][dest]
            self.assertEqual(record["source"], key)
            self.assertEqual(record["sha256"], self.owned[key])

    def test_engine_bridge_is_built(self):
        cmake = (ROOT / "src/CMakeLists.txt").read_text()
        self.assertIn("dvbridge_dovi.c", cmake)
        self.assertIn("dovi_core/dovi_engine.c", cmake)
        for name in ("dvbridge_dovi.c", "dvbridge_dovi.h"):
            key = "src/" + name
            self.assertIn(key, self.owned, key)
            self.assertIn(key, {r["source"] for r in self.lock["staging"].values()})

    def test_kodi_patch_registers_the_switch(self):
        self.assertTrue(PATCH.is_file(), PATCH)
        key = "projects/Generic/patches/kodi/kodi-9999-zzzz-dovi-mode.patch"
        self.assertIn(key, self.lock["integration_files"])
        self.assertEqual(self.lock["integration_files"][key], sha(PATCH))
        text = PATCH.read_text()
        for token in ('id="dvbridge.dovimode"', "DVBRIDGE_ENGINE_DOVI", 'GetBool("dvbridge.dovimode")',
                      'msgctxt "#40906"', 'msgctxt "#40907"'):
            self.assertIn(token, text, token)


if __name__ == "__main__":
    unittest.main()
