"""Run via Unreal MCP. Validate authoritative String Table + PO, then compile standard locres.

No seed is read here. Missing/fuzzy/stale translations and mismatched named arguments
are editor errors and prevent any output from being compiled.
"""
import ast
import json
import os
import tempfile
import re
from pathlib import Path
import unreal

TABLE = '/Game/QuakeLike_1_0/Localization/ST_Game.ST_Game'


def read_po(path):
    entries, current, field = [], {}, None
    fuzzy = False
    for number, raw in enumerate(path.read_text(encoding='utf-8-sig').splitlines() + [''], 1):
        line = raw.strip()
        if not line:
            if 'msgctxt' in current:
                if 'msgid' not in current or 'msgstr' not in current:
                    raise ValueError(f'{path.name}:{number}: incomplete PO entry')
                entries.append({**current, 'fuzzy': fuzzy})
            current, field, fuzzy = {}, None, False
        elif line.startswith('#,'):
            fuzzy = fuzzy or 'fuzzy' in [flag.strip() for flag in line[2:].split(',')]
        elif line.startswith('#'):
            continue
        else:
            try:
                if line.startswith(('msgctxt ', 'msgid ', 'msgstr ')):
                    field, value = line.split(' ', 1)
                    if field in current:
                        raise ValueError(f'duplicate {field}')
                    value = ast.literal_eval(value)
                    if not isinstance(value, str):
                        raise ValueError('expected quoted string')
                    current[field] = value
                elif line.startswith('"') and field:
                    value = ast.literal_eval(line)
                    if not isinstance(value, str):
                        raise ValueError('expected quoted string')
                    current[field] += value
                else:
                    raise ValueError('unsupported PO syntax')
            except (ValueError, SyntaxError) as error:
                raise ValueError(f'{path.name}:{number}: {error}') from error
    return entries


def validate_po(entries, sources, culture):
    errors, translations = [], {}
    for entry in entries:
        namespace, _, key = entry.get('msgctxt', '').partition(',')
        if namespace != 'Game' or key not in sources:
            continue
        if key in translations:
            errors.append(f'{culture}/{key}: duplicate entry')
        value = entry.get('msgstr', '')
        if entry['msgid'] != sources[key]:
            errors.append(f'{culture}/{key}: source changed; review the translation')
        if entry.get('fuzzy') or not value:
            errors.append(f'{culture}/{key}: missing or fuzzy translation')
        args = lambda text: set(re.findall(r'(?<!\{)\{([^{}]+)\}(?!\})', text))
        if args(value) != args(sources[key]):
            errors.append(f'{culture}/{key}: format arguments differ')
        translations[key] = value
    for key in sources.keys() - translations.keys():
        errors.append(f'{culture}/{key}: missing translation')
    return errors, translations


def _stage_bytes(folder, data):
    descriptor, filename = tempfile.mkstemp(prefix='.Game-', suffix='.tmp', dir=folder)
    path = Path(filename)
    try:
        with os.fdopen(descriptor, 'wb') as stream:
            stream.write(data)
            stream.flush()
            os.fsync(stream.fileno())
    except Exception:
        path.unlink(missing_ok=True)
        raise
    return path


def publish_resources(resources, directory):
    """Stage every culture first; atomically replace each file and roll back on failure."""
    staged, backups, published, preserve = {}, {}, [], set()
    try:
        for culture, data in resources.items():
            folder = directory / culture
            folder.mkdir(parents=True, exist_ok=True)
            target = folder / 'Game.locres'
            backups[target] = _stage_bytes(folder, target.read_bytes()) if target.exists() else None
            staged[target] = _stage_bytes(folder, data)
        for target, temporary in staged.items():
            os.replace(temporary, target)
            published.append(target)
    except Exception as error:
        recovery = []
        for target in reversed(published):
            try:
                if backups[target] is None:
                    target.unlink(missing_ok=True)
                else:
                    os.replace(backups[target], target)
            except OSError as rollback_error:
                if backups[target] is not None:
                    preserve.add(backups[target])
                recovery.append(f'{target}: backup={backups[target]}; {rollback_error}')
        if recovery:
            raise RuntimeError('Could not restore resources; keep recovery backups: ' + '; '.join(recovery)) from error
        raise
    finally:
        for temporary in [*staged.values(), *backups.values()]:
            if temporary is not None and temporary not in preserve:
                temporary.unlink(missing_ok=True)


def _fail(errors):
    for error in errors:
        unreal.log_error('[Language] ' + error)
    raise RuntimeError(f'Language validation/compilation failed with {len(errors)} errors; existing locres preserved')


def compile_all():
    root = Path(unreal.Paths.project_dir()).resolve()
    table = unreal.load_asset(TABLE)
    if table is None or table.get_class().get_path_name() != '/Script/Engine.StringTable':
        _fail(['ST_Game is missing or has the wrong asset type'])
    sources = {str(k): unreal.StringTableLibrary.get_table_entry_source_string(TABLE, k)
               for k in unreal.StringTableLibrary.get_keys_from_string_table(TABLE)}
    if not sources:
        _fail(['String Table is empty'])
    dt = unreal.load_asset('/Game/QuakeLike_1_0/Localization/DT_Language')
    row_type = dt.get_editor_property('row_struct') if dt is not None and dt.get_class().get_path_name() == '/Script/Engine.DataTable' else None
    if row_type is None or row_type.get_path_name() != '/Script/ShootingArena.LanguageRow':
        _fail(['DT_Language is missing or has the wrong row type'])
    rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(dt))
    enabled = [r for r in rows if r['IsEnabled']]
    errors = []
    if sum(bool(r['IsDefault']) for r in enabled) != 1:
        errors.append('DT_Language: exactly one enabled default language is required')
    codes = [r['CultureCode'].casefold() for r in rows]
    if len(codes) != len(set(codes)):
        errors.append('DT_Language: duplicate CultureCode')
    outputs = {}
    for row in enabled:
        culture = row['CultureCode']
        if not re.fullmatch('[a-z]{2,3}(?:-[A-Za-z0-9]+)*', culture):
            errors.append(f'DT_Language: invalid CultureCode {culture!r}')
            continue
        if row['Font'] in ['', 'None']:
            errors.append(f'DT_Language/{culture}: missing Font')
        po = root / 'Content/Localization/Game' / culture / 'Game.po'
        try:
            failures, translations = validate_po(read_po(po), sources, culture)
        except (OSError, ValueError, UnicodeError) as error:
            errors.append(f'{culture}: {error}')
            continue
        errors.extend(failures)
        outputs[culture] = translations
    if errors:
        _fail(errors)
    resources = {}
    for culture, translations in outputs.items():
        data = unreal.LanguageAuthoringLibrary.build_localization_resource(culture, translations)
        if data is None or not data:
            errors.append(f'{culture}: engine format validation or resource generation failed')
        else:
            resources[culture] = bytes(data)
    if errors:
        _fail(errors)
    try:
        publish_resources(resources, root / 'Content/Localization/Game')
    except (OSError, RuntimeError) as error:
        unreal.log_error(f'[Language] Could not publish localization resources: {error}')
        raise
    return {'entries': len(sources), 'cultures': list(outputs), 'validation': 'passed'}
