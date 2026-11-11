#!/usr/bin/env python3
"""Aplica a traducao no APK mesclado.
Uso: apply_translation.py <apk> <texts_pt.json> <idioma: en|fr|de|jp|zhhans|zhhant>
Troca assets/common/game_res/memorytext_<idioma>.dat.jpg. Indices ausentes no JSON ficam no texto original."""
import sys, os, re, json, zipfile, collections
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import mt_tool

apk, tjson, lang = sys.argv[1:4]
name = 'assets/common/game_res/memorytext_%s.dat.jpg' % lang
T = json.load(open(tjson, encoding='utf-8'))
if not T:
    sys.exit('traducao vazia: ' + tjson)

CTRL = re.compile(r'[$&~@%][A-Za-z0-9]')
tmp_in, tmp_out = '/tmp/mt_orig.dat.jpg', '/tmp/mt_new.dat.jpg'
with zipfile.ZipFile(apk) as z:
    info = z.getinfo(name)
    open(tmp_in, 'wb').write(z.read(name))

S = mt_tool.read_dat(tmp_in)
bad = hard = 0
for k, v in T.items():
    i = int(k)
    if i < 0 or i >= len(S):
        sys.exit('indice fora do arquivo: %s' % k)
    if '\x00' in v:
        sys.exit('texto com caractere nulo no indice %s' % k)
    orig = S[i][:-1].decode('utf-8')
    a, b = collections.Counter(CTRL.findall(orig)), collections.Counter(CTRL.findall(v))
    if a != b:
        bad += 1
        fmt = lambda c: sum(n for t, n in c.items() if t in ('%d', '%s'))
        if {t: n for t, n in a.items() if t in ('%d', '%s')} != {t: n for t, n in b.items() if t in ('%d', '%s')}:
            hard += 1
            print('ERRO %%d/%%s diferente no indice %s: %r -> %r' % (k, orig[:60], v[:60]))
        elif bad <= 15:
            print('aviso: codigos de controle diferentes no indice %s: %s vs %s' % (k, dict(a), dict(b)))
    S[i] = v.encode('utf-8') + b'\x00'
if hard:
    sys.exit('%d textos com %%d/%%s diferentes do original (travaria o jogo). Corrija.' % hard)
print('%d textos traduzidos (%d com codigos de controle diferentes)' % (len(T), bad))

mt_tool.write_dat(tmp_out, mt_tool.build_raw(S))
new = open(tmp_out, 'rb').read()

dst = apk + '.tmp'
with zipfile.ZipFile(apk) as zi, zipfile.ZipFile(dst, 'w') as zo:
    for it in zi.infolist():
        data = new if it.filename == name else zi.read(it.filename)
        zo.writestr(it, data, compress_type=it.compress_type)
os.replace(dst, apk)
print('substituido', name, '(%d bytes, era %d)' % (len(new), info.file_size))
