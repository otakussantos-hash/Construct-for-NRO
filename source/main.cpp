// Teste - port em C++ do projeto Construct 3 "Teste" para Nintendo Switch (.nro)
//
// No Switch usa apenas a libnx (sem SDL2, sem pacotes extras).
// No PC (opcional, para testar) usa SDL2 e o teclado.

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>

#ifdef __SWITCH__
#include <switch.h>
#define ASSET(p) "romfs:/" p
#else
#include <SDL2/SDL.h>
#define ASSET(p) "romfs/" p
#endif

// ---- Valores copiados do projeto Construct 3 (data.json) ----
static const int   SCREEN_W   = 1280;
static const int   SCREEN_H   = 720;
static const int   SPRITE_SRC = 250;      // tamanho da imagem original
static const int   SPRITE_W   = 76;       // tamanho da instância no layout
static const int   SPRITE_H   = 78;
static const float START_X    = 404.0f;   // posição inicial (origem no centro)
static const float START_Y    = 291.0f;
// Comportamento "8Direções": velocidade máx., aceleração, desaceleração
static const float MAX_SPEED  = 200.0f;
static const float ACCEL      = 600.0f;
static const float DECEL      = 500.0f;
// -------------------------------------------------------------

static const float STICK_DEADZONE = 0.25f;

// Sprite já reduzido para 76x78, em bytes R,G,B,A
static std::vector<unsigned char> g_sprite(SPRITE_W * SPRITE_H * 4);

// Lê romfs/sprite.rgba (250x250) e reduz para o tamanho do layout.
// Se falhar, preenche com um quadrado azul para o jogo continuar funcionando.
static void loadSprite() {
    std::vector<unsigned char> src(SPRITE_SRC * SPRITE_SRC * 4);
    bool ok = false;
    FILE* f = fopen(ASSET("sprite.rgba"), "rb");
    if (f) {
        ok = fread(src.data(), 1, src.size(), f) == src.size();
        fclose(f);
    }
    for (int y = 0; y < SPRITE_H; y++) {
        for (int x = 0; x < SPRITE_W; x++) {
            unsigned char* d = &g_sprite[(y * SPRITE_W + x) * 4];
            if (ok) {
                int sx = (int)((x + 0.5f) * SPRITE_SRC / SPRITE_W);
                int sy = (int)((y + 0.5f) * SPRITE_SRC / SPRITE_H);
                if (sx >= SPRITE_SRC) sx = SPRITE_SRC - 1;
                if (sy >= SPRITE_SRC) sy = SPRITE_SRC - 1;
                memcpy(d, &src[(sy * SPRITE_SRC + sx) * 4], 4);
            } else {
                d[0] = 40; d[1] = 90; d[2] = 200; d[3] = 255;
            }
        }
    }
}

// Movimento 8 direções com aceleração/desaceleração (como o behavior do Construct)
struct Player {
    float x = START_X, y = START_Y;
    float vx = 0.0f, vy = 0.0f;

    void update(float ix, float iy, float dt) {
        float tx = ix * MAX_SPEED;
        float ty = iy * MAX_SPEED;
        float ddx = tx - vx;
        float ddy = ty - vy;
        float dist = std::sqrt(ddx * ddx + ddy * ddy);
        float rate = (ix != 0.0f || iy != 0.0f) ? ACCEL : DECEL;
        float step = rate * dt;
        if (dist <= step) {
            vx = tx;
            vy = ty;
        } else {
            vx += ddx / dist * step;
            vy += ddy / dist * step;
        }

        x += vx * dt;
        y += vy * dt;

        // Mantém o personagem dentro da tela
        const float hw = SPRITE_W / 2.0f, hh = SPRITE_H / 2.0f;
        if (x < hw)            { x = hw;            vx = 0.0f; }
        if (x > SCREEN_W - hw) { x = SCREEN_W - hw; vx = 0.0f; }
        if (y < hh)            { y = hh;            vy = 0.0f; }
        if (y > SCREEN_H - hh) { y = SCREEN_H - hh; vy = 0.0f; }
    }
};

#ifdef __SWITCH__
// =============================== SWITCH (libnx) ===============================

static PadState g_pad;

// Direção desejada em ix/iy (-1 a 1, y para baixo). Retorna true para sair.
static bool readInput(float& ix, float& iy) {
    ix = 0.0f;
    iy = 0.0f;

    padUpdate(&g_pad);
    u64 held = padGetButtons(&g_pad);
    bool quit = (padGetButtonsDown(&g_pad) & HidNpadButton_Plus) != 0;

    float dx = 0.0f, dy = 0.0f;
    if (held & HidNpadButton_Left)  dx -= 1.0f;
    if (held & HidNpadButton_Right) dx += 1.0f;
    if (held & HidNpadButton_Up)    dy -= 1.0f;
    if (held & HidNpadButton_Down)  dy += 1.0f;

    if (dx != 0.0f || dy != 0.0f) {
        float len = std::sqrt(dx * dx + dy * dy);   // diagonal normalizada
        ix = dx / len;
        iy = dy / len;
    } else {
        // Analógico esquerdo (y do libnx é positivo para cima)
        HidAnalogStickState s = padGetStickPos(&g_pad, 0);
        float sx = s.x / 32767.0f;
        float sy = -s.y / 32767.0f;
        float mag = std::sqrt(sx * sx + sy * sy);
        if (mag > STICK_DEADZONE) {
            float scaled = (mag - STICK_DEADZONE) / (1.0f - STICK_DEADZONE);
            if (scaled > 1.0f) scaled = 1.0f;
            ix = sx / mag * scaled;
            iy = sy / mag * scaled;
        }
    }
    return quit;
}

