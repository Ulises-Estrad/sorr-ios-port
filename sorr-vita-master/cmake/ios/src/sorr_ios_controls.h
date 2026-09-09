/* SDL touch controls using Kenney Mobile Controls (CC0), plus DualSense input.
 * Included by the iOS shell after the Bennu virtual-key bridge declaration.
 */
#include <zlib.h>
#include "sorr_ios_touch_art.h"

enum { TOUCH_ATTACK, TOUCH_JUMP, TOUCH_SPECIAL, TOUCH_POLICE, TOUCH_START,
       TOUCH_SERIES, TOUCH_BACK_ATTACK, TOUCH_COMBO, TOUCH_COUNT };
static const int touch_keys[TOUCH_COUNT] = {46, 47, 45, 48, 28, 30, 32, 31};
static const char *touch_labels[TOUCH_COUNT] = {
    "ATTACK", "JUMP", "SPECIAL", "POLICE", "START", "SERIES", "BACK ATK", "COMBO"
};
typedef struct { SDL_FingerID id; int active, control; } TouchFinger;
static TouchFinger touch_fingers[16];
static SDL_FRect touch_rects[TOUCH_COUNT];
static float touch_joy_x, touch_joy_y, touch_joy_radius, touch_joy_dx, touch_joy_dy;
static int touch_width = 1, touch_height = 1;
static int touch_counts[TOUCH_COUNT], touch_direction;
static SDL_GameController *touch_controller;
static int touch_controller_buttons[TOUCH_COUNT], touch_controller_back;
static int touch_background;
static SDL_Renderer *touch_art_renderer;
static SDL_Texture *touch_art[3];

static void sorr_ios_d4a_set_active_layout(const sorr_ios_data_layout *layout)
{
    (void)layout; /* The fixed layout supersedes the old editable CFG file. */
}

static void touch_set_button(int index, int count)
{
    count = count > 0 ? count : 0;
    if (!!count != !!touch_counts[index])
        sorr_ios_touch_set_bennu_key(touch_keys[index], count > 0);
    touch_counts[index] = count;
}

static void touch_set_direction(int mask)
{
    static const int keys[4] = {72, 80, 75, 77};
    int i;
    for (i = 0; i < 4; ++i)
        if ((mask ^ touch_direction) & (1 << i))
            sorr_ios_touch_set_bennu_key(keys[i], !!(mask & (1 << i)));
    touch_direction = mask;
}

static void touch_release_all(void)
{
    int i;
    for (i = 0; i < TOUCH_COUNT; ++i) touch_set_button(i, 0);
    memset(touch_fingers, 0, sizeof(touch_fingers));
    memset(touch_controller_buttons, 0, sizeof(touch_controller_buttons));
    touch_set_direction(0);
    sorr_ios_touch_set_bennu_key(1, 0);
    touch_controller_back = 0;
    touch_joy_dx = touch_joy_dy = 0;
}

/* All rectangles are in output pixels; finger coordinates use the same space.
 * Diameter and gaps scale with screen height and remain circular in landscape.
 */
static void touch_layout(int width, int height)
{
    float size = fminf(height * .145f, width * .083f);
    float gap = size * .20f;
    float right = width * .94f;
    float x = right - 3 * size - 2 * gap;
    float bottom = height * .88f;
    int rows[3][3] = {{TOUCH_POLICE, -1, -1},
                     {TOUCH_ATTACK, TOUCH_JUMP, TOUCH_SPECIAL},
                     {TOUCH_BACK_ATTACK, TOUCH_SERIES, TOUCH_COMBO}};
    int row, col;
    touch_width = width;
    touch_height = height;
    for (row = 0; row < 3; ++row)
        for (col = 0; col < 3; ++col)
            if (rows[row][col] >= 0)
                touch_rects[rows[row][col]] = (SDL_FRect){
                    x + col * (size + gap), bottom - size - (2-row)*(size+gap), size, size};
    touch_rects[TOUCH_START] = (SDL_FRect){right-size*1.25f, height*.085f, size*1.25f, size*.65f};
    touch_joy_radius = fminf(height * .18f, width * .12f);
    touch_joy_x = width * .17f;
    touch_joy_y = height * .72f;
}

