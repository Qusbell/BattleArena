"""Extract standalone diagrams and check local links, pin snapshots and diagram structure."""
from pathlib import Path
import json
import re
from collections import Counter
import hashlib

DOC = Path(__file__).resolve().parents[1]
ROOT = DOC.parents[1]
DIAGRAMS = DOC / 'Diagrams'
DIAGRAMS.mkdir(exist_ok=True)

def label(s):
    import html
    return html.escape(s,quote=True).replace('\n','<br/>')

for file in [DOC / 'README.md', DOC / 'RefactoringBoundaries.md', *sorted(DOC.glob('[0-9][0-9]_*.md'))]:
    for i,block in enumerate(re.findall(r'```mermaid\n(.*?)\n```',file.read_text(encoding='utf-8'),re.S),1):
        (DIAGRAMS / f'{file.stem}-{i:02d}.mmd').write_text(block+'\n',encoding='utf-8')

tree = json.loads((DOC / 'Evidence/behavior-tree.json').read_text(encoding='utf-8'))
lines=['flowchart TD']
index=0
def add_tree(n,parent=None,order=None):
    global index
    current=f'bt{index}'
    index+=1
    name=n['node_name']+'\n'+n['node_class']
    if n.get('decorators'):name+='\n조건: '+', '.join(d['name'] for d in n['decorators'])
    if n.get('services'):name+='\nService: '+', '.join(d['name'] for d in n['services'])
    lines.append(f'  {current}["{label(name)}"]')
    if parent:lines.append(f'  {parent} -->|"{order}"| {current}')
    for i,c in enumerate(n.get('children',[]),1):add_tree(c,current,i)
add_tree(tree['root'])
(DIAGRAMS / 'behavior-tree.mmd').write_text('\n'.join(lines)+'\n',encoding='utf-8')

bad_links=[]
diagrams=0
bad_mermaid=[]
for file in sorted((ROOT / 'Docs').rglob('*.md')):
    text=file.read_text(encoding='utf-8')
    for target in re.findall(r'\[[^\]\n]+\]\(([^)\n]+)\)',text):
        target=target.split('#')[0]
        if not target or target.startswith(('https:','http:','codex:','app:')):continue
        if not (file.parent / target).exists():bad_links.append({'file':str(file.relative_to(ROOT)),'target':target})
    for block in re.findall(r'```mermaid\n(.*?)\n```',text,re.S):
        diagrams+=1
        without_labels=re.sub(r'"[^"]*"', '""', block)
        defined=set(re.findall(r'\b([A-Za-z][A-Za-z0-9_]*)[\[{]',without_labels))
        referenced=set()
        for line in block.splitlines():
            if '-->' in line or '-.->' in line:
                first=re.match(r'\s*([A-Za-z][A-Za-z0-9_]*)',line)
                if first:referenced.add(first[1])
                dest=re.search(r'(?:-->|-\.->)(?:\|[^\n]*?\|)?\s*([A-Za-z][A-Za-z0-9_]*)',line)
                if dest:referenced.add(dest[1])
        missing=referenced-defined
        if missing:bad_mermaid.append({'file':str(file.relative_to(ROOT)),'undefined_nodes':sorted(missing)})

bad_pins=[]
for file in sorted((DOC / 'Evidence').glob('blueprints-*.json')):
    data=json.loads(file.read_text(encoding='utf-8'))
    for bp in data['blueprints']:
        for g in bp['graphs']:
            names={n['node_name'] for n in g['nodes']}
            if len(g['nodes'])!=g['node_count']:bad_pins.append([bp['path'],g['graph_name'],'count mismatch'])
            for n in g['nodes']:
                for p in n['pins']:
                    for link in p.get('linked_to',[]):
                        if link['node_name'] not in names:bad_pins.append([bp['path'],g['graph_name'],link['node_name']])

hashes={}
for folder in ['Source','Config']:
    for file in sorted((ROOT / folder).rglob('*')):
        if file.is_file() and file.suffix in ['.h','.cpp','.cs','.ini']:
            hashes[file.relative_to(ROOT).as_posix()]=hashlib.sha256(file.read_bytes()).hexdigest()
(DOC / 'Evidence/source-hashes.json').write_text(json.dumps(hashes,indent=2),encoding='utf-8')

result={'markdown_files':len(list((ROOT/'Docs').rglob('*.md'))),'embedded_mermaid_diagrams':diagrams,
        'standalone_mmd_files':len(list(DIAGRAMS.glob('*.mmd'))),
        'broken_local_links':bad_links,'undefined_mermaid_nodes':bad_mermaid,'snapshot_node_count_or_link_errors':bad_pins,
        'mermaid_check':'structural: declarations and edge endpoints; no visual rendering confirmed',
        'runtime_tests':'not run; documentation-only audit'}
(DOC / 'Evidence/validation.json').write_text(json.dumps(result,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(result,ensure_ascii=False))
if bad_links or bad_mermaid or bad_pins:raise SystemExit(1)
