# Inotia 4 – ModMenu + Tradução PT-BR

1. Coloque o `.apkm` em `input/inotia4.apkm` OU suba-o numa Release com a tag `apk` (arquivo `inotia4.apkm`).
2. Actions -> "Build Inotia4 ModMenu" -> Run workflow, e escolha:
   - `with_mod`: injeta o menu de mod.
   - `with_translation`: aplica `translation/texts_pt.json`.
   - `lang`: idioma do jogo que será trocado pelo PT-BR (padrão `en`; no jogo, selecione esse idioma).
3. Baixe o artefato `inotia4-build`. O arquivo se chama `inotia4.apk`, `inotia4-mod.apk`,
   `inotia4-ptbr.apk` ou `inotia4-mod-ptbr.apk`, conforme as opções.
4. Desinstale o original antes

## Tradução
- `translation/texts_en.json`: todos os textos em inglês
- `translation/texts_pt.json`: a tradução, mesmo formato e mesmos índices, o resto continua em inglês.
- `tools/mt_tool.py unpack|pack`: extrai/recria os arquivos `memorytext_*.dat.jpg`.
