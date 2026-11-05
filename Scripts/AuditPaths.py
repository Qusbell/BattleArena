"""Resolve audit relocation aliases without rewriting historical collection evidence."""
from functools import lru_cache
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


@lru_cache(maxsize=1)
def relocation():
    path = ROOT / 'docs/repository-audit/relocation.json'
    return json.loads(path.read_text(encoding='utf-8')) if path.exists() else {'path_aliases': {}, 'files': {}}


def resolve_audit_path(value):
    path = Path(value)
    if not path.is_absolute():
        path = ROOT / path
    path = path.resolve()
    if not path.is_relative_to(ROOT):
        raise ValueError('Audit path is outside this workspace')
    relative = path.relative_to(ROOT).as_posix()
    for old, new in relocation()['path_aliases'].items():
        if relative.casefold() == old.casefold() or relative.casefold().startswith(old.casefold() + '/'):
            target = (ROOT / (new + relative[len(old):])).resolve()
            if not target.is_relative_to(ROOT):
                raise ValueError('Relocation target is outside this workspace')
            return target
    return path


def recorded_hash_matches(path, expected):
    """Accept original bytes or this recorded, exact relocation edit; reject later changes."""
    path = resolve_audit_path(path)
    actual = hashlib.sha256(path.read_bytes()).hexdigest()
    if actual == expected:
        return True
    key = path.relative_to(ROOT).as_posix().casefold()
    entry = next((v for k, v in relocation()['files'].items() if k.casefold() == key), {})
    return entry.get('sha256_before') == expected and entry.get('sha256_after') == actual