static void touch_update_joystick(float x, float y)
{
    float dx = (x - touch_joy_x) / touch_joy_radius;
    float dy = (y - touch_joy_y) / touch_joy_radius;
    float length = sqrtf(dx*dx + dy*dy);
    int mask = 0;
    if (length > 1) { dx /= length; dy /= length; }
    touch_joy_dx = dx;
    touch_joy_dy = dy;
    if (length >= .22f)
    {
        if (fabsf(dx) >= .30f && fabsf(dx) >= fabsf(dy)*.58f) mask |= dx < 0 ? 4 : 8;
        if (fabsf(dy) >= .30f && fabsf(dy) >= fabsf(dx)*.58f) mask |= dy < 0 ? 1 : 2;
    }
    touch_set_direction(mask);
}

static void touch_finger(SDL_FingerID id, float nx, float ny, int down, int motion)
{
    int i, slot = -1, hit = -1, joy_owned = 0;
    float x = nx * touch_width, y = ny * touch_height;
    for (i = 0; i < 16; ++i)
    {
        if (touch_fingers[i].active && touch_fingers[i].id == id) slot = i;
        if (touch_fingers[i].active && touch_fingers[i].control == TOUCH_COUNT) joy_owned = 1;
    }
    /* A move from a finger that was held during controller handoff cannot latch. */
    if (slot < 0 && (!down || motion)) return;
    if (slot < 0)
    {
        for (i = 0; i < 16; ++i) if (!touch_fingers[i].active) { slot = i; break; }
        if (slot < 0) return;
        touch_fingers[slot] = (TouchFinger){id, 1, -1};
        if (!joy_owned && hypotf(x-touch_joy_x, y-touch_joy_y) <= touch_joy_radius*1.25f)
            touch_fingers[slot].control = TOUCH_COUNT;
    }
    if (touch_fingers[slot].control == TOUCH_COUNT)
    {
        if (down) touch_update_joystick(x, y);
        else { touch_set_direction(0); touch_joy_dx = touch_joy_dy = 0; }
    }
    else
    {
        if (down)
            for (i = 0; i < TOUCH_COUNT; ++i)
            {
                SDL_FRect r = touch_rects[i];
                if (x >= r.x && x <= r.x+r.w && y >= r.y && y <= r.y+r.h) { hit = i; break; }
            }
        if (hit != touch_fingers[slot].control)
        {
            int old = touch_fingers[slot].control;
            if (old >= 0) touch_set_button(old, touch_counts[old]-1);
            if (hit >= 0) touch_set_button(hit, touch_counts[hit]+1);
            touch_fingers[slot].control = hit;
        }
    }
    if (!down) touch_fingers[slot].active = 0;
}

/* Poll on the rendering thread, not SDL's possibly asynchronous event watcher.
 * Re-enumeration handles preconnected controllers, hotplug and reconnect.
 */
