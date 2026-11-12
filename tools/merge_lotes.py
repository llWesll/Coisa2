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
# --- Quebra de linha manual ---------------------------------------------
# O jogo quebra palavras no meio quando o texto tem acento. Para evitar,
# inserimos &N (quebra de linha) entre palavras nos textos de dialogo.
WRAP_W = 52          # largura maxima (caracteres visiveis) por linha
WRAP_FROM = 4028     # a partir deste indice (dialogos)
WRAP_OVERRIDE = {3945: 20}   # larguras especiais (caixa estreita)
CODE = re.compile(r'[$&~@%][A-Za-z0-9]')
def vis(s): return len(CODE.sub('', s))
def wrap_seg(seg, w):
    if vis(seg) <= w: return seg
    out, cur, curlen = [], [], 0
    for word in seg.split(' '):
        wl = vis(word)
        if cur and curlen + 1 + wl > w:
            out.append(' '.join(cur)); cur, curlen = [word], wl
        else:
            curlen = curlen + (1 if cur else 0) + wl; cur.append(word)
    out.append(' '.join(cur))
    return '&N'.join(out)
def wrap(text, w):
    parts = re.split(r'(&N|&P)', text)
    return ''.join(p if p in ('&N', '&P') else wrap_seg(p, w) for p in parts)
wrapped = 0
for k in list(pt):
    i = int(k)
    w = WRAP_OVERRIDE.get(i, WRAP_W if i >= WRAP_FROM else None)
    if w:
        new = wrap(pt[k], w)
        if new != pt[k]: pt[k] = new; wrapped += 1
print(wrapped, 'textos com quebra manual inserida')

C = re.compile(r'[$&~@%][A-Za-z0-9]')
def codes(t): return collections.Counter(c for c in C.findall(t) if c != '&N')
bad = [k for k in pt if codes(en[k]) != codes(pt[k])]
json.dump(dict(sorted(pt.items(), key=lambda x: int(x[0]))), open(R + '/translation/texts_pt.json', 'w', encoding='utf-8'), ensure_ascii=False, indent=0)
print(len(pt), 'de', len(en), 'textos;', len(bad), 'com codigos de controle diferentes', bad[:10])
