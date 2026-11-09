# Inotia 4 ModMenu

1. Coloque o `.apkm` em `input/inotia4.apkm` (arquivo grande: o commit pelo site pode falhar; use git push).
2. GitHub -> Actions -> "Build Inotia4 ModMenu" -> Run workflow.
3. Baixe o artefato `inotia4-mod` (zip com `inotia4-mod.apk`).
4. Desinstale o Inotia 4 original (assinatura diferente) e instale o APK.

Teste de sanidade: rode com `with_mod = false`. Se esse APK ja fechar sozinho, o problema e a
verificacao de integridade (Hercules), nao o mod.

No menu, "Salvar dump" cria `Android/data/<pacote>/files/modmenu_dump.txt`: envie para continuar
o recurso de itens cash.