static void drawFrame(u8* base, u32 stride, const Player& p) {
    // Fundo branco
    for (int y = 0; y < SCREEN_H; y++) memset(base + (size_t)y * stride, 0xFF, SCREEN_W * 4);

    int x0 = (int)std::lround(p.x - SPRITE_W / 2.0f);
    int y0 = (int)std::lround(p.y - SPRITE_H / 2.0f);
    for (int sy = 0; sy < SPRITE_H; sy++) {
        int py = y0 + sy;
        if (py < 0 || py >= SCREEN_H) continue;
        u8* row = base + (size_t)py * stride;
        for (int sx = 0; sx < SPRITE_W; sx++) {
            int px = x0 + sx;
            if (px < 0 || px >= SCREEN_W) continue;
            const unsigned char* s = &g_sprite[(sy * SPRITE_W + sx) * 4];
            u8* d = row + px * 4;                       // formato RGBA8888
            int a = s[3];
            d[0] = (u8)((s[0] * a + d[0] * (255 - a)) / 255);
            d[1] = (u8)((s[1] * a + d[1] * (255 - a)) / 255);
            d[2] = (u8)((s[2] * a + d[2] * (255 - a)) / 255);
            d[3] = 255;
        }
    }
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    romfsInit();
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    padInitializeDefault(&g_pad);

    NWindow* win = nwindowGetDefault();
    Framebuffer fb;
    framebufferCreate(&fb, win, SCREEN_W, SCREEN_H, PIXEL_FORMAT_RGBA_8888, 2);
    framebufferMakeLinear(&fb);

    loadSprite();

    Player player;
    u64 last = armGetSystemTick();
    const double freq = (double)armGetSystemTickFreq();

    while (appletMainLoop()) {
        u64 now = armGetSystemTick();
        float dt = (float)((now - last) / freq);
        last = now;
        if (dt > 0.05f) dt = 0.05f;

        float ix, iy;
        if (readInput(ix, iy)) break;
        player.update(ix, iy, dt);

        u32 stride;
        u8* buf = (u8*)framebufferBegin(&fb, &stride);
        drawFrame(buf, stride, player);
        framebufferEnd(&fb);      // apresenta o quadro (sincroniza com o vsync)
    }

    framebufferClose(&fb);
    romfsExit();
    return 0;
}

#else
// ================================ PC (SDL2) ===================================

static bool readInput(float& ix, float& iy) {
    ix = 0.0f;
    iy = 0.0f;
    const Uint8* k = SDL_GetKeyboardState(nullptr);
    float dx = 0.0f, dy = 0.0f;
    if (k[SDL_SCANCODE_LEFT]  || k[SDL_SCANCODE_A]) dx -= 1.0f;
    if (k[SDL_SCANCODE_RIGHT] || k[SDL_SCANCODE_D]) dx += 1.0f;
    if (k[SDL_SCANCODE_UP]    || k[SDL_SCANCODE_W]) dy -= 1.0f;
    if (k[SDL_SCANCODE_DOWN]  || k[SDL_SCANCODE_S]) dy += 1.0f;
    if (dx != 0.0f || dy != 0.0f) {
        float len = std::sqrt(dx * dx + dy * dy);
        ix = dx / len;
        iy = dy / len;
    }
    return k[SDL_SCANCODE_ESCAPE] != 0;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        printf("SDL_Init falhou: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window* win = SDL_CreateWindow("Teste", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                       SCREEN_W, SCREEN_H, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = win ? SDL_CreateRenderer(win, -1, SDL_RENDERER_PRESENTVSYNC) : nullptr;
    if (!ren) {
        printf("Falha ao criar janela/renderer: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_RenderSetLogicalSize(ren, SCREEN_W, SCREEN_H);

    loadSprite();
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
                                         SPRITE_W, SPRITE_H);
    SDL_UpdateTexture(tex, nullptr, g_sprite.data(), SPRITE_W * 4);
    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);

    Player player;
    Uint64 last = SDL_GetPerformanceCounter();
    const double freq = (double)SDL_GetPerformanceFrequency();
    bool running = true;

    while (running) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
        }
        Uint64 now = SDL_GetPerformanceCounter();
        float dt = (float)((now - last) / freq);
        last = now;
        if (dt > 0.05f) dt = 0.05f;

        float ix, iy;
        if (readInput(ix, iy)) running = false;
        player.update(ix, iy, dt);

        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderClear(ren);
        SDL_Rect dst;
        dst.x = (int)std::lround(player.x - SPRITE_W / 2.0f);
        dst.y = (int)std::lround(player.y - SPRITE_H / 2.0f);
        dst.w = SPRITE_W;
        dst.h = SPRITE_H;
        SDL_RenderCopy(ren, tex, nullptr, &dst);
        SDL_RenderPresent(ren);
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
#endif
