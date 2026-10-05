# Teste (Construct 3 → Switch .nro)

Port em C++ (libnx, sem dependências extras) do projeto "Teste" do Construct 3: um sprite (76x78) que se move em
8 direções com aceleração 600, desaceleração 500 e velocidade máxima 200, os mesmos
valores do behavior "8Direções" do projeto original.

## Controles (Switch)
- Analógico esquerdo ou D-pad: mover
- `+`: sair

## Compilar pelo GitHub
1. Crie um repositório no GitHub e suba todo o conteúdo desta pasta (incluindo `.github`).
2. Abra a aba **Actions**; o workflow "Compilar NRO" roda sozinho a cada push.
   Também dá para rodar manualmente em "Run workflow".
3. Quando terminar, baixe o artefato **Teste-nro** (contém `Teste.nro`).
4. Copie para o cartão SD em `/switch/Teste/Teste.nro` e abra pelo Homebrew Menu.

## Testar no PC (opcional)
Com SDL2 instalado (Linux: `sudo apt install libsdl2-dev`):

    g++ -std=c++17 source/main.cpp $(sdl2-config --cflags --libs) -o teste && ./teste

Setas ou WASD movem; `Esc` sai.

## Trocar a imagem do sprite
Edite `assets/sprite.png` (250x250) e rode `python3 tools/convert_sprite.py`
(precisa de `pip install pillow`). Isso gera `romfs/sprite.rgba`, que o jogo carrega.
