#!/usr/bin/env python3
"""Junta translation/lotes/*.tsv (indice<TAB>texto) em translation/texts_pt.json.
Textos EN identicos recebem a mesma traducao (so para indices ja cobertos pelos lotes).
Valida codigos de controle. Uso: python tools/merge_lotes.py"""
import json, glob, re, collections, os
R = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
en = json.load(open(R + '/translation/texts_en.json', encoding='utf-8'))
pt = {}
for f in sorted(glob.glob(R + '/translation/lotes/*.tsv')):
    for ln in open(f, encoding='utf-8'):
        ln = ln.rstrip('\n')
        if ln:
            k, v = ln.split('\t', 1)
            assert k in en, (f, k)
            pt[k] = v
top = max(map(int, pt))
m = {}
for k, v in pt.items(): m.setdefault(en[k].strip(), v)
for k, e in en.items():
    if k not in pt and int(k) <= top and e.strip() in m: pt[k] = m[e.strip()]
C = re.compile(r'[$&~@%][A-Za-z0-9]')
bad = [k for k in pt if collections.Counter(C.findall(en[k])) != collections.Counter(C.findall(pt[k]))]
json.dump(dict(sorted(pt.items(), key=lambda x: int(x[0]))), open(R + '/translation/texts_pt.json', 'w', encoding='utf-8'), ensure_ascii=False, indent=0)
print(len(pt), 'de', len(en), 'textos;', len(bad), 'com codigos de controle diferentes', bad[:10])