static void touch_poll_controller(void)
{
    int i, mask = 0, buttons[TOUCH_COUNT] = {0};
    if (touch_controller && !SDL_GameControllerGetAttached(touch_controller))
    {
        touch_release_all();
        SDL_GameControllerClose(touch_controller);
        touch_controller = NULL;
    }
    if (!touch_controller)
        for (i = 0; i < SDL_NumJoysticks(); ++i)
        {
            SDL_GameController *candidate;
            if (!SDL_IsGameController(i)) continue;
            candidate = SDL_GameControllerOpen(i);
            if (!candidate) continue;
            if (SDL_GameControllerGetType(candidate) == SDL_CONTROLLER_TYPE_PS5)
            {
                touch_release_all();
                touch_controller = candidate;
                break;
            }
            SDL_GameControllerClose(candidate);
        }
    if (!touch_controller || touch_background) return;
    SDL_GameControllerUpdate();
#define PAD(button) SDL_GameControllerGetButton(touch_controller, SDL_CONTROLLER_BUTTON_##button)
    {
        int x = SDL_GameControllerGetAxis(touch_controller, SDL_CONTROLLER_AXIS_LEFTX);
        int y = SDL_GameControllerGetAxis(touch_controller, SDL_CONTROLLER_AXIS_LEFTY);
        if (PAD(DPAD_UP) || y < -9000) mask |= 1;
        if (PAD(DPAD_DOWN) || y > 9000) mask |= 2;
        if (PAD(DPAD_LEFT) || x < -9000) mask |= 4;
        if (PAD(DPAD_RIGHT) || x > 9000) mask |= 8;
        if ((mask & 3) == 3) mask &= ~3;
        if ((mask & 12) == 12) mask &= ~12;
    }
    touch_set_direction(mask);
    buttons[TOUCH_JUMP] = PAD(A);       /* Cross */
    buttons[TOUCH_ATTACK] = PAD(X);     /* Square */
    buttons[TOUCH_SPECIAL] = PAD(Y);    /* Triangle */
    buttons[TOUCH_SERIES] = PAD(B);     /* Circle */
    buttons[TOUCH_POLICE] = PAD(LEFTSHOULDER);
    buttons[TOUCH_BACK_ATTACK] = PAD(RIGHTSHOULDER);
    buttons[TOUCH_START] = PAD(START);  /* Options */
    /* Hysteresis prevents trigger jitter producing repeated presses. */
    buttons[TOUCH_COMBO] = SDL_GameControllerGetAxis(touch_controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT)
        > (touch_controller_buttons[TOUCH_COMBO] ? 6000 : 10000);
    for (i = 0; i < TOUCH_COUNT; ++i)
    {
        touch_set_button(i, buttons[i]);
        touch_controller_buttons[i] = buttons[i];
    }
    if (!!PAD(BACK) != touch_controller_back)
    {
        touch_controller_back = !!PAD(BACK);
        sorr_ios_touch_set_bennu_key(1, touch_controller_back); /* Create: menu back */
    }
#undef PAD
}

void sorr_ios_d4a_process_sdl_event(const SDL_Event *event)
{
    if (!event) return;
    switch (event->type)
    {
        case SDL_APP_WILLENTERBACKGROUND:
        case SDL_APP_DIDENTERBACKGROUND:
        case SDL_APP_TERMINATING:
        case SDL_QUIT:
            touch_background = 1;
            touch_release_all();
            return;
        case SDL_APP_DIDENTERFOREGROUND:
            touch_background = 0;
            return;
        case SDL_WINDOWEVENT:
            if (event->window.event == SDL_WINDOWEVENT_FOCUS_LOST) touch_release_all();
            return;
        default: break;
    }
    if (touch_controller || touch_background) return;
    if (event->type == SDL_FINGERDOWN || event->type == SDL_FINGERMOTION || event->type == SDL_FINGERUP)
        touch_finger(event->tfinger.fingerId, event->tfinger.x, event->tfinger.y,
                     event->type != SDL_FINGERUP, event->type == SDL_FINGERMOTION);
}

static SDL_Texture *touch_texture(SDL_Renderer *renderer, const unsigned char *pixels,
                                  size_t size, int w, int h)
{
    SDL_Surface *surface = SDL_CreateRGBSurfaceWithFormat(0, w, h, 32, SDL_PIXELFORMAT_RGBA32);
    SDL_Texture *texture = NULL;
    uLongf length = (uLongf)w*h*4;
    if (!surface) return NULL;
    if (surface->pitch == w*4 && uncompress(surface->pixels, &length, pixels, (uLong)size) == Z_OK)
        texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (texture) { SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND); SDL_SetTextureScaleMode(texture, SDL_ScaleModeLinear); }
    return texture;
}

