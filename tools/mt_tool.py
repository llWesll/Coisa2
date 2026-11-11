#!/usr/bin/env python3
"""Ferramenta de texto do Inotia 4 (memorytext_*.dat.jpg).

Formato (verificado): arquivo .dat.jpg = 2 bytes (01 00) + LZMA 'alone'.
Dentro: uint32 N, depois N offsets de 3 bytes (little-endian), depois as strings UTF-8
terminadas em \\0. Codigos de controle do jogo ($B $R &N &P $S %d %s ...) devem ser mantidos.

Uso:
  mt_tool.py unpack memorytext_en.dat.jpg texts.json   # extrai {indice: texto}
  mt_tool.py pack   texts.json memorytext_en.dat.jpg   # recria o arquivo
  (pack usa o original como base: indices ausentes no JSON ficam como estavam)
    mt_tool.py pack texts.json novo.dat.jpg --base memorytext_en.dat.jpg
"""
import sys, json, lzma, struct

def read_dat(path):
    d = open(path, 'rb').read()
    raw = lzma.LZMADecompressor(format=lzma.FORMAT_ALONE).decompress(d[2:])
    n = struct.unpack('<I', raw[:4])[0]
    offs = [int.from_bytes(raw[4+3*i:7+3*i], 'little') for i in range(n)] + [len(raw)]
    return [raw[offs[i]:offs[i+1]] for i in range(n)]   # cada item inclui o \0 final

def build_raw(strs):
    n = len(strs)
    base = 4 + 3 * n
    offs, pos = [], base
    for s in strs:
        offs.append(pos); pos += len(s)
    assert pos < (1 << 24), 'arquivo grande demais para offsets de 3 bytes'
    return struct.pack('<I', n) + b''.join(o.to_bytes(3, 'little') for o in offs) + b''.join(strs)

def write_dat(path, raw):
    filt = [{'id': lzma.FILTER_LZMA1, 'dict_size': 1 << 24, 'lc': 3, 'lp': 0, 'pb': 2,
             'preset': 9 | lzma.PRESET_EXTREME}]
    comp = lzma.compress(raw, format=lzma.FORMAT_ALONE, filters=filt)
    # cabecalho: props(5) + tamanho real (8 bytes) no lugar do 'desconhecido'
    comp = comp[:5] + struct.pack('<Q', len(raw)) + comp[13:]
    open(path, 'wb').write(b'\x01\x00' + comp)

def main():
    a = sys.argv[1:]
    if len(a) >= 3 and a[0] == 'unpack':
        S = read_dat(a[1])
        out = {str(i): s[:-1].decode('utf-8') for i, s in enumerate(S) if s not in (b'\x00', b'')}
        json.dump(out, open(a[2], 'w', encoding='utf-8'), ensure_ascii=False, indent=0)
        print(len(S), 'registros,', len(out), 'com texto ->', a[2])
    elif len(a) >= 3 and a[0] == 'pack':
        base = a[a.index('--base') + 1] if '--base' in a else a[2]
        S = read_dat(base)
        T = json.load(open(a[1], encoding='utf-8'))
        for k, v in T.items():
            S[int(k)] = v.encode('utf-8') + b'\x00'
        write_dat(a[2], build_raw(S))
        print('gravado', a[2], len(S), 'registros')
    else:
        print(__doc__)
if __name__ == "__main__":
    main()
