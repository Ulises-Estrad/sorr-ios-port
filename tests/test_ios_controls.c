#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
typedef struct { int unused; } sorr_ios_data_layout;
static int keys[127];
static void sorr_ios_touch_set_bennu_key(int key, int pressed) { assert(key > 0 && key < 127); keys[key] = pressed; }
static void sorr_ios_draw_text(SDL_Renderer *r, int x, int y, int scale, const char *text)
{ (void)r; (void)x; (void)y; (void)scale; (void)text; }
#include "../sorr-vita-master/cmake/ios/src/sorr_ios_controls.h"

static void finger(int id, int control, Uint32 type)
{
    SDL_Event e = {0};
    e.type = type;
    e.tfinger.fingerId = id;
    e.tfinger.x = (touch_rects[control].x + touch_rects[control].w*.5f) / touch_width;
    e.tfinger.y = (touch_rects[control].y + touch_rects[control].h*.5f) / touch_height;
    sorr_ios_d4a_process_sdl_event(&e);
}

int main(void)
{
    SDL_Surface *surface;
    SDL_Renderer *renderer;
    SDL_Joystick *joystick;
    SDL_VirtualJoystickDesc desc = {0};
    int device, i;
    assert(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) == 0);
    surface = SDL_CreateRGBSurfaceWithFormat(0, 844, 390, 32, SDL_PIXELFORMAT_RGBA32);
    renderer = SDL_CreateSoftwareRenderer(surface);
    assert(renderer);
    sorr_ios_d4a_draw_touch_overlay(renderer);
    assert(touch_art[0] && touch_art[1] && touch_art[2]);
    assert(touch_rects[TOUCH_POLICE].y < touch_rects[TOUCH_ATTACK].y);
    assert(touch_rects[TOUCH_ATTACK].x < touch_rects[TOUCH_JUMP].x);
    assert(touch_rects[TOUCH_JUMP].x < touch_rects[TOUCH_SPECIAL].x);
    assert(touch_rects[TOUCH_BACK_ATTACK].y > touch_rects[TOUCH_ATTACK].y);
    /* Two fingers on the same button must not release each other. */
    finger(1, TOUCH_ATTACK, SDL_FINGERDOWN);
    finger(2, TOUCH_ATTACK, SDL_FINGERDOWN);
    finger(1, TOUCH_ATTACK, SDL_FINGERUP);
    assert(keys[46]);
    finger(3, TOUCH_JUMP, SDL_FINGERDOWN);
    assert(keys[46] && keys[47]);
    finger(2, TOUCH_ATTACK, SDL_FINGERUP);
    finger(3, TOUCH_JUMP, SDL_FINGERUP);
    assert(!keys[46] && !keys[47]);
    touch_finger(4, touch_joy_x/touch_width, touch_joy_y/touch_height, 1, 0);
    touch_finger(4, (touch_joy_x+touch_joy_radius)/touch_width, (touch_joy_y-touch_joy_radius)/touch_height, 1, 1);
    assert(keys[72] && keys[77]);
    touch_finger(4, 0, 0, 0, 0);
    assert(!keys[72] && !keys[77]);
    finger(5, TOUCH_POLICE, SDL_FINGERDOWN);
    assert(keys[48]);

    desc.version = SDL_VIRTUAL_JOYSTICK_DESC_VERSION;
    desc.type = SDL_JOYSTICK_TYPE_GAMECONTROLLER;
    desc.naxes = SDL_CONTROLLER_AXIS_MAX;
    desc.nbuttons = SDL_CONTROLLER_BUTTON_MAX;
    desc.vendor_id = 0x054c;
    desc.product_id = 0x0ce6;
    desc.name = "DualSense Wireless Controller";
    desc.button_mask = (1u << SDL_CONTROLLER_BUTTON_MAX)-1;
    desc.axis_mask = (1u << SDL_CONTROLLER_AXIS_MAX)-1;
    device = SDL_JoystickAttachVirtualEx(&desc);
    assert(device >= 0);
    joystick = SDL_JoystickOpen(device);
    assert(joystick);
    SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERLEFT, -32768);
    SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, -32768);
    SDL_JoystickUpdate();
    touch_poll_controller();
    assert(touch_controller);
    assert(!keys[48]); /* Connect releases held touch input. */
    finger(6, TOUCH_ATTACK, SDL_FINGERDOWN);
    assert(!keys[46]); /* Hidden controls do not respond to touches. */
    {
        int buttons[] = {SDL_CONTROLLER_BUTTON_A, SDL_CONTROLLER_BUTTON_X, SDL_CONTROLLER_BUTTON_Y,
            SDL_CONTROLLER_BUTTON_LEFTSHOULDER, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,
            SDL_CONTROLLER_BUTTON_B, SDL_CONTROLLER_BUTTON_START};
        int expected[] = {47, 46, 45, 48, 32, 30, 28};
        for (i=0; i<7; ++i)
        {
            SDL_JoystickSetVirtualButton(joystick, buttons[i], 1);
            touch_poll_controller();
            assert(keys[expected[i]]);
            SDL_JoystickSetVirtualButton(joystick, buttons[i], 0);
            touch_poll_controller();
            assert(!keys[expected[i]]);
        }
    }
    SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_LEFTX, 24000);
    SDL_JoystickSetVirtualButton(joystick, SDL_CONTROLLER_BUTTON_DPAD_UP, 1);
    SDL_JoystickSetVirtualAxis(joystick, SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 32767);
    touch_poll_controller();
    assert(keys[77] && keys[72] && keys[31]);
    {
        SDL_Event e = {0};
        e.type = SDL_APP_WILLENTERBACKGROUND;
        sorr_ios_d4a_process_sdl_event(&e);
        touch_poll_controller();
        assert(!keys[77] && !keys[31]);
        e.type = SDL_APP_DIDENTERFOREGROUND;
        sorr_ios_d4a_process_sdl_event(&e);
    }
    SDL_JoystickDetachVirtual(device);
    touch_poll_controller();
    assert(!touch_controller);
    for (i=0; i<127; ++i) assert(!keys[i]);
    finger(7, TOUCH_COMBO, SDL_FINGERDOWN);
    assert(keys[31]); /* Touch becomes usable immediately after disconnect. */
    finger(7, TOUCH_COMBO, SDL_FINGERUP);
    SDL_JoystickClose(joystick);
    touch_release_all();
    SDL_DestroyRenderer(renderer);
    SDL_FreeSurface(surface);
    SDL_Quit();
    puts("PASS: multi-touch, layout, DualSense map, held-input handoff, disconnect and lifecycle");
    return 0;
}