static void touch_draw_sprite(SDL_Renderer *renderer, int art, SDL_FRect rect, int pressed)
{
    if (!touch_art[art]) return;
    SDL_SetTextureColorMod(touch_art[art], pressed ? 120 : 255, pressed ? 205 : 255, 255);
    SDL_SetTextureAlphaMod(touch_art[art], pressed ? 255 : 215);
    SDL_RenderCopyF(renderer, touch_art[art], NULL, &rect);
}

void sorr_ios_d4a_draw_touch_overlay(SDL_Renderer *renderer)
{
    int w, h, lw, lh, i;
    float sx, sy;
    SDL_Rect viewport, clip;
    SDL_bool clipped;
    SDL_BlendMode blend;
    Uint8 r, g, b, a;
    touch_poll_controller();
    if (!renderer || touch_controller || touch_background) return;
    if (SDL_GetRendererOutputSize(renderer, &w, &h) || w <= 0 || h <= 0) return;
    touch_layout(w, h);
    if (touch_art_renderer != renderer)
    {
        /* SDL owns textures and frees them when their renderer is destroyed. */
        touch_art_renderer = renderer;
        touch_art[0] = touch_texture(renderer, touch_button_pixels, sizeof(touch_button_pixels), touch_button_w, touch_button_h);
        touch_art[1] = touch_texture(renderer, touch_pad_pixels, sizeof(touch_pad_pixels), touch_pad_w, touch_pad_h);
        touch_art[2] = touch_texture(renderer, touch_nub_pixels, sizeof(touch_nub_pixels), touch_nub_w, touch_nub_h);
    }
    SDL_RenderGetLogicalSize(renderer, &lw, &lh);
    SDL_RenderGetScale(renderer, &sx, &sy);
    SDL_RenderGetViewport(renderer, &viewport);
    SDL_RenderGetClipRect(renderer, &clip);
    clipped = SDL_RenderIsClipEnabled(renderer);
    SDL_GetRenderDrawColor(renderer, &r, &g, &b, &a);
    SDL_GetRenderDrawBlendMode(renderer, &blend);
    SDL_RenderSetLogicalSize(renderer, 0, 0);
    SDL_RenderSetViewport(renderer, NULL);
    SDL_RenderSetClipRect(renderer, NULL);
    SDL_RenderSetScale(renderer, 1, 1);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    {
        float radius = touch_joy_radius;
        SDL_FRect pad = {touch_joy_x-radius, touch_joy_y-radius, radius*2, radius*2};
        float nx = touch_joy_x + touch_joy_dx*radius*.50f;
        float ny = touch_joy_y + touch_joy_dy*radius*.50f;
        SDL_FRect nub = {nx-radius*.43f, ny-radius*.43f, radius*.86f, radius*.86f};
        touch_draw_sprite(renderer, 1, pad, 0);
        touch_draw_sprite(renderer, 2, nub, touch_direction != 0);
    }
    for (i = 0; i < TOUCH_COUNT; ++i)
    {
        SDL_FRect rect = touch_rects[i];
        int scale = (int)(rect.w / (strlen(touch_labels[i])*6 + 8));
        if (scale < 1) scale = 1;
        touch_draw_sprite(renderer, 0, rect, touch_counts[i] > 0);
        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
        sorr_ios_draw_text(renderer, (int)(rect.x + (rect.w-strlen(touch_labels[i])*6*scale)/2),
                           (int)(rect.y+(rect.h-7*scale)/2), scale, touch_labels[i]);
    }
    SDL_RenderSetLogicalSize(renderer, lw, lh);
    SDL_RenderSetViewport(renderer, &viewport);
    SDL_RenderSetClipRect(renderer, clipped ? &clip : NULL);
    SDL_RenderSetScale(renderer, sx, sy);
    SDL_SetRenderDrawBlendMode(renderer, blend);
    SDL_SetRenderDrawColor(renderer, r, g, b, a);
}
