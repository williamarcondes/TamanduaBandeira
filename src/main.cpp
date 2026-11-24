#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

#ifdef __SWITCH__
#include <switch.h>
#endif

namespace {
constexpr int screen_w = 1280;
constexpr int screen_h = 720;
constexpr int stage_count = 3;
constexpr int max_lives = 5;
constexpr int visible_beans_per_stage = 8;
constexpr int hidden_beans_per_stage = 4;
constexpr int beans_per_stage = visible_beans_per_stage + hidden_beans_per_stage;
constexpr int total_beans = stage_count * beans_per_stage;
constexpr float world_w = 4300.0F;
constexpr float ground_y = 622.0F;
constexpr float dig_duration = 0.64F;
constexpr float pi = 3.14159265F;

struct Texture { SDL_Texture* texture = nullptr; int w = 0; int h = 0; };
struct Platform {
    float x;
    float y;
    float w;
    float base_y = 0.0F;
    float travel = 0.0F;
    float speed = 0.0F;
    float phase = 0.0F;
    float previous_y = 0.0F;
    bool spring = false;
};
struct Ground { float x0; float x1; };
enum class EnemyKind { Brute, Gremlin, Ant, DigitalBug };
struct Enemy {
    float x;
    float min_x;
    float max_x;
    float speed;
    int hp;
    EnemyKind kind;
    bool alive = true;
    bool right = true;
};
struct Cache { float x; int bean_index; bool dug = false; };
struct Bean {
    float x;
    float y;
    bool found = false;
    bool revealed = true;
    bool popping = false;
    float pop_time = 0.0F;
};
struct Angel {
    float x = 0.0F;
    float y = 0.0F;
    float timer = 0.0F;
    float respawn_x = 0.0F;
    float respawn_y = 0.0F;
    bool active = false;
};
struct Heart { float x = 0.0F; float y = 0.0F; bool found = false; };
struct Boss {
    float x = 0.0F;
    int parts = 3;
    float hit_lock = 0.0F;
    float flash = 0.0F;
    bool active = false;
    bool defeated = false;
};
struct Player {
    float x = 72.0F;
    float y = ground_y - 114.0F;
    float vx = 0.0F;
    float vy = 0.0F;
    float digging = 0.0F;
    float coffee_boost = 0.0F;
    float coyote_timer = 0.0F;
    float respawn_invulnerable = 0.0F;
    int lives = max_lives;
    int beans = 0;
    int facing = 1;
    bool grounded = true;
    bool crouched = false;
    bool rolling = false;
    bool jump_attack_armed = false;
    bool finished = false;
};
struct Controls {
    float horizontal = 0.0F;
    bool jump = false;
    bool roll = false;
    bool dig = false;
    bool crouch = false;
    bool pause = false;
};

std::array<std::uint8_t, 7> glyph(char c) {
    switch (c) {
        case 'A': return {14,17,17,31,17,17,17}; case 'B': return {30,17,17,30,17,17,30};
        case 'C': return {14,17,16,16,16,17,14}; case 'D': return {30,17,17,17,17,17,30};
        case 'E': return {31,16,16,30,16,16,31}; case 'F': return {31,16,16,30,16,16,16};
        case 'G': return {14,17,16,23,17,17,15}; case 'H': return {17,17,17,31,17,17,17};
        case 'I': return {14,4,4,4,4,4,14}; case 'J': return {7,2,2,2,18,18,12};
        case 'K': return {17,18,20,24,20,18,17}; case 'L': return {16,16,16,16,16,16,31};
        case 'M': return {17,27,21,21,17,17,17}; case 'N': return {17,25,25,21,19,19,17};
        case 'O': return {14,17,17,17,17,17,14}; case 'P': return {30,17,17,30,16,16,16};
        case 'Q': return {14,17,17,17,21,18,13}; case 'R': return {30,17,17,30,20,18,17};
        case 'S': return {15,16,16,14,1,1,30}; case 'T': return {31,4,4,4,4,4,4};
        case 'U': return {17,17,17,17,17,17,14}; case 'V': return {17,17,17,17,17,10,4};
        case 'W': return {17,17,17,21,21,21,10}; case 'X': return {17,17,10,4,10,17,17};
        case 'Y': return {17,17,10,4,4,4,4}; case 'Z': return {31,1,2,4,8,16,31};
        case '0': return {14,17,19,21,25,17,14}; case '1': return {4,12,4,4,4,4,14};
        case '2': return {14,17,1,2,4,8,31}; case '3': return {30,1,1,14,1,1,30};
        case '4': return {2,6,10,18,31,2,2}; case '5': return {31,16,16,30,1,1,30};
        case '6': return {14,16,16,30,17,17,14}; case '7': return {31,1,2,4,8,8,8};
        case '8': return {14,17,17,14,17,17,14}; case '9': return {14,17,17,15,1,1,14};
        case ':': return {0,4,4,0,4,4,0}; case '.': return {0,0,0,0,0,12,12};
        case '-': return {0,0,0,31,0,0,0}; case '/': return {1,2,2,4,8,8,16};
        case '+': return {0,4,4,31,4,4,0}; case '!': return {4,4,4,4,4,0,4};
        default: return {0,0,0,0,0,0,0};
    }
}
void color(SDL_Renderer* r, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255) {
    SDL_SetRenderDrawColor(r, red, green, blue, alpha);
}
void rect(SDL_Renderer* r, float x, float y, float w, float h, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255) {
    const SDL_FRect box{x, y, w, h};
    color(r, red, green, blue, alpha);
    SDL_RenderFillRect(r, &box);
}
void text(SDL_Renderer* r, const std::string& value, int x, int y, int scale, Uint8 red, Uint8 green, Uint8 blue) {
    int cursor = x;
    for (char raw : value) {
        const char c = raw >= 'a' && raw <= 'z' ? static_cast<char>(raw - 'a' + 'A') : raw;
        const auto rows = glyph(c);
        for (int row = 0; row < 7; ++row) for (int col = 0; col < 5; ++col) {
            if ((rows[static_cast<std::size_t>(row)] & (1U << (4 - col))) != 0U)
                rect(r, static_cast<float>(cursor + col * scale), static_cast<float>(y + row * scale),
                     static_cast<float>(scale), static_cast<float>(scale), red, green, blue);
        }
        cursor += (c == ' ' ? 4 : 6) * scale;
    }
}
Texture load_texture(SDL_Renderer* r, const std::string& path, bool keyed, bool smooth = false) {
    Texture result;
    SDL_Surface* surface = SDL_LoadBMP(path.c_str());
    if (!surface) { std::fprintf(stderr, "Asset %s: %s\n", path.c_str(), SDL_GetError()); return result; }
    if (keyed) SDL_SetSurfaceColorKey(surface, true, SDL_MapSurfaceRGB(surface, 255, 0, 255));
    result.w = surface->w;
    result.h = surface->h;
    result.texture = SDL_CreateTextureFromSurface(r, surface);
    SDL_DestroySurface(surface);
    if (result.texture) {
        SDL_SetTextureScaleMode(result.texture, smooth ? SDL_SCALEMODE_LINEAR : SDL_SCALEMODE_NEAREST);
        SDL_SetTextureBlendMode(result.texture, SDL_BLENDMODE_BLEND);
    }
    return result;
}
void draw_texture(SDL_Renderer* r, const Texture& t, float x, float y, float w, float h, bool flip = false) {
    if (!t.texture) return;
    const SDL_FRect dst{x, y, w, h};
    SDL_RenderTextureRotated(r, t.texture, nullptr, &dst, 0.0, nullptr, flip ? SDL_FLIP_HORIZONTAL : SDL_FLIP_NONE);
}
std::string asset(const char* file) {
#ifdef __SWITCH__
    return std::string("romfs:/assets/") + file;
#else
    return std::string("assets/") + file;
#endif
}
Controls read_controls(SDL_Gamepad* pad, const bool* keys) {
    const auto held = [pad](SDL_GamepadButton button) { return pad && SDL_GetGamepadButton(pad, button); };
    const auto key = [keys](SDL_Scancode code) { return keys && keys[code]; };
#ifdef __SWITCH__
    // SDL's private libnx backend exposes labeled A/B/X/Y in raw indices 0..3.
    // Use those physical buttons directly so unknown controller face styles
    // cannot silently swap B (bottom) and A (right).
    SDL_Joystick* joystick = pad ? SDL_GetGamepadJoystick(pad) : nullptr;
    const auto physical = [joystick](int button) { return joystick && SDL_GetJoystickButton(joystick, button); };
#endif
    Controls c;
    const bool left = key(SDL_SCANCODE_LEFT) || key(SDL_SCANCODE_A) || held(SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    const bool right = key(SDL_SCANCODE_RIGHT) || key(SDL_SCANCODE_D) || held(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    c.horizontal = (right ? 1.0F : 0.0F) - (left ? 1.0F : 0.0F);
    if (pad) {
        const Sint16 axis = SDL_GetGamepadAxis(pad, SDL_GAMEPAD_AXIS_LEFTX);
        if (axis < -12000) c.horizontal = -1.0F;
        else if (axis > 12000) c.horizontal = 1.0F;
    }
#ifdef __SWITCH__
    c.jump = key(SDL_SCANCODE_SPACE) || key(SDL_SCANCODE_Z) || physical(1); // B
    c.roll = key(SDL_SCANCODE_LSHIFT) || key(SDL_SCANCODE_Y) || physical(3); // Y
    c.dig = key(SDL_SCANCODE_X) || physical(0); // A
    c.crouch = key(SDL_SCANCODE_DOWN) || physical(13); // D-pad down
    c.pause = key(SDL_SCANCODE_ESCAPE) || physical(9); // +
#else
    c.jump = key(SDL_SCANCODE_SPACE) || key(SDL_SCANCODE_Z) || held(SDL_GAMEPAD_BUTTON_SOUTH);
    c.roll = key(SDL_SCANCODE_LSHIFT) || key(SDL_SCANCODE_Y) || held(SDL_GAMEPAD_BUTTON_WEST);
    c.dig = key(SDL_SCANCODE_X) || held(SDL_GAMEPAD_BUTTON_EAST);
    c.crouch = key(SDL_SCANCODE_DOWN) || held(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    c.pause = key(SDL_SCANCODE_ESCAPE) || held(SDL_GAMEPAD_BUTTON_START);
#endif
    return c;
}
bool overlaps(float ax, float ay, float aw, float ah, float bx, float by, float bw, float bh) {
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}
void panel(SDL_Renderer* r, float x, float y, float w, float h) {
    rect(r,x+6,y+7,w,h,3,12,19,190);
    rect(r,x,y,w,h,48,105,119,240);
    rect(r,x+2,y+2,w-4,h-4,8,28,43,255);
    rect(r,x+5,y+5,w-10,h-10,13,39,55,255);
    rect(r,x+8,y+7,w-16,2,241,192,103,225);
    rect(r,x+8,y+h-9,w-16,2,44,91,107,230);
}
void bean_sprite(SDL_Renderer* r, float x, float y, float scale = 1.45F) {
    // Grao oval com casca brilhante, sombra e vinco lateral curvo.
    const float oy=y-8.0F*scale;
    const auto bean_rect=[&](float rx,float ry,float rw,float rh,Uint8 red,Uint8 green,Uint8 blue) {
        rect(r,x+rx*scale,oy+ry*scale,rw*scale,rh*scale,red,green,blue);
    };
    bean_rect(8,0,9,2,49,25,20); bean_rect(5,2,15,3,49,25,20);
    bean_rect(2,5,21,4,49,25,20); bean_rect(1,9,23,9,49,25,20);
    bean_rect(2,18,21,4,49,25,20); bean_rect(5,22,15,3,49,25,20);
    bean_rect(9,25,8,2,49,25,20);
    bean_rect(7,3,12,3,133,71,39); bean_rect(4,6,16,4,173,102,54);
    bean_rect(3,10,17,5,151,78,41); bean_rect(4,15,16,5,119,56,32);
    bean_rect(7,20,11,3,94,41,27); bean_rect(5,8,3,8,209,143,82);
    bean_rect(9,6,3,3,230,174,111);
    bean_rect(14,5,2,4,87,40,28); bean_rect(12,8,2,5,76,33,25);
    bean_rect(11,12,2,5,67,29,23); bean_rect(12,17,2,4,75,34,24);
    bean_rect(14,21,2,2,87,40,28);
    bean_rect(7,11,3,3,204,125,68); bean_rect(7,15,3,2,164,87,47);
}
void heart_sprite(SDL_Renderer* r, float x, float y) {
    rect(r,x+4,y,7,5,235,68,89);
    rect(r,x+16,y,7,5,235,68,89);
    rect(r,x+1,y+4,25,8,213,48,73);
    rect(r,x+4,y+12,19,5,190,38,64);
    rect(r,x+8,y+17,11,5,167,31,56);
    rect(r,x+11,y+22,5,4,143,25,47);
    rect(r,x+6,y+3,5,3,255,166,160);
}
float item_y(const Bean& b) {
    return b.y - (b.popping ? std::sin(b.pop_time * pi) * 58.0F : 0.0F);
}
void dig_mark(SDL_Renderer* r, float x, float y, bool dug, int stage) {
    if (dug) {
        rect(r,x-35,y-3,70,4,47,35,28,220);
        rect(r,x-30,y-7,60,5,28,24,26,235);
        rect(r,x-24,y-10,48,4,17,19,23,245);
        rect(r,x-18,y-12,36,3,11,15,20,255);
        rect(r,x-31,y-8,7,3,126,82,48);
        rect(r,x+24,y-9,8,3,145,91,52);
        if(stage==0) {
            rect(r,x-39,y-8,12,4,69,142,57);
            rect(r,x+27,y-8,12,4,69,142,57);
        } else if(stage==2) {
            rect(r,x-22,y-11,44,2,98,203,218);
        }
        return;
    }
    if(stage==0) {
        rect(r,x-31,y-7,62,6,107,68,40);
        rect(r,x-25,y-11,50,5,147,91,48);
        rect(r,x-17,y-14,34,4,177,115,60);
        rect(r,x-34,y-10,11,3,81,155,63);
        rect(r,x+23,y-11,11,3,81,155,63);
        rect(r,x-8,y-17,3,5,65,46,35);
        rect(r,x+5,y-17,3,5,65,46,35);
        rect(r,x-20,y-6,7,2,124,177,78);
        rect(r,x+13,y-6,7,2,124,177,78);
    } else if(stage==1) {
        rect(r,x-32,y-7,64,6,129,91,58);
        rect(r,x-25,y-11,50,4,191,150,89);
        rect(r,x-14,y-14,28,3,225,190,119);
        rect(r,x-17,y-9,3,6,91,67,52);
        rect(r,x+10,y-13,3,7,91,67,52);
        rect(r,x-3,y-17,3,5,91,67,52);
    } else {
        rect(r,x-32,y-7,64,6,39,77,90);
        rect(r,x-26,y-11,52,4,73,155,166);
        rect(r,x-17,y-14,34,3,117,207,210);
        rect(r,x-15,y-9,3,7,24,52,65);
        rect(r,x+12,y-13,3,9,24,52,65);
        rect(r,x-4,y-17,8,3,123,238,222);
    }
}
void coffee_bottle(SDL_Renderer* r, float x, float base, float scale = 1.0F) {
    rect(r, x + 7*scale, base - 9*scale, 40*scale, 8*scale, 61, 39, 25);
    rect(r, x + 13*scale, base - 91*scale, 28*scale, 12*scale, 116, 55, 32);
    rect(r, x + 17*scale, base - 103*scale, 20*scale, 14*scale, 181, 70, 45);
    rect(r, x + 6*scale, base - 81*scale, 42*scale, 73*scale, 93, 49, 31);
    rect(r, x + 10*scale, base - 76*scale, 7*scale, 54*scale, 146, 82, 42);
    rect(r, x + 10*scale, base - 58*scale, 34*scale, 29*scale, 227, 195, 126);
    text(r, "CAFE", static_cast<int>(x + 14*scale), static_cast<int>(base - 49*scale),
         std::max(1, static_cast<int>(scale)), 95, 54, 31);
}
void guardian(SDL_Renderer* r, const Texture& hero, float x, float y, float w, float h, float phase) {
    const float flap = std::sin(phase * 12.0F) * 7.0F;
    const float wy = y + h * 0.42F + flap;
    rect(r, x - w*.12F, wy + 7, w*.25F, h*.16F, 160,181,205);
    rect(r, x - w*.16F, wy, w*.21F, h*.14F, 239,245,250);
    rect(r, x - w*.22F, wy + h*.06F, w*.16F, h*.11F, 255,255,255);
    rect(r, x + w*.95F, wy + 7, w*.25F, h*.16F, 160,181,205);
    rect(r, x + w*.95F, wy, w*.21F, h*.14F, 239,245,250);
    rect(r, x + w*1.06F, wy + h*.06F, w*.16F, h*.11F, 255,255,255);
    const float hx = x + w*.34F;
    rect(r, hx, y - 7, w*.34F, 5, 255,225,127);
    rect(r, hx + 4, y - 11, w*.34F - 8, 4, 255,246,190);
    draw_texture(r, hero, x, y, w, h);
    const float sx = x + w*(.16F + .68F*(.5F + .5F*std::sin(phase*5.0F)));
    rect(r, sx, y + h*.27F, 4, 4, 255,232,156);
}
void digital_bug(SDL_Renderer* r, float x, float base, float phase, bool facing_right) {
    const float flicker=std::sin(phase*17.0F)*2.0F;
    rect(r,x+9,base-5,52,5,8,20,34,130);
    rect(r,x+4,base-27,13,5,26,82,105);
    rect(r,x+52,base-27,13,5,26,82,105);
    rect(r,x+10,base-17,5,13,65,190,195);
    rect(r,x+27,base-17,5,13,65,190,195);
    rect(r,x+50,base-17,5,13,65,190,195);
    rect(r,x+13,base-49,42,29,18,49,77);
    rect(r,x+18,base-55,31,12,38,142,175);
    rect(r,x+17,base-45,34,19,37,124,146);
    rect(r,x+24,base-40,19,10,30,54,74);
    const float eye_x=x+(facing_right?37.0F:23.0F);
    rect(r,eye_x,base-38,6,6,255,236,139);
    rect(r,eye_x+1,base-37,3,3,255,255,226);
    rect(r,x+21,base-58,4,8,75,218,226);
    rect(r,x+43,base-59,4,9,75,218,226);
    rect(r,x+18+flicker,base-31,5,3,122,245,220);
    rect(r,x+47-flicker,base-25,6,3,177,105,255);
    rect(r,x+30,base-24,4,4,238,246,255);
}
float queen_spring_offset(float phase) {
    return 10.0F + std::abs(std::sin(phase * 3.4F)) * 18.0F;
}
float boss_top_y(int stage, float base, float phase) {
    return base - 112.0F - (stage == 0 ? queen_spring_offset(phase) : 0.0F);
}
void boss_sprite(SDL_Renderer* r, int stage, float x, float base, int parts, float phase, bool flash) {
    rect(r,x+18,base-5,158,5,8,18,28,145);
    if(stage==0) {
        const float bob=queen_spring_offset(phase);
        const float head_y=base-110.0F-bob;
        // Six articulated legs, wide abdomen and segmented thorax make the queen read as an ant.
        rect(r,x+50,base-35,8,25,57,28,25); rect(r,x+35,base-13,24,5,82,39,27);
        rect(r,x+91,base-35,8,28,57,28,25); rect(r,x+80,base-11,24,5,82,39,27);
        rect(r,x+144,base-36,8,25,57,28,25); rect(r,x+143,base-14,24,5,82,39,27);
        rect(r,x+62,base-55,65,33,75,28,25);
        rect(r,x+29,base-53,63,38,111,42,31);
        rect(r,x+34,base-49,52,29,151,57,38);
        rect(r,x+40,base-46,39,4,185,80,49);
        rect(r,x+42,base-40,37,3,96,35,28);
        rect(r,x+49,base-34,27,3,96,35,28);
        rect(r,x+82,base-54,40,23,133,49,33);
        rect(r,x+88,base-51,27,17,181,72,42);
        // A visible spring neck flexes below the head.
        const float coil_top=head_y+35.0F;
        const float coil_height=std::max(8.0F,base-60.0F-coil_top);
        rect(r,x+98,coil_top,4,coil_height,78,35,27);
        rect(r,x+122,coil_top,4,coil_height,78,35,27);
        for(int i=0;i<5;++i) {
            const float t=static_cast<float>(i)/4.0F;
            const float cy=coil_top+t*coil_height;
            rect(r,x+(i%2==0?98.0F:105.0F),cy,28,4,207,112,54);
            rect(r,x+(i%2==0?101.0F:108.0F),cy+1,18,2,255,177,88);
        }
        // Head, compound eyes, mandibles, antennae and a small queen's crown.
        rect(r,x+87,head_y+6,51,27,75,28,25);
        rect(r,x+92,head_y+2,42,29,150,55,37);
        rect(r,x+98,head_y,29,6,191,78,47);
        rect(r,x+88,head_y+17,51,12,177,64,42);
        rect(r,x+92,head_y+13,13,12,247,221,164);
        rect(r,x+119,head_y+13,13,12,247,221,164);
        rect(r,x+97,head_y+15,7,8,44,25,22); rect(r,x+124,head_y+15,7,8,44,25,22);
        rect(r,x+99,head_y+16,3,3,255,255,225); rect(r,x+126,head_y+16,3,3,255,255,225);
        rect(r,x+96,head_y-5,4,9,83,35,28); rect(r,x+95,head_y-9,13,4,115,49,34);
        rect(r,x+125,head_y-5,4,9,83,35,28); rect(r,x+121,head_y-9,13,4,115,49,34);
        rect(r,x+103,head_y+27,8,11,82,31,27); rect(r,x+123,head_y+27,8,11,82,31,27);
        rect(r,x+101,head_y-13,6,5,255,211,108); rect(r,x+113,head_y-17,6,9,255,211,108);
        rect(r,x+125,head_y-13,6,5,255,211,108);
        if(parts>0) rect(r,x+33,base-49,16,20,207,91,49);
        if(parts>1) rect(r,x+53,base-46,17,19,218,112,58);
        if(parts>2) rect(r,x+73,base-48,14,20,199,77,43);
    } else if(stage==1) {
        rect(r,x+34,base-40,36,24,86,83,76);
        rect(r,x+120,base-40,36,24,86,83,76);
        rect(r,x+48,base-83,102,66,115,112,99);
        rect(r,x+66,base-105,66,31,142,138,121);
        rect(r,x+80,base-96,8,7,255,207,103);
        rect(r,x+112,base-96,8,7,255,207,103);
        rect(r,x+91,base-80,22,5,54,52,48);
        rect(r,x+24,base-47,24,10,75,72,67);
        rect(r,x+145,base-47,25,10,75,72,67);
        rect(r,x+18,base-38,10,22,91,87,78);
        rect(r,x+157,base-38,10,22,91,87,78);
        if(parts>0) rect(r,x+51,base-68,26,17,174,166,142);
        if(parts>1) rect(r,x+86,base-68,27,17,184,172,145);
        if(parts>2) rect(r,x+122,base-68,26,17,174,166,142);
        rect(r,x+90,base-116,14,13,103,100,89);
        rect(r,x+79,base-119,36,5,116,114,104);
    } else {
        const Uint8 core=flash?255:38, glow=flash?250:221;
        rect(r,x+35,base-41,111,39,16,38,62);
        rect(r,x+48,base-83,87,56,19,65,93);
        rect(r,x+63,base-105,57,31,36,105,137);
        rect(r,x+70,base-94,9,8,glow,255,227);
        rect(r,x+104,base-94,9,8,glow,255,227);
        rect(r,x+84,base-73,31,25,core,glow,255);
        rect(r,x+89,base-69,21,17,19,32,52);
        rect(r,x+18,base-57,27,9,47,167,188);
        rect(r,x+139,base-57,29,9,47,167,188);
        rect(r,x+23,base-49,9,23,38,111,135);
        rect(r,x+153,base-49,9,23,38,111,135);
        if(parts>0) rect(r,x+45,base-75,20,18,117,243,222);
        if(parts>1) rect(r,x+83,base-82,20,20,154,240,250);
        if(parts>2) rect(r,x+123,base-75,20,18,117,243,222);
        const float jitter=std::sin(phase*21.0F)*5.0F;
        rect(r,x+33+jitter,base-90,12,3,221,110,255);
        rect(r,x+140-jitter,base-37,14,3,78,231,241);
    }
    if(flash) {
        rect(r,x+75,base-123,37,3,255,252,225);
        rect(r,x+7,base-72,13,3,255,255,238);
        rect(r,x+165,base-68,14,3,255,255,238);
    }
}
} // namespace

int main(int, char**) {
#ifdef __SWITCH__
    if (R_FAILED(romfsInit())) { std::fprintf(stderr, "Falha ao montar RomFS.\n"); return 1; }
#endif
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD)) {
        std::fprintf(stderr, "SDL: %s\n", SDL_GetError());
#ifdef __SWITCH__
        romfsExit();
#endif
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Tamandua Bandeira", screen_w, screen_h, SDL_WINDOW_FULLSCREEN);
    SDL_Renderer* renderer = window ? SDL_CreateRenderer(window, nullptr) : nullptr;
    if (!window || !renderer) {
        std::fprintf(stderr, "Renderer: %s\n", SDL_GetError());
        if (renderer) SDL_DestroyRenderer(renderer);
        if (window) SDL_DestroyWindow(window);
        SDL_Quit();
#ifdef __SWITCH__
        romfsExit();
#endif
        return 1;
    }
    SDL_SetRenderLogicalPresentation(renderer, screen_w, screen_h, SDL_LOGICAL_PRESENTATION_LETTERBOX);
    SDL_SetRenderVSync(renderer, 1);

    SDL_AudioStream* music_stream = nullptr;
    Uint8* music_data = nullptr;
    Uint32 music_length = 0;
    SDL_AudioSpec music_spec{};
    if (SDL_InitSubSystem(SDL_INIT_AUDIO)) {
        if (SDL_LoadWAV(asset("rock-guitarra.wav").c_str(), &music_spec, &music_data, &music_length)) {
            music_stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &music_spec, nullptr, nullptr);
            if (!music_stream || !SDL_PutAudioStreamData(music_stream, music_data, static_cast<int>(music_length)) ||
                !SDL_ResumeAudioStreamDevice(music_stream)) {
                std::fprintf(stderr, "Audio desativado: %s\n", SDL_GetError());
                if (music_stream) SDL_DestroyAudioStream(music_stream);
                music_stream = nullptr;
                SDL_free(music_data);
                music_data = nullptr;
                music_length = 0;
            }
        } else {
            std::fprintf(stderr, "Trilha nao encontrada: %s\n", SDL_GetError());
        }
    } else {
        std::fprintf(stderr, "Audio indisponivel: %s\n", SDL_GetError());
    }

    const std::array<Texture, stage_count> backgrounds{{
        load_texture(renderer, asset("mata.bmp"), false, true),
        load_texture(renderer, asset("cidade.bmp"), false, true),
        load_texture(renderer, asset("escritorio.bmp"), false, true)
    }};
    std::array<Texture, 8> hero{};
    std::array<Texture, 4> digging{};
    std::array<Texture, 4> brute{};
    std::array<Texture, 4> gremlin{};
    std::array<Texture, 4> ants{};
    const std::array<const char*, 8> hero_names{"tamandua_idle.bmp","tamandua_run1.bmp","tamandua_run2.bmp","tamandua_jump.bmp",
        "tamandua_crouch.bmp","tamandua_dig.bmp","tamandua_ball.bmp","tamandua_roll.bmp"};
    const std::array<const char*, 4> dig_names{"tamandua_dig_1.bmp","tamandua_dig_2.bmp","tamandua_dig_3.bmp","tamandua_dig_4.bmp"};
    const std::array<const char*, 4> brute_names{"bruto_idle.bmp","bruto_walk.bmp","bruto_attack.bmp","bruto_hit.bmp"};
    const std::array<const char*, 4> gremlin_names{"grilo_idle.bmp","grilo_walk.bmp","grilo_jump.bmp","grilo_ball.bmp"};
    const std::array<const char*, 4> ant_names{"formiga_idle.bmp","formiga_walk1.bmp","formiga_walk2.bmp","formiga_bite.bmp"};
    for (std::size_t i=0; i<hero.size(); ++i) hero[i]=load_texture(renderer,asset(hero_names[i]),true);
    for (std::size_t i=0; i<digging.size(); ++i) digging[i]=load_texture(renderer,asset(dig_names[i]),true);
    for (std::size_t i=0; i<brute.size(); ++i) brute[i]=load_texture(renderer,asset(brute_names[i]),true);
    for (std::size_t i=0; i<gremlin.size(); ++i) gremlin[i]=load_texture(renderer,asset(gremlin_names[i]),true);
    for (std::size_t i=0; i<ants.size(); ++i) ants[i]=load_texture(renderer,asset(ant_names[i]),true);

    int pad_count=0;
    SDL_JoystickID* pad_ids=SDL_GetGamepads(&pad_count);
    SDL_Gamepad* pad=pad_ids && pad_count>0 ? SDL_OpenGamepad(pad_ids[0]) : nullptr;
    SDL_free(pad_ids);

    const std::array<Ground,4> forest_grounds{{{0,1110},{1200,2140},{2230,3160},{3250,world_w}}};
    const std::array<Ground,4> city_grounds{{{0,830},{950,1850},{1970,2890},{3010,world_w}}};
    const std::array<Ground,4> office_grounds{{{0,900},{1020,1900},{2020,2920},{3040,world_w}}};
    std::array<Ground,4> grounds=forest_grounds;
    const std::array<Platform,8> forest_platforms{{{370,510,210},{780,430,205},{1430,510,205},{1800,442,220},
        {2520,500,220},{2870,425,230},{3620,500,205},{4050,510,170}}};
    const std::array<Platform,8> city_platforms{{{280,530,175},{540,455,190},{1110,520,190},{1410,440,185},
        {2150,525,190},{2450,455,190},{3190,520,200},{3500,445,200}}};
    const std::array<Platform,8> office_platforms{{{250,525,180},{490,455,185},{1160,525,190},{1430,450,190},
        {2180,525,190},{2460,450,190},{3190,525,195},{3470,450,200}}};
    std::array<Platform,8> platforms=forest_platforms;
    const std::array<Enemy,11> forest_enemies{{
        {310,250,350,118,1,EnemyKind::Ant},{530,495,685,66,2,EnemyKind::Brute},
        {750,690,800,132,1,EnemyKind::Ant},{960,915,1080,90,1,EnemyKind::Gremlin},
        {1570,1490,1715,72,2,EnemyKind::Brute},{1835,1760,1900,126,1,EnemyKind::Ant},
        {1985,1900,2090,100,1,EnemyKind::Gremlin},{2650,2550,2790,76,2,EnemyKind::Brute},
        {3010,2920,3100,136,1,EnemyKind::Ant},{3480,3390,3570,105,1,EnemyKind::Gremlin},
        {3740,3650,3820,142,1,EnemyKind::Ant}}};
    const std::array<Enemy,11> city_enemies{{
        {330,275,400,108,1,EnemyKind::Ant},{625,545,730,94,1,EnemyKind::Gremlin},
        {1080,1000,1165,126,1,EnemyKind::Ant},{1395,1320,1470,92,1,EnemyKind::Gremlin},
        {1710,1635,1815,72,2,EnemyKind::Brute},{2075,2010,2170,122,1,EnemyKind::Ant},
        {2350,2275,2430,101,1,EnemyKind::Gremlin},{2770,2690,2855,132,1,EnemyKind::Ant},
        {3200,3120,3280,75,2,EnemyKind::Brute},{3500,3430,3590,108,1,EnemyKind::Gremlin},
        {3860,3770,3950,138,1,EnemyKind::Ant}}};
    const std::array<Enemy,11> office_enemies{{
        {300,250,390,118,1,EnemyKind::DigitalBug},{600,530,735,128,1,EnemyKind::DigitalBug},
        {1100,1035,1190,112,1,EnemyKind::DigitalBug},{1375,1300,1465,106,1,EnemyKind::DigitalBug},
        {1740,1660,1840,94,1,EnemyKind::DigitalBug},{2110,2040,2195,122,1,EnemyKind::DigitalBug},
        {2430,2350,2525,118,1,EnemyKind::DigitalBug},{2780,2700,2870,132,1,EnemyKind::DigitalBug},
        {3190,3110,3290,100,1,EnemyKind::DigitalBug},{3480,3410,3590,121,1,EnemyKind::DigitalBug},
        {3830,3750,3950,140,1,EnemyKind::DigitalBug}}};
    std::array<Enemy,11> enemies=forest_enemies;
    const std::array<float,visible_beans_per_stage> forest_bean_x{{180,450,850,1035,1350,1760,2380,3010}};
    const std::array<float,visible_beans_per_stage> city_bean_x{{180,420,735,1085,1370,1690,2240,3170}};
    const std::array<float,visible_beans_per_stage> office_bean_x{{180,470,790,1130,1450,1780,2240,3200}};
    const std::array<float,hidden_beans_per_stage> forest_cache_x{{690,1690,2450,3500}};
    const std::array<float,hidden_beans_per_stage> city_cache_x{{690,1510,2500,3510}};
    const std::array<float,hidden_beans_per_stage> office_cache_x{{710,1570,2500,3550}};
    std::array<Cache,4> caches{{{690,8,false},{1690,9,false},{2450,10,false},{3500,11,false}}};
    std::array<Bean,beans_per_stage> beans{{
        {180,ground_y-34},{450,ground_y-37},{850,ground_y-34},{1035,ground_y-34},
        {1350,ground_y-34},{1760,ground_y-34},{2380,ground_y-34},{3010,ground_y-34},
        {690,ground_y-36,false,false,false,0},{1690,ground_y-36,false,false,false,0},
        {2450,ground_y-36,false,false,false,0},{3500,ground_y-36,false,false,false,0}}};

    Player player;
    Angel angel;
    Heart heart;
    Boss boss;
    bool game_over=false;
    bool bottle_collected=false;
    bool celebrating=false;
    float celebration_timer=0.0F;
    SDL_Event event{};
    bool running=true, title_screen=true, paused=false, prev_jump=false, prev_dig=false, prev_pause=false;
    float camera=0, elapsed=0;
    int current_stage=0;
    float stage_banner_timer=3.2F;
    float calm_start_timer=0.0F;
    const std::array<const char*,3> stage_names{"MATA DE POCOS","POCOS DE CALDAS","PREDIO DE VIDRO"};
    const std::array<const char*,3> stage_banners{"FASE 1 - MATA DE POCOS","FASE 2 - POCOS DE CALDAS","FASE 3 - ESCRITORIO DE PROGRAMACAO"};
    const std::array<const char*,3> stage_objectives{
        "DERROTE A RAINHA E PEGUE A GARRAFA DE CAFE",
        "VENCA O GUARDIAO E PEGUE A GARRAFA DE CAFE",
        "DERROTE O BUG E PEGUE A GARRAFA DE CAFE"
    };
    const std::array<float,stage_count> heart_positions{{2280.0F,2440.0F,2420.0F}};
    const auto reset_stage=[&]() {
        grounds=current_stage==0?forest_grounds:current_stage==1?city_grounds:office_grounds;
        platforms=current_stage==0?forest_platforms:current_stage==1?city_platforms:office_platforms;
        enemies=current_stage==0?forest_enemies:current_stage==1?city_enemies:office_enemies;
        const auto& bean_x=current_stage==0?forest_bean_x:current_stage==1?city_bean_x:office_bean_x;
        const auto& cache_x=current_stage==0?forest_cache_x:current_stage==1?city_cache_x:office_cache_x;
        for (std::size_t i=0;i<platforms.size();++i) {
            platforms[i].base_y=platforms[i].y;
            platforms[i].previous_y=platforms[i].y;
            platforms[i].travel=0.0F; platforms[i].speed=0.0F; platforms[i].phase=0.0F;
        }
        if (current_stage==stage_count-1) {
            platforms[2].travel=52.0F; platforms[2].speed=0.82F; platforms[2].phase=0.4F;
            platforms[5].travel=45.0F; platforms[5].speed=0.68F; platforms[5].phase=pi;
        }
        const std::size_t spring_index=current_stage==0?4U:current_stage==1?2U:4U;
        platforms[spring_index].spring=true;
        player.x=72.0F; player.y=ground_y-114.0F; player.vx=0.0F; player.vy=0.0F;
        player.coffee_boost=0.0F;
        player.coyote_timer=0.0F; player.respawn_invulnerable=0.0F;
        player.grounded=true; player.crouched=false; player.rolling=false; player.jump_attack_armed=false; player.digging=0.0F;
        angel.active=false;
        bottle_collected=false; celebrating=false; celebration_timer=0.0F;
        boss.x=world_w-430.0F; boss.parts=3; boss.hit_lock=0.0F; boss.flash=0.0F;
        boss.active=false; boss.defeated=false;
        heart.x=heart_positions[static_cast<std::size_t>(current_stage)];
        heart.y=ground_y-28.0F; heart.found=false;
        for (auto& foe:enemies) {
            foe.alive=true; foe.right=true;
            foe.x=(foe.min_x+foe.max_x)*0.5F;
            foe.hp=1;
        }
        for (auto& bean:beans) {
            bean.found=false; bean.popping=false; bean.pop_time=0.0F;
            bean.revealed=true;
        }
        for (std::size_t i=visible_beans_per_stage;i<beans.size();++i) beans[i].revealed=false;
        for (std::size_t i=0;i<visible_beans_per_stage;++i) {
            beans[i].x=bean_x[i]; beans[i].y=ground_y-34.0F;
        }
        for (std::size_t i=0;i<caches.size();++i) {
            caches[i].x=cache_x[i]; caches[i].bean_index=visible_beans_per_stage+static_cast<int>(i); caches[i].dug=false;
            Bean& bean=beans[static_cast<std::size_t>(caches[i].bean_index)];
            bean.x=cache_x[i]; bean.y=ground_y-36.0F;
        }
        stage_banner_timer=4.5F;
    };
    reset_stage();
    const auto respawn_near_death=[&]() {
        float best_score=1.0e9F;
        float safe_x=72.0F;
        float safe_y=ground_y-114.0F;
        for (const auto& ground:grounds) {
            const float min_x=ground.x0+8.0F;
            const float max_x=ground.x1-65.0F;
            if (max_x<min_x) continue;
            const float candidate_x=std::clamp(angel.respawn_x,min_x,max_x);
            const float candidate_y=ground_y-114.0F;
            const float score=std::abs(candidate_x-angel.respawn_x)+
                std::abs(candidate_y-angel.respawn_y)*0.8F;
            if (score<best_score) { best_score=score; safe_x=candidate_x; safe_y=candidate_y; }
        }
        for (const auto& platform:platforms) {
            const float platform_y=platform.y-114.0F;
            if (std::abs(platform_y-angel.respawn_y)>90.0F ||
                angel.respawn_x+52.0F<platform.x || angel.respawn_x+8.0F>platform.x+platform.w) continue;
            const float candidate_x=std::clamp(angel.respawn_x,platform.x-42.0F,platform.x+platform.w-10.0F);
            const float score=std::abs(candidate_x-angel.respawn_x)+std::abs(platform_y-angel.respawn_y)*0.8F;
            if (score<best_score) { best_score=score; safe_x=candidate_x; safe_y=platform_y; }
        }
        player.x=safe_x; player.y=safe_y; player.vx=0.0F; player.vy=0.0F;
        player.grounded=true; player.crouched=false; player.rolling=false;
        player.coyote_timer=0.18F; player.respawn_invulnerable=2.0F;
    };
    const auto start_death=[&]() {
        if (angel.active || game_over || player.finished) return;
        if (player.lives>0) --player.lives;
        angel.active=true;
        angel.x=player.x+30.0F;
        angel.y=std::min(player.y+120.0F,static_cast<float>(screen_h)+80.0F);
        angel.timer=0.0F;
        angel.respawn_x=player.x; angel.respawn_y=player.y;
        player.vx=0.0F; player.vy=0.0F; player.digging=0.0F;
        player.coffee_boost=0.0F; player.rolling=false; player.crouched=false; player.jump_attack_armed=false;
    };
    Uint64 previous=SDL_GetTicks();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type==SDL_EVENT_QUIT) running=false;
            else if (event.type==SDL_EVENT_GAMEPAD_ADDED && !pad) pad=SDL_OpenGamepad(event.gdevice.which);
            else if (event.type==SDL_EVENT_GAMEPAD_REMOVED && pad && SDL_GetGamepadID(pad)==event.gdevice.which) {
                SDL_CloseGamepad(pad); pad=nullptr;
            }
        }
        Controls c=read_controls(pad,SDL_GetKeyboardState(nullptr));
        const Uint64 now=SDL_GetTicks();
        const float dt=std::min(0.04F,static_cast<float>(now-previous)/1000.0F);
        previous=now;
        elapsed+=dt;
        if (!paused && !title_screen) stage_banner_timer=std::max(0.0F,stage_banner_timer-dt);
        if (!paused && !title_screen && !angel.active)
            player.respawn_invulnerable=std::max(0.0F,player.respawn_invulnerable-dt);
        if (!paused && !title_screen && calm_start_timer>0.0F) {
            calm_start_timer=std::max(0.0F,calm_start_timer-dt);
            if (calm_start_timer==0.0F) {
                for (auto& foe:enemies)
                    if (std::abs(foe.x-player.x)<300.0F) foe.alive=false;
            }
        }
        if (music_stream && music_data && music_length>0 &&
            SDL_GetAudioStreamAvailable(music_stream)<static_cast<int>(music_length/2U)) {
            SDL_PutAudioStreamData(music_stream,music_data,static_cast<int>(music_length));
        }
        if (title_screen) {
            if (c.jump && !prev_jump) {
                title_screen=false;
                stage_banner_timer=4.5F;
                calm_start_timer=4.0F;
            }
            prev_jump=c.jump; prev_dig=c.dig; prev_pause=c.pause;
            camera=0.0F;
            color(renderer,9,28,44); SDL_RenderClear(renderer);
            const Texture& cover=backgrounds[0];
            if (cover.texture) draw_texture(renderer,cover,0,0,screen_w,screen_h);
            rect(renderer,0,0,screen_w,screen_h,4,17,31,144);
            panel(renderer,230,64,820,592);
            text(renderer,"TAMANDUA BANDEIRA",394,100,4,255,246,218);
            text(renderer,"UMA AVENTURA PELO SUL DE MINAS",392,151,2,177,226,208);
            draw_texture(renderer,hero[0],282,214,230,282);
            coffee_bottle(renderer,889,505,1.05F);
            bean_sprite(renderer,840,337);
            bean_sprite(renderer,1000,301);
            panel(renderer,552,218,422,89);
            text(renderer,"MATA  -  PRACA  -  ESCRITORIO",582,244,2,240,247,224);
            panel(renderer,552,322,422,89);
            text(renderer,"CAFE, PLATAFORMAS E BUGS",579,348,2,240,247,224);
            panel(renderer,295,406,690,111);
            text(renderer,"UMA TRILHA DE GRAOS LEVA O TAMANDUA",317,416,2,255,224,151);
            text(renderer,"DA MATA A CIDADE. NO PREDIO DE VIDRO",317,443,2,247,243,221);
            text(renderer,"TRES CHEFES GUARDAM O CAFE EXCELENTE!",317,470,2,247,243,221);
            panel(renderer,408,523,464,55);
            text(renderer,"B COMECAR",521,538,3,255,223,138);
            text(renderer,"B PULAR   A CAVAR   Y ROLAR COM TURBO",326,588,2,250,248,232);
            text(renderer,"BAIXO AGACHAR   DIRECIONAL MOVER",355,615,2,180,224,207);
            SDL_RenderPresent(renderer);
            continue;
        }
        bool restart_frame=false;
        if ((game_over || player.finished) && c.jump && !prev_jump) {
            game_over=false; player.finished=false; paused=false; title_screen=false;
            current_stage=0; player.lives=max_lives; player.beans=0;
            calm_start_timer=4.0F;
            reset_stage(); restart_frame=true;
        }
        if (!game_over && !player.finished && !celebrating && !angel.active && c.pause && !prev_pause) paused=!paused;
        prev_pause=c.pause;

        if (!paused && angel.active) {
            angel.timer+=dt;
            angel.y-=410.0F*dt;
            if (angel.timer>=1.65F) {
                angel.active=false;
                if (player.lives<=0) {
                    game_over=true;
                } else {
                    respawn_near_death();
                }
            }
        }
        if (!paused && celebrating) {
            celebration_timer+=dt;
            if (celebration_timer>=1.55F) {
                celebrating=false;
                if (current_stage<stage_count-1) {
                    ++current_stage;
                    reset_stage();
                    restart_frame=true;
                } else {
                    player.finished=true;
                }
            }
        }
        if (!paused && !angel.active && !game_over && !player.finished && !celebrating && !restart_frame) {
            for (auto& platform:platforms) {
                platform.previous_y=platform.y;
                if (platform.travel>0.0F)
                    platform.y=platform.base_y+std::sin(elapsed*platform.speed+platform.phase)*platform.travel;
            }
            const bool jump_pressed=c.jump && !prev_jump;
            const bool dig_pressed=c.dig && !prev_dig;
            if (player.grounded) player.coyote_timer=0.18F;
            else player.coyote_timer=std::max(0.0F,player.coyote_timer-dt);
            const float old_h=player.crouched?62.0F:player.rolling?74.0F:114.0F;
            player.rolling=c.roll && player.grounded && player.coffee_boost>0.0F;
            player.crouched=c.crouch && player.grounded && !player.rolling;
            const float h=player.crouched?62.0F:player.rolling?74.0F:114.0F;
            if (player.grounded) {
                player.y+=old_h-h;
                for (const auto& platform:platforms) {
                    if (platform.travel>0.0F && player.x+58>platform.x && player.x+2<platform.x+platform.w &&
                        std::abs((player.y+h)-platform.previous_y)<8.0F) {
                        player.y+=platform.y-platform.previous_y;
                    }
                }
            }
            if (c.horizontal!=0) player.facing=c.horizontal>0?1:-1;
            if (player.rolling) player.vx=static_cast<float>(player.facing)*510.0F;
            else if (player.crouched) player.vx=c.horizontal*80.0F;
            else player.vx=c.horizontal*265.0F;
            if (jump_pressed && (player.grounded || player.coyote_timer>0.0F) && !player.rolling) {
                player.vy=-840.0F;
                player.grounded=false;
                player.coyote_timer=0.0F;
                player.jump_attack_armed=true;
            }
            if (dig_pressed && player.grounded && !player.rolling) {
                player.digging=dig_duration;
                for (auto& cache:caches) if (!cache.dug && std::abs((player.x+30.0F)-cache.x)<78.0F) {
                    cache.dug=true;
                    Bean& bean=beans[static_cast<std::size_t>(cache.bean_index)];
                    bean.revealed=true;
                    bean.x=cache.x+(player.x+30.0F<cache.x?72.0F:-72.0F);
                    bean.y=ground_y-37.0F;
                    bean.popping=true;
                    bean.pop_time=0.0F;
                    break;
                }
            }
            player.digging=std::max(0.0F,player.digging-dt);
            if (player.rolling) player.coffee_boost=std::max(0.0F,player.coffee_boost-dt*2.4F);

            const float old_bottom=player.y+h;
            player.x=std::clamp(player.x+player.vx*dt,0.0F,world_w-70.0F);
            player.vy=std::min(player.vy+1700.0F*dt,1000.0F);
            player.y+=player.vy*dt;
            player.grounded=false;
            const float new_bottom=player.y+h;
            bool boss_top_contact=false;
            if (player.vy>=0) {
                for (const auto& ground:grounds) if (player.x+50>ground.x0 && player.x+10<ground.x1 &&
                    old_bottom<=ground_y+5 && new_bottom>=ground_y) {
                    player.y=ground_y-h; player.vy=0; player.grounded=true;
                }
                for (const auto& platform:platforms) if (player.x+58>platform.x && player.x+2<platform.x+platform.w &&
                    old_bottom<=platform.y+4 && new_bottom>=platform.y) {
                    player.y=platform.y-h;
                    if (platform.spring) {
                        player.vy=-880.0F; player.grounded=false; player.coyote_timer=0.0F; player.jump_attack_armed=true;
                    } else {
                        player.vy=0; player.grounded=true;
                    }
                    break;
                }
                if (boss.active && !boss.defeated) {
                    constexpr float boss_w=180.0F;
                    const float boss_top=boss_top_y(current_stage,ground_y,elapsed);
                    if (player.x+58>boss.x && player.x+2<boss.x+boss_w &&
                        old_bottom<=boss_top+4.0F && new_bottom>=boss_top) {
                        player.y=boss_top-h; player.vy=0; player.grounded=true;
                        boss_top_contact=true;
                    }
                }
            }
            if (player.y>screen_h+80) {
                start_death();
            }

            if (!angel.active) {
                for (auto& bean:beans) {
                    if (bean.found || !bean.revealed) continue;
                    if (bean.popping) {
                        bean.pop_time=std::min(1.0F,bean.pop_time+dt/dig_duration);
                        if (bean.pop_time>=1.0F) bean.popping=false;
                    }
                    if (overlaps(player.x+3,player.y,58,h,bean.x,item_y(bean)-10.0F,35,38)) {
                        bean.found=true; ++player.beans;
                        player.coffee_boost=std::min(12.0F,player.coffee_boost+8.0F);
                    }
                }
                if (!heart.found && player.lives<max_lives &&
                    overlaps(player.x+2,player.y+2,58,h-4,heart.x-2,heart.y,30,28)) {
                    heart.found=true;
                    ++player.lives;
                }

                if (!boss.active && !boss.defeated && player.x>world_w-1120.0F) {
                    boss.active=true;
                    for (auto& foe:enemies) if (foe.x>world_w-1000.0F) foe.alive=false;
                }
                boss.hit_lock=std::max(0.0F,boss.hit_lock-dt);
                boss.flash=std::max(0.0F,boss.flash-dt);
                if (calm_start_timer<=0.0F) for (auto& foe:enemies) {
                    if (!foe.alive) continue;
                    foe.x+=(foe.right?foe.speed:-foe.speed)*dt;
                    if (foe.x<foe.min_x) {foe.x=foe.min_x;foe.right=true;}
                    if (foe.x>foe.max_x) {foe.x=foe.max_x;foe.right=false;}
                    const bool big=foe.kind==EnemyKind::Brute;
                    const bool ant=foe.kind==EnemyKind::Ant;
                    const bool glitch=foe.kind==EnemyKind::DigitalBug;
                    const float ew=big?64.0F:ant?54.0F:glitch?54.0F:44.0F;
                    const float eh=big?92.0F:ant?40.0F:glitch?48.0F:54.0F;
                    const float ey=ground_y-eh;
                    if (player.respawn_invulnerable>0.0F) continue;
                    const float player_center=player.x+30.0F;
                    const bool stomp=player.vy>80.0F && old_bottom<=ey+38.0F && new_bottom>=ey &&
                        player_center>=foe.x-18.0F && player_center<=foe.x+ew+18.0F;
                    const bool body_contact=overlaps(player.x+12.0F,player.y+16.0F,36.0F,h-18.0F,
                        foe.x+5.0F,ey+4.0F,ew-10.0F,eh-5.0F);
                    if (stomp || (player.rolling && body_contact) || body_contact) {
                        if (player.rolling || stomp) {
                            --foe.hp; if (foe.hp<=0) foe.alive=false;
                            if (stomp) player.jump_attack_armed=false;
                            player.vy=stomp?-510.0F:-360.0F; player.grounded=false;
                            if (stomp) break;
                        } else {
                            start_death();
                            break;
                        }
                    }
                }

                if (!angel.active && boss.active && !boss.defeated) {
                    constexpr float boss_w=180.0F;
                    const float boss_top=boss_top_y(current_stage,ground_y,elapsed);
                    const float boss_h=ground_y-boss_top;
                    if (boss_top_contact && player.jump_attack_armed && boss.hit_lock<=0.0F) {
                        --boss.parts; boss.hit_lock=0.45F; boss.flash=0.24F;
                        player.jump_attack_armed=current_stage==0;
                        if(current_stage==0) { player.vy=-840.0F; player.grounded=false; }
                        if (boss.parts<=0) boss.defeated=true;
                    } else if (!boss_top_contact &&
                               player.respawn_invulnerable<=0.0F &&
                               overlaps(player.x+12.0F,player.y+16.0F,36.0F,h-18.0F,boss.x+8.0F,boss_top+12.0F,boss_w-16.0F,boss_h-12.0F)) {
                        start_death();
                    }
                }
                if (!angel.active && boss.defeated && !bottle_collected && player.grounded) {
                    const float bottle_x=world_w-200.0F;
                    if (overlaps(player.x+8.0F,player.y,52.0F,h,bottle_x+4.0F,ground_y-136.0F,67.0F,136.0F)) {
                        bottle_collected=true; celebrating=true; celebration_timer=0.0F;
                        player.vx=0.0F; player.vy=0.0F;
                    }
                }
            }
        }
        prev_jump=c.jump; prev_dig=c.dig;

        camera=std::clamp(player.x-static_cast<float>(screen_w)*0.38F,0.0F,world_w-screen_w);
        color(renderer,11,29,49); SDL_RenderClear(renderer);
        const Texture& background=backgrounds[static_cast<std::size_t>(current_stage)];
        if (background.texture) {
            const float bg_w=static_cast<float>(background.w)*1.3F;
            const float bg_h=static_cast<float>(background.h)*1.3F;
            const float bg_offset=std::min(camera*0.12F,std::max(0.0F,bg_w-static_cast<float>(screen_w)));
            draw_texture(renderer,background,-bg_offset,0,bg_w,bg_h);
        } else rect(renderer,0,0,screen_w,screen_h,44,105,153);

        const std::array<std::array<Uint8,3>,3> soil{{{{87,51,31}},{{121,76,49}},{{47,63,78}}}};
        const std::array<std::array<Uint8,3>,3> surface{{{{70,142,69}},{{166,124,67}},{{91,155,170}}}};
        const std::array<std::array<Uint8,3>,3> highlight{{{{119,183,78}},{{217,173,97}},{{157,211,218}}}};
        const std::size_t stage_color=static_cast<std::size_t>(current_stage);
        for (const auto& ground:grounds) {
            rect(renderer,ground.x0-camera,ground_y,ground.x1-ground.x0,98,soil[stage_color][0],soil[stage_color][1],soil[stage_color][2]);
            rect(renderer,ground.x0-camera,ground_y,ground.x1-ground.x0,13,surface[stage_color][0],surface[stage_color][1],surface[stage_color][2]);
            rect(renderer,ground.x0-camera,ground_y+13,ground.x1-ground.x0,6,highlight[stage_color][0],highlight[stage_color][1],highlight[stage_color][2]);
        }
        for (const auto& p:platforms) if (p.travel>0.0F) {
            const float rail_x=p.x-camera;
            const float rail_top=p.base_y-p.travel-18.0F;
            const float rail_h=p.travel*2.0F+34.0F;
            rect(renderer,rail_x+13,rail_top,5,rail_h,42,91,111,175);
            rect(renderer,rail_x+p.w-18,rail_top,5,rail_h,42,91,111,175);
            rect(renderer,rail_x+18,rail_top+4,p.w-36,3,133,218,224,150);
        }
        for (const auto& p:platforms) {
            const float x=p.x-camera;
            rect(renderer,x+12,p.y+28,p.w-24,9,12,23,29,185);
            rect(renderer,x+5,p.y+12,p.w-10,25,soil[stage_color][0],soil[stage_color][1],soil[stage_color][2]);
            rect(renderer,x+8,p.y+14,7,19,highlight[stage_color][0],highlight[stage_color][1],highlight[stage_color][2],110);
            rect(renderer,x+p.w-14,p.y+14,7,21,18,29,37,150);
            rect(renderer,x+12,p.y+33,p.w-24,3,18,27,33,185);
            rect(renderer,x,p.y+4,p.w,9,soil[stage_color][0],soil[stage_color][1],soil[stage_color][2]);
            rect(renderer,x+2,p.y,p.w-4,9,surface[stage_color][0],surface[stage_color][1],surface[stage_color][2]);
            rect(renderer,x+8,p.y+1,p.w-16,3,highlight[stage_color][0],highlight[stage_color][1],highlight[stage_color][2]);
            rect(renderer,x+2,p.y+5,6,3,highlight[stage_color][0],highlight[stage_color][1],highlight[stage_color][2]);
            if (current_stage==0) {
                for (float tuft=x+19;tuft<x+p.w-24;tuft+=54) {
                    rect(renderer,tuft,p.y-3,18,4,76,157,66);
                    rect(renderer,tuft+7,p.y-5,18,3,105,184,74);
                    rect(renderer,tuft+4,p.y+19,18,5,149,93,53);
                    rect(renderer,tuft+23,p.y+24,10,4,111,66,41);
                }
            } else if (current_stage==1) {
                for (float tile=x+25;tile<x+p.w-16;tile+=46) {
                    rect(renderer,tile,p.y+17,2,16,211,165,101,190);
                    rect(renderer,tile+4,p.y+20,24,3,218,176,115,170);
                }
            } else {
                rect(renderer,x+16,p.y+17,p.w-32,3,96,208,221,200);
                rect(renderer,x+22,p.y+24,24,7,130,223,236,130);
                rect(renderer,x+p.w-48,p.y+24,24,7,130,223,236,130);
                rect(renderer,x+12,p.y+34,p.w-24,2,63,103,118,210);
            }
            if (p.spring) {
                const float sx=x+p.w*.5F-30.0F;
                rect(renderer,sx,p.y-9,60,10,242,190,88);
                rect(renderer,sx+5,p.y-13,50,5,255,229,153);
                rect(renderer,sx+10,p.y-5,5,8,88,164,121);
                rect(renderer,sx+23,p.y-5,5,8,88,164,121);
                rect(renderer,sx+36,p.y-5,5,8,88,164,121);
                rect(renderer,sx+49,p.y-5,5,8,88,164,121);
            }
        }
        for (const auto& bean:beans) if (!bean.found && bean.revealed) bean_sprite(renderer,bean.x-camera,item_y(bean));
        for (const auto& cache:caches) dig_mark(renderer,cache.x-camera,ground_y,cache.dug,current_stage);
        if (!heart.found) heart_sprite(renderer,heart.x-camera,heart.y);
        if (boss.defeated && !bottle_collected)
            coffee_bottle(renderer,world_w-200.0F-camera,ground_y,1.28F);

        for (const auto& foe:enemies) {
            if (calm_start_timer>0.0F) continue;
            if (!foe.alive) continue;
            const bool big=foe.kind==EnemyKind::Brute, ant=foe.kind==EnemyKind::Ant;
            const bool glitch=foe.kind==EnemyKind::DigitalBug;
            const float ew=big?64.0F:ant?54.0F:glitch?54.0F:44.0F;
            const bool near=std::abs((player.x+30.0F)-(foe.x+ew*.5F))<210.0F;
            const int frame=static_cast<int>(elapsed*5.0F)%2;
            const int ant_frame=near?3:1+static_cast<int>(elapsed*8.0F)%2;
            const float base=ground_y-(ant?1.0F:big?0.0F:glitch?0.0F:4.0F);
            if (glitch) {
                const bool facing_right=near?player.x+30.0F>foe.x+ew*.5F:foe.right;
                digital_bug(renderer,foe.x-camera,base,elapsed,facing_right);
                continue;
            }
            const Texture& sprite=big?brute[static_cast<std::size_t>(frame)]:
                ant?ants[static_cast<std::size_t>(ant_frame)]:gremlin[static_cast<std::size_t>(frame)];
            const float target_h=big?146.0F:ant?66.0F:glitch?78.0F:80.0F;
            const float target_w=sprite.h>0?target_h*static_cast<float>(sprite.w)/static_cast<float>(sprite.h):target_h;
            const bool flip=near?player.x+30.0F<foe.x:!foe.right;
            draw_texture(renderer,sprite,foe.x-camera-target_w*.40F,base-target_h,target_w,target_h,flip);
        }

        if (boss.active && !boss.defeated) {
            boss_sprite(renderer,current_stage,boss.x-camera,ground_y,boss.parts,elapsed,boss.flash>0.0F);
            const float head_top=boss_top_y(current_stage,ground_y,elapsed);
            text(renderer,current_stage==0?"FORMIGA RAINHA":current_stage==1?"GUARDIAO DA PRACA":"BUG SUPREMO",
                 static_cast<int>(boss.x-camera+28.0F),static_cast<int>(head_top-22.0F),1,255,244,210);
            for (int part=0;part<3;++part)
                rect(renderer,boss.x-camera+63.0F+static_cast<float>(part)*21.0F,head_top-10.0F,16,7,
                     part<boss.parts?255:74,part<boss.parts?202:57,part<boss.parts?102:70);
        }

        const float ph=player.digging>0.0F?150.0F:player.crouched?108.0F:player.rolling?100.0F:180.0F;
        const float ch=player.crouched?62.0F:player.rolling?74.0F:114.0F;
        const float idle_phase=std::fmod(elapsed+0.8F,3.6F);
        const std::size_t frame=player.rolling?7U:player.crouched?4U:player.digging>0?5U:
            !player.grounded?3U:std::abs(player.vx)>10?(static_cast<int>(elapsed*10.0F)%2==0?1U:2U):
            (idle_phase<0.18F?1U:0U);
        std::size_t dig_frame=0;
        if (player.digging>0) dig_frame=std::min<std::size_t>(3U,static_cast<std::size_t>(
            std::clamp((dig_duration-player.digging)/dig_duration,0.0F,0.999F)*4.0F));
        const Texture& sprite=player.digging>0?digging[dig_frame]:hero[frame];
        const float pw=sprite.h>0?ph*static_cast<float>(sprite.w)/static_cast<float>(sprite.h):ph;
        const float base=player.y+ch;
        if (!angel.active && (player.respawn_invulnerable<=0.0F || static_cast<int>(elapsed*12.0F)%2==0))
            draw_texture(renderer,sprite,player.x-camera+30-pw*.5F,base-ph,pw,ph,player.facing<0);
        if (angel.active) {
            const Texture& spirit=hero[0];
            const float sh=120.0F, sw=spirit.h>0?sh*static_cast<float>(spirit.w)/static_cast<float>(spirit.h):sh;
            guardian(renderer,spirit,angel.x-camera-sw*.5F,angel.y-sh,sw,sh,angel.timer);
        }

        panel(renderer,28,22,385,81);
        text(renderer,"TAMANDUA BANDEIRA",48,40,3,245,241,218);
        text(renderer,stage_names[static_cast<std::size_t>(current_stage)],51,76,2,185,221,161);
        panel(renderer,976,22,276,106);
        text(renderer,"GRAOS",999,39,3,248,210,123);
        text(renderer,std::to_string(player.beans)+"/"+std::to_string(total_beans),1124,39,3,255,248,221);
        text(renderer,"VIDA",999,76,2,212,230,198);
        for (int i=0;i<player.lives;++i) {
            const float hx=1073.0F+static_cast<float>(i)*30.0F;
            rect(renderer,hx+4,73,10,8,224,91,69); rect(renderer,hx,78,23,10,224,91,69); rect(renderer,hx+6,88,11,5,224,91,69);
        }
        text(renderer,"ROLAR",999,103,2,248,210,123);
        rect(renderer,1072,104,155,11,38,65,60);
        const float boost=155.0F*std::clamp(player.coffee_boost/12.0F,0.0F,1.0F);
        if (boost>0) rect(renderer,1072,104,boost,11,239,184,77);

        if (calm_start_timer>0.0F && !paused) {
            panel(renderer,250,143,780,96);
            text(renderer,"AREA SEGURA PARA TREINAR",458,156,2,255,224,151);
            text(renderer,"OS MONSTROS AINDA NAO APARECERAM",340,191,2,247,243,221);
        } else if (stage_banner_timer>0.0F && !paused && !player.finished) {
            panel(renderer,312,143,656,100);
            text(renderer,stage_banners[static_cast<std::size_t>(current_stage)],336,160,2,255,244,210);
            text(renderer,"OBJETIVO: "+std::string(stage_objectives[static_cast<std::size_t>(current_stage)]),336,200,2,180,224,207);
        }

        if (game_over || paused || player.finished || celebrating) {
            rect(renderer,0,0,screen_w,screen_h,5,16,24,178);
            if (celebrating) {
                panel(renderer,210,164,860,394);
                draw_texture(renderer,hero[0],356,215,250,290);
                // A small upturned mouth makes the pickup read as a happy celebration.
                rect(renderer,559,311,13,3,63,32,25);
                rect(renderer,572,314,10,3,63,32,25);
                rect(renderer,582,311,8,3,63,32,25);
                text(renderer,"BOM TRABALHO!",652,222,3,255,241,213);
                text(renderer,"CAFE ENCONTRADO!",652,278,3,255,224,151);
                text(renderer,"UM CAFE EXCELENTE ESPERA POR VOCE",652,333,2,238,245,226);
                coffee_bottle(renderer,887,497,1.35F);
                rect(renderer,632,415,10,10,255,219,114);
                rect(renderer,674,447,7,7,151,221,155);
                rect(renderer,824,431,8,8,255,219,114);
            } else if (game_over) {
                panel(renderer,235,205,810,300);
                text(renderer,"FIM DE JOGO",464,245,4,255,244,210);
                text(renderer,"VOCE PERDEU AS CINCO VIDAS",370,323,2,186,222,170);
                text(renderer,"B RECOMECAR",499,407,3,255,223,138);
            } else if (player.finished) {
                panel(renderer,190,170,900,380);
                draw_texture(renderer,hero[0],230,238,200,240);
                text(renderer,"TAMANDUA VENCEU",493,207,3,255,244,210);
                text(renderer,"3 FASES CONCLUIDAS",493,257,2,186,222,170);
                text(renderer,"GRAOS: "+std::to_string(player.beans)+"/"+std::to_string(total_beans),493,300,3,255,218,126);
                text(renderer,"ENCONTROU UM CAFE EXCELENTE!",493,362,2,239,199,111);
                coffee_bottle(renderer,918,458,0.72F);
                text(renderer,"B RECOMECAR",529,485,2,255,223,138);
            } else {
                panel(renderer,205,168,870,384);
                text(renderer,"TRILHA PAUSADA",482,201,3,255,241,213);
                panel(renderer,300,267,680,61);
                text(renderer,stage_banners[static_cast<std::size_t>(current_stage)],326,288,2,188,225,219);
                panel(renderer,300,345,680,103);
                text(renderer,"OBJETIVO",326,359,2,241,192,103);
                text(renderer,std::string(stage_objectives[static_cast<std::size_t>(current_stage)]),326,390,2,255,246,225);
                panel(renderer,420,472,440,57);
                text(renderer,"+ CONTINUAR",493,490,2,255,223,138);
            }
        }
        SDL_RenderPresent(renderer);
    }

    if (pad) SDL_CloseGamepad(pad);
    for (auto& t:hero) if (t.texture) SDL_DestroyTexture(t.texture);
    for (auto& t:digging) if (t.texture) SDL_DestroyTexture(t.texture);
    for (auto& t:brute) if (t.texture) SDL_DestroyTexture(t.texture);
    for (auto& t:gremlin) if (t.texture) SDL_DestroyTexture(t.texture);
    for (auto& t:ants) if (t.texture) SDL_DestroyTexture(t.texture);
    for (const auto& background:backgrounds) if (background.texture) SDL_DestroyTexture(background.texture);
    if (music_stream) SDL_DestroyAudioStream(music_stream);
    if (music_data) SDL_free(music_data);
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
#ifdef __SWITCH__
    romfsExit();
#endif
    return 0;
}
