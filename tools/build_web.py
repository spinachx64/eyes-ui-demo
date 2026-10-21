#!/usr/bin/env python3
"""Embed shared JSON presets into a standalone offline web demo."""
import json
from pathlib import Path
r=Path(__file__).resolve().parent.parent
s=json.loads((r/'presets.json').read_text(encoding='utf-8'))
t=(r/'web/index.template.html').read_text(encoding='utf-8')
(r/'web/index.html').write_text(t.replace('__PRESETS_JSON__',json.dumps(s,ensure_ascii=False,separators=(',',':'))),encoding='utf-8')
print('Generated web/index.html')
