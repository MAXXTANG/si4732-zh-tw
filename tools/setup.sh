#!/bin/bash
set -euo pipefail
PROJECT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
python3 - "$PROJECT_DIR" <<'PY'
from pathlib import Path
import json
import os
import shutil
import sys
root = Path(sys.argv[1])
data = root / '.dev/data'
for name in ['data', 'downloads', 'user', 'cache']:
    (root / '.dev' / name).mkdir(parents=True, exist_ok=True)
# Optionally reuse an installed SDK, without copying or changing that SDK.
reuse = os.environ.get('ARDUINO_CORE_DATA')
if reuse and not (data / 'packages').exists():
    source = Path(reuse).expanduser().resolve()
    assert (source / 'packages').is_dir(), source
    (data / 'packages').symlink_to(source / 'packages', target_is_directory=True)
    for p in source.glob('*.json*'):
        shutil.copy2(p, data / p.name)
config = {'directories': {n: str(root / '.dev' / n) for n in ['data', 'downloads', 'user']},
          'build_cache': {'path': str(root / '.dev/cache')},
          'board_manager': {'additional_urls': ['https://espressif.github.io/arduino-esp32/package_esp32_index.json']}}
# JSON is a valid YAML subset accepted by arduino-cli.
(root / '.dev/arduino-cli.yaml').write_text(json.dumps(config, ensure_ascii=False, indent=2)+'\n')
PY
if [ ! -d "$PROJECT_DIR/.dev/data/packages/esp32/hardware/esp32/3.3.12" ]; then
  arduino-cli --config-file "$PROJECT_DIR/.dev/arduino-cli.yaml" core update-index
  arduino-cli --config-file "$PROJECT_DIR/.dev/arduino-cli.yaml" core install esp32:esp32@3.3.12
fi
arduino-cli --config-file "$PROJECT_DIR/.dev/arduino-cli.yaml" core list
