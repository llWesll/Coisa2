#!/usr/bin/env python3
"""Insere ModMenu.start(this) no onCreate do MainActivity (smali do apktool).
Uso: patch_smali.py <pasta_apktool>"""
import sys, glob, re

root = sys.argv[1]
fs = glob.glob(root + '/smali*/com/com2us/inotia4/normal/freefull/google/global/android/common/MainActivity.smali')
assert fs, 'MainActivity.smali nao encontrado'
p = fs[0]
s = open(p, encoding='utf-8').read()
if 'Lcom/mod/ModMenu;' in s:
    print('ja patcheado'); sys.exit(0)

INJECT = '    invoke-static {p0}, Lcom/mod/ModMenu;->start(Landroid/app/Activity;)V\n'

m = re.search(r'^\.method [^\n]*\bonCreate\(Landroid/os/Bundle;\)V\n(.*?)^\.end method', s, re.S | re.M)
if m:
    body = m.group(1)
    mm = re.search(r'^[ \t]*invoke-super \{[^}]*\}, L[^;\n]+;->onCreate\(Landroid/os/Bundle;\)V\n', body, re.M)
    assert mm, 'invoke-super onCreate nao encontrado: ajustar manualmente'
    pos = m.start(1) + mm.end()
    s = s[:pos] + INJECT + s[pos:]
else:
    sup = re.search(r'^\.super (L[^;\n]+;)', s, re.M).group(1)
    s = s.rstrip('\n') + (
        '\n\n.method protected onCreate(Landroid/os/Bundle;)V\n'
        '    .locals 0\n\n'
        '    invoke-super {p0, p1}, %s->onCreate(Landroid/os/Bundle;)V\n\n'
        '%s\n    return-void\n.end method\n' % (sup, INJECT))
    print('onCreate nao existia: metodo criado')

open(p, 'w', encoding='utf-8').write(s)
print('patch ok em', p)
