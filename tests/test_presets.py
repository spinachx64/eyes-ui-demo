"""Expression additions must reach both firmware targets without hand edits."""
import copy
import importlib.util
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('generate_presets', ROOT / 'tools/generate_presets.py')
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)


class PresetTests(unittest.TestCase):
    def setUp(self):
        self.settings = json.loads((ROOT / 'presets.json').read_text())

    def test_add_expression_and_escape_strings(self):
        extra = copy.deepcopy(self.settings['emotions'][0])
        extra.update(id='new-expression', name='test "quote"\\backslash')
        self.settings['emotions'].append(extra)
        result = generator.generate(self.settings)
        self.assertIn('"new-expression"', result)
        self.assertIn('test \\"quote\\"\\\\backslash', result)
        self.assertEqual(result.count(' },'), len(self.settings['emotions']))

    def test_duplicate_id_rejected(self):
        self.settings['emotions'].append(copy.deepcopy(self.settings['emotions'][0]))
        with self.assertRaises(ValueError):
            generator.generate(self.settings)

    def test_invalid_pose_rejected(self):
        for value in (float('nan'), float('inf'), -1):
            self.settings['emotions'][0]['left']['w'] = value
            with self.assertRaises(ValueError):
                generator.generate(self.settings)

    def test_generated_headers_match_source(self):
        result = generator.generate(self.settings)
        for path in ('components/eyes/src/presets.h', 'firmware/eyes_demo/presets.h'):
            self.assertEqual((ROOT / path).read_text(), result)


if __name__ == '__main__':
    unittest.main()
