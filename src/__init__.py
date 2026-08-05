"""Configure local runtime caches before importing the LLM stack."""

from __future__ import annotations

import os
from pathlib import Path


_PROJECT_ROOT = Path(__file__).resolve().parent.parent
_LOCAL_HF_HOME = _PROJECT_ROOT / ".cache" / "huggingface"
_LOCAL_TMP_DIR = _PROJECT_ROOT / ".tmp"

if "HF_HOME" not in os.environ:
    _LOCAL_HF_HOME.mkdir(parents=True, exist_ok=True)
    os.environ["HF_HOME"] = str(_LOCAL_HF_HOME)

if "HF_HUB_CACHE" not in os.environ:
    _hf_home = Path(os.environ["HF_HOME"]).expanduser()
    os.environ["HF_HUB_CACHE"] = str(_hf_home / "hub")

if "TMPDIR" not in os.environ:
    _LOCAL_TMP_DIR.mkdir(parents=True, exist_ok=True)
    os.environ["TMPDIR"] = str(_LOCAL_TMP_DIR)
