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
struct Hazard { float x0; float x1; };
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
    float min_x = 0.0F;
    float max_x = world_w;
    float speed = 70.0F;
    float direction = -1.0F;
    int parts = 3;
    float hit_lock = 0.0F;
    float flash = 0.0F;
    float attack_timer = 2.8F;
    float attack_animation = 0.0F;
    bool active = false;
    bool defeated = false;
};
struct CornProjectile {
    float x = 0.0F;
    float y = 0.0F;
    float vx = 0.0F;
    float vy = 0.0F;
    int kind = 0;
    bool alive = true;
};
struct HitEffect {
    float x = 0.0F;
    float y = 0.0F;
    float timer = 0.0F;
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
    int cursor=x;
    for (std::size_t i=0;i<value.size();++i) {
        const auto byte=static_cast<unsigned char>(value[i]);
        char c=static_cast<char>(byte);
        bool tilde=false;
        if (byte==0xC3U && i+1<value.size()) {
            const auto accent=static_cast<unsigned char>(value[i+1]);
            if (accent==0x83U || accent==0xA3U) { c='A'; tilde=true; ++i; }
            else if (accent==0x95U || accent==0xB5U) { c='O'; tilde=true; ++i; }
            else if (accent==0x89U || accent==0xA9U) { c='E'; ++i; }
            else if (accent==0x81U || accent==0xA1U) { c='A'; ++i; }
            else if (accent==0x93U || accent==0xB3U) { c='O'; ++i; }
            else if (accent==0x87U || accent==0xA7U) { c='C'; ++i; }
        }
        if (c>='a' && c<='z') c=static_cast<char>(c-'a'+'A');
        const auto rows=glyph(c);
        for (int row=0;row<7;++row) for (int col=0;col<5;++col) {
            if ((rows[static_cast<std::size_t>(row)] & (1U << (4-col))) != 0U)
                rect(r,static_cast<float>(cursor+col*scale),static_cast<float>(y+row*scale),
                     static_cast<float>(scale),static_cast<float>(scale),red,green,blue);
        }
        if (tilde) {
            rect(r,static_cast<float>(cursor+scale),static_cast<float>(y-2*scale),static_cast<float>(scale),static_cast<float>(scale),red,green,blue);
            rect(r,static_cast<float>(cursor+2*scale),static_cast<float>(y-3*scale),static_cast<float>(scale),static_cast<float>(scale),red,green,blue);
            rect(r,static_cast<float>(cursor+3*scale),static_cast<float>(y-3*scale),static_cast<float>(scale),static_cast<float>(scale),red,green,blue);
            rect(r,static_cast<float>(cursor+4*scale),static_cast<float>(y-2*scale),static_cast<float>(scale),static_cast<float>(scale),red,green,blue);
        }
        cursor+=(c==' '?4:6)*scale;
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
void draw_texture_rotated(SDL_Renderer* r, const Texture& t, float x, float y, float w, float h, double angle) {
    if (!t.texture) return;
    const SDL_FRect dst{x, y, w, h};
    SDL_RenderTextureRotated(r, t.texture, nullptr, &dst, angle, nullptr, SDL_FLIP_NONE);
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
void heart_sprite(SDL_Renderer* r, const Texture& heart, float x, float y, float size) {
    draw_texture(r,heart,x,y,size,size);
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
void coffee_bottle(SDL_Renderer* r, const Texture& bottle, float x, float base, float scale = 1.0F, double angle = 0.0) {
    const float h=132.0F*scale;
    const float w=bottle.h>0?h*static_cast<float>(bottle.w)/static_cast<float>(bottle.h):88.0F*scale;
    if (angle==0.0) draw_texture(r,bottle,x,base-h,w,h);
    else draw_texture_rotated(r,bottle,x,base-h,w,h,angle);
}
void stroke(SDL_Renderer* r, float x0, float y0, float x1, float y1, float width,
            Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255) {
    const float dx=x1-x0, dy=y1-y0;
    const float length=std::max(1.0F,std::sqrt(dx*dx+dy*dy));
    const float nx=-dy/length, ny=dx/length;
    color(r,red,green,blue,alpha);
    for (float offset=-width*.5F; offset<=width*.5F; offset+=1.0F)
        SDL_RenderLine(r,x0+nx*offset,y0+ny*offset,x1+nx*offset,y1+ny*offset);
}
void coffee_drink_pose(SDL_Renderer* r, const Texture& sprite,
                       float x, float y, float w, float h, float time) {
    const float bob=std::sin(time*3.0F)*2.0F;
    const float scale=sprite.w>0 && sprite.h>0
        ?std::min(w/static_cast<float>(sprite.w),h/static_cast<float>(sprite.h)):1.0F;
    const float draw_w=static_cast<float>(sprite.w)*scale;
    const float draw_h=static_cast<float>(sprite.h)*scale;
    const double tilt=std::sin(time*1.7F)*1.2;
    draw_texture_rotated(r,sprite,x+(w-draw_w)*.5F,y+bob+(h-draw_h)*.5F,draw_w,draw_h,tilt);
}
void celebration_fireworks(SDL_Renderer* r, float time) {
    constexpr std::array<std::array<Uint8,3>,4> colors{{{{255,218,112}},{{255,130,96}},{{132,229,194}},{{166,190,255}}}};
    constexpr std::array<std::array<float,2>,4> centers{{{{170,190}},{{1090,205}},{{350,470}},{{955,450}}}};
    for (std::size_t burst=0; burst<centers.size(); ++burst) {
        const float phase=std::fmod(time*0.72F+static_cast<float>(burst)*0.23F,1.0F);
        const float radius=12.0F+phase*78.0F;
        const Uint8 alpha=static_cast<Uint8>((1.0F-phase)*245.0F);
        for (int ray=0; ray<12; ++ray) {
            const float angle=static_cast<float>(ray)*pi/6.0F;
            const float x=centers[burst][0]+std::cos(angle)*radius;
            const float y=centers[burst][1]+std::sin(angle)*radius;
            const auto& tint=colors[burst];
            rect(r,x,y,6,6,tint[0],tint[1],tint[2],alpha);
            rect(r,x+std::cos(angle)*8.0F,y+std::sin(angle)*8.0F,3,3,255,247,202,alpha);
        }
    }
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
    return base - 171.0F - (stage == 0 ? queen_spring_offset(phase) * 0.35F : 0.0F);
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

    SDL_AudioSpec impact_spec{};
    Uint8* impact_data=nullptr;
    Uint32 impact_length=0;
    SDL_AudioStream* impact_stream=nullptr;
    if (SDL_LoadWAV(asset("impacto-golpe.wav").c_str(),&impact_spec,&impact_data,&impact_length)) {
        impact_stream=SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,&impact_spec,nullptr,nullptr);
        if (!impact_stream || !SDL_ResumeAudioStreamDevice(impact_stream)) {
            if (impact_stream) SDL_DestroyAudioStream(impact_stream);
            impact_stream=nullptr;
            SDL_free(impact_data);
            impact_data=nullptr;
            impact_length=0;
        }
    } else {
        std::fprintf(stderr,"Efeito de impacto nao encontrado: %s\n",SDL_GetError());
    }
    const auto play_impact=[&]() {
        if (!impact_stream || !impact_data || impact_length==0) return;
        SDL_ClearAudioStream(impact_stream);
        SDL_PutAudioStreamData(impact_stream,impact_data,static_cast<int>(impact_length));
    };

    const std::array<Texture, stage_count> backgrounds{{
        load_texture(renderer, asset("mata.bmp"), false, true),
        load_texture(renderer, asset("cidade.bmp"), false, true),
        load_texture(renderer, asset("escritorio.bmp"), false, true)
    }};
    const Texture coffee_bottle_texture=load_texture(renderer,asset("cafe-garrafa.bmp"),true);
    const Texture life_heart_texture=load_texture(renderer,asset("coracao-vida.bmp"),true,true);
    const Texture victory_sprite=load_texture(renderer,asset("tamandua-bebendo.bmp"),true,true);
    std::array<Texture, 8> hero{};
    std::array<Texture, 4> digging{};
    std::array<Texture, 4> brute{};
    std::array<Texture, 4> gremlin{};
    std::array<Texture, 4> ants{};
    std::array<std::array<Texture, 4>, stage_count> boss_textures{};
    const std::array<std::array<const char*, 4>, stage_count> boss_texture_names{{
        {{"boss_queen_walk1.bmp", "boss_queen_walk2.bmp", "boss_queen_windup.bmp", "boss_queen_attack.bmp"}},
        {{"boss_guardian_walk1.bmp", "boss_guardian_walk2.bmp", "boss_guardian_windup.bmp", "boss_guardian_attack.bmp"}},
        {{"boss_digital_walk1.bmp", "boss_digital_walk2.bmp", "boss_digital_windup.bmp", "boss_digital_attack.bmp"}}
    }};
    const std::array<Texture, stage_count> corn_textures{{
        load_texture(renderer, asset("projectile_corn.bmp"), true),
        load_texture(renderer, asset("projectile_cob.bmp"), true),
        load_texture(renderer, asset("projectile_popcorn.bmp"), true)
    }};
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
    for (std::size_t stage=0; stage<boss_textures.size(); ++stage)
        for (std::size_t pose=0; pose<boss_textures[stage].size(); ++pose)
            boss_textures[stage][pose]=load_texture(renderer,asset(boss_texture_names[stage][pose]),true);

    int pad_count=0;
    SDL_JoystickID* pad_ids=SDL_GetGamepads(&pad_count);
    SDL_Gamepad* pad=pad_ids && pad_count>0 ? SDL_OpenGamepad(pad_ids[0]) : nullptr;
    SDL_free(pad_ids);

    const std::array<Ground,4> forest_grounds{{{0,1110},{1200,world_w},{world_w,world_w},{world_w,world_w}}};
    const std::array<Ground,4> city_grounds{{{0,830},{950,1090},{1620,1850},{1970,world_w}}};
    const std::array<Ground,4> office_grounds{{{0,900},{1020,1900},{2020,world_w},{world_w,world_w}}};
    const std::array<Hazard,stage_count> traversal_hazards{{{1450,1980},{1090,1620},{1135,1645}}};
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
        {1280,1220,1370,72,2,EnemyKind::Brute},{2040,1990,2125,126,1,EnemyKind::Ant},
        {2320,2250,2400,100,1,EnemyKind::Gremlin},{2650,2550,2790,76,2,EnemyKind::Brute},
        {3010,2920,3100,136,1,EnemyKind::Ant},{3480,3390,3570,105,1,EnemyKind::Gremlin},
        {3740,3650,3820,142,1,EnemyKind::Ant}}};
    const std::array<Enemy,11> city_enemies{{
        {330,275,400,108,1,EnemyKind::Ant},{625,545,730,94,1,EnemyKind::Gremlin},
        {760,735,820,126,1,EnemyKind::Ant},{1750,1665,1820,92,1,EnemyKind::Gremlin},
        {2020,1980,2110,72,2,EnemyKind::Brute},{2300,2240,2370,122,1,EnemyKind::Ant},
        {2350,2275,2430,101,1,EnemyKind::Gremlin},{2770,2690,2855,132,1,EnemyKind::Ant},
        {3200,3120,3280,75,2,EnemyKind::Brute},{3500,3430,3590,108,1,EnemyKind::Gremlin},
        {3860,3770,3950,138,1,EnemyKind::Ant}}};
    const std::array<Enemy,11> office_enemies{{
        {300,250,390,118,1,EnemyKind::DigitalBug},{600,530,735,128,1,EnemyKind::DigitalBug},
        {1050,1020,1070,112,1,EnemyKind::DigitalBug},{2050,2020,2130,106,1,EnemyKind::DigitalBug},
        {1740,1660,1840,94,1,EnemyKind::DigitalBug},{2110,2040,2195,122,1,EnemyKind::DigitalBug},
        {2430,2350,2525,118,1,EnemyKind::DigitalBug},{2780,2700,2870,132,1,EnemyKind::DigitalBug},
        {3190,3110,3290,100,1,EnemyKind::DigitalBug},{3480,3410,3590,121,1,EnemyKind::DigitalBug},
        {3830,3750,3950,140,1,EnemyKind::DigitalBug}}};
    std::array<Enemy,11> enemies=forest_enemies;
    const std::array<float,visible_beans_per_stage> forest_bean_x{{180,450,850,1035,1350,2200,2380,3010}};
    const std::array<float,visible_beans_per_stage> city_bean_x{{180,420,735,2010,1775,1690,2240,3170}};
    const std::array<float,visible_beans_per_stage> office_bean_x{{180,470,790,1050,1810,1880,2240,3200}};
    const std::array<float,hidden_beans_per_stage> forest_cache_x{{690,2250,2530,3500}};
    const std::array<float,hidden_beans_per_stage> city_cache_x{{690,1800,2500,3510}};
    const std::array<float,hidden_beans_per_stage> office_cache_x{{710,2070,2500,3550}};
    std::array<Cache,4> caches{{{690,8,false},{2250,9,false},{2530,10,false},{3500,11,false}}};
    std::array<Bean,beans_per_stage> beans{{
        {180,ground_y-34},{450,ground_y-37},{850,ground_y-34},{1035,ground_y-34},
        {1350,ground_y-34},{2200,ground_y-34},{2380,ground_y-34},{3010,ground_y-34},
        {690,ground_y-36,false,false,false,0},{2250,ground_y-36,false,false,false,0},
        {2450,ground_y-36,false,false,false,0},{3500,ground_y-36,false,false,false,0}}};

    Player player;
    Angel angel;
    Heart heart;
    Boss boss;
    std::vector<CornProjectile> corn_projectiles;
    bool game_over=false;
    std::vector<HitEffect> hit_effects;
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
        boss.x=world_w-430.0F;
        boss.min_x=world_w-1580.0F;
        boss.max_x=world_w-300.0F;
        boss.speed=65.0F+static_cast<float>(current_stage)*12.0F;
        boss.direction=-1.0F;
        boss.parts=3; boss.hit_lock=0.0F; boss.flash=0.0F;
        boss.attack_timer=3.0F; boss.attack_animation=0.0F; corn_projectiles.clear();
        hit_effects.clear();
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
        const Hazard& hazard=traversal_hazards[static_cast<std::size_t>(current_stage)];
        float best_score=1.0e9F;
        float safe_x=72.0F;
        for (const auto& ground:grounds) {
            const float min_x=ground.x0+8.0F;
            const float max_x=ground.x1-66.0F;
            if (max_x<min_x || ground.x1-ground.x0<80.0F) continue;
            const std::array<float,3> candidates{{
                std::clamp(angel.respawn_x,min_x,max_x),
                std::clamp(hazard.x0-76.0F,min_x,max_x),
                std::clamp(hazard.x1+10.0F,min_x,max_x)
            }};
            for (float candidate:candidates) {
                if (candidate+58.0F>hazard.x0 && candidate+2.0F<hazard.x1) continue;
                float score=std::abs(candidate-angel.respawn_x);
                for (const auto& foe:enemies) if (foe.alive &&
                    std::abs(candidate+30.0F-(foe.x+27.0F))<105.0F) score+=420.0F;
                if (boss.active && !boss.defeated &&
                    std::abs(candidate+30.0F-(boss.x+140.0F))<170.0F) score+=500.0F;
                if (score<best_score) { best_score=score; safe_x=candidate; }
            }
        }
        player.x=safe_x;
        player.y=ground_y-114.0F;
        player.vx=0.0F; player.vy=0.0F;
        player.grounded=true; player.crouched=false; player.rolling=false;
        player.coyote_timer=0.18F; player.respawn_invulnerable=2.8F;
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
        if (!paused && !title_screen) {
            for (auto& fx:hit_effects) fx.timer+=dt;
            hit_effects.erase(std::remove_if(hit_effects.begin(),hit_effects.end(),
                [](const HitEffect& fx){return fx.timer>0.86F;}),hit_effects.end());
        }
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
            coffee_bottle(renderer,coffee_bottle_texture,889,505,1.05F);
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
            if (celebration_timer>=2.8F) {
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
            const Hazard& hazard=traversal_hazards[static_cast<std::size_t>(current_stage)];
            const bool over_river=current_stage==1 && player.x+50.0F>hazard.x0 && player.x+10.0F<hazard.x1;
            bool boss_top_contact=false;
            if (player.vy>=0) {
                for (const auto& ground:grounds) if (!over_river && player.x+50>ground.x0 && player.x+10<ground.x1 &&
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
                    constexpr float boss_w=280.0F;
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
            if (current_stage!=1 && player.respawn_invulnerable<=0.0F && player.grounded &&
                player.y+h>=ground_y-2.0F && player.x+58.0F>hazard.x0 && player.x+2.0F<hazard.x1)
                start_death();

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
                if (boss.active && !boss.defeated) {
                    const float boss_center=boss.x+125.0F;
                    const float player_center=player.x+30.0F;
                    const float distance=player_center-boss_center;
                    if (std::abs(distance)>72.0F) boss.direction=distance>0.0F?1.0F:-1.0F;
                    else boss.direction=std::sin(elapsed*0.8F)>=0.0F?1.0F:-1.0F;
                    boss.x+=boss.direction*boss.speed*dt;
                    if (boss.x<boss.min_x) { boss.x=boss.min_x; boss.direction=1.0F; }
                    if (boss.x>boss.max_x) { boss.x=boss.max_x; boss.direction=-1.0F; }
                    boss.attack_timer-=dt;
                    boss.attack_animation=std::max(0.0F,boss.attack_animation-dt);
                    if (boss.attack_timer<=0.0F) {
                        const bool player_left=player_center<boss.x+125.0F;
                        const float muzzle_x=boss.x+(player_left?18.0F:232.0F);
                        const float predicted_x=player_center+std::clamp(player.vx*0.38F,-110.0F,110.0F);
                        const float vx=std::clamp((predicted_x-muzzle_x)*1.18F,-420.0F,420.0F);
                        corn_projectiles.push_back({muzzle_x,ground_y-58.0F,vx,-112.0F,current_stage,true});
                        boss.attack_timer=4.6F;
                        boss.attack_animation=0.48F;
                    }
                }
                for (auto& shot:corn_projectiles) {
                    if (!shot.alive) continue;
                    shot.x+=shot.vx*dt;
                    shot.y+=shot.vy*dt;
                    shot.vy+=390.0F*dt;
                    const float size=shot.kind==1?36.0F:28.0F;
                    if (shot.x<0.0F || shot.x>world_w || shot.y>ground_y-8.0F) shot.alive=false;
                    if (shot.alive && !angel.active && player.respawn_invulnerable<=0.0F &&
                        overlaps(player.x+14.0F,player.y+18.0F,34.0F,h-20.0F,shot.x,shot.y,size,size)) {
                        shot.alive=false;
                        start_death();
                    }
                }
                corn_projectiles.erase(std::remove_if(corn_projectiles.begin(),corn_projectiles.end(),
                    [](const CornProjectile& shot){return !shot.alive;}),corn_projectiles.end());
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
                            --foe.hp;
                            const bool defeated=foe.hp<=0;
                            if (defeated) foe.alive=false;
                            hit_effects.push_back({foe.x+ew*.5F,ey+eh*.48F,0.0F,defeated});
                            play_impact();
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
                    constexpr float boss_w=280.0F;
                    const float boss_top=boss_top_y(current_stage,ground_y,elapsed);
                    const float boss_h=ground_y-boss_top;
                    if (boss_top_contact && player.jump_attack_armed && boss.hit_lock<=0.0F) {
                        --boss.parts; boss.hit_lock=0.45F; boss.flash=0.24F;
                        hit_effects.push_back({boss.x+140.0F,boss_top+boss_h*.48F,0.0F,boss.parts<=0});
                        play_impact();
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
                    const float bottle_x=world_w-220.0F;
                    if (overlaps(player.x+8.0F,player.y,52.0F,h,bottle_x+4.0F,ground_y-190.0F,118.0F,190.0F)) {
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
        const Hazard& hazard=traversal_hazards[static_cast<std::size_t>(current_stage)];
        const float hazard_left=hazard.x0-camera, hazard_width=hazard.x1-hazard.x0;
        if (current_stage==0) {
            rect(renderer,hazard_left,ground_y-6.0F,hazard_width,7,67,126,54);
            for (float hx=hazard.x0;hx<hazard.x1;hx+=26.0F) {
                const float sx=hx-camera;
                const float sway=std::sin(elapsed*2.0F+hx*.018F)*2.0F;
                stroke(renderer,sx+10.0F,ground_y-2.0F,sx+5.0F,ground_y-27.0F+sway,4.0F,76,131,58);
                stroke(renderer,sx+10.0F,ground_y-9.0F,sx+20.0F,ground_y-30.0F-sway,4.0F,91,151,62);
                rect(renderer,sx+8.0F,ground_y-20.0F+sway,5.0F,7.0F,115,69,51);
                rect(renderer,sx+2.0F,ground_y-30.0F+sway,7.0F,5.0F,185,49,70);
            }
            for (float hx=hazard.x0+24.0F;hx<hazard.x1;hx+=112.0F) {
                const float sx=hx-camera+std::sin(elapsed*2.0F+hx*.01F)*2.0F;
                rect(renderer,sx-10.0F,ground_y-36.0F,10,9,173,43,67);
                rect(renderer,sx-4.0F,ground_y-41.0F,12,11,219,65,83);
                rect(renderer,sx+6.0F,ground_y-35.0F,10,9,173,43,67);
                rect(renderer,sx-1.0F,ground_y-33.0F,8,8,255,166,130);
                rect(renderer,sx+1.0F,ground_y-32.0F,4,5,255,218,164);
            }
        } else if (current_stage==1) {
            for (float hx=hazard.x0;hx<hazard.x1;hx+=12.0F) {
                const float fraction=(hx-hazard.x0)/hazard_width;
                const float edge=std::clamp(std::min(fraction*11.0F,(1.0F-fraction)*11.0F),0.0F,1.0F);
                const float sx=hx-camera;
                const float wave=std::sin(elapsed*2.7F+hx*.019F)*2.8F;
                const float water_y=ground_y-5.0F+(1.0F-edge)*16.0F+wave;
                const Uint8 blue=static_cast<Uint8>(123.0F+edge*52.0F);
                const Uint8 green=static_cast<Uint8>(145.0F-edge*40.0F);
                rect(renderer,sx,water_y,14.0F,static_cast<float>(screen_h)-water_y,19,green,blue);
                rect(renderer,sx,water_y,14.0F,3.0F,static_cast<Uint8>(87.0F+edge*42.0F),
                     static_cast<Uint8>(188.0F+edge*35.0F),229,235);
                if (static_cast<int>(hx/12.0F+elapsed*2.0F)%4==0)
                    rect(renderer,sx+5.0F,water_y-5.0F,5.0F,2.0F,197,241,237,190);
            }
            for (int bubble=0;bubble<5;++bubble) {
                const float phase=std::fmod(elapsed*26.0F+static_cast<float>(bubble)*31.0F,91.0F);
                const float bx=hazard_left+std::fmod(static_cast<float>(bubble)*113.0F+elapsed*17.0F,hazard_width-16.0F);
                rect(renderer,bx,ground_y+86.0F-phase,4.0F,4.0F,151,231,224,130);
            }
        } else {
            for (float hx=hazard.x0;hx<hazard.x1;hx+=14.0F) {
                const float x0=hx-camera;
                const float y0=ground_y-3.0F+std::sin(elapsed*5.0F+hx*.024F)*2.5F;
                const float x1=hx+14.0F-camera;
                const float y1=ground_y-3.0F+std::sin(elapsed*5.0F+(hx+14.0F)*.024F)*2.5F;
                stroke(renderer,x0,y0,x1,y1,5.0F,36,69,85);
                stroke(renderer,x0,y0,x1,y1,2.0F,255,202,80);
            }
            for (float hx=hazard.x0+24.0F;hx<hazard.x1;hx+=108.0F) {
                const float sx=hx-camera;
                rect(renderer,sx,ground_y-9.0F,8.0F,8.0F,94,125,137);
                rect(renderer,sx+2.0F,ground_y-7.0F,4.0F,4.0F,255,209,91);
            }
            for (int spark=0;spark<6;++spark) {
                const float phase=elapsed*8.0F+static_cast<float>(spark)*1.7F;
                if (std::sin(phase)>0.55F) {
                    const float sx=hazard_left+std::fmod(static_cast<float>(spark)*83.0F+elapsed*35.0F,hazard_width);
                    const float sy=ground_y-15.0F-std::abs(std::sin(phase))*8.0F;
                    stroke(renderer,sx-4.0F,sy,sx+4.0F,sy,2.0F,152,250,255);
                    stroke(renderer,sx,sy-4.0F,sx,sy+4.0F,2.0F,152,250,255);
                }
            }
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
        for (const auto& bean:beans) if (!bean.found && bean.revealed) {
            const float bob=std::sin(elapsed*3.4F+bean.x*.021F)*4.0F;
            bean_sprite(renderer,bean.x-camera,item_y(bean)+bob);
            if (std::sin(elapsed*5.0F+bean.x*.03F)>0.82F)
                rect(renderer,bean.x-camera+13.0F,item_y(bean)+bob-10.0F,4,4,255,230,175);
        }
        for (const auto& cache:caches) dig_mark(renderer,cache.x-camera,ground_y,cache.dug,current_stage);
        if (!heart.found) {
            const float pulse=std::sin(elapsed*4.0F)*3.0F;
            rect(renderer,heart.x-camera-5.0F,heart.y-4.0F,39.0F,37.0F,255,92,103,static_cast<Uint8>(45+std::abs(pulse)*8));
            heart_sprite(renderer,life_heart_texture,heart.x-camera-5.0F,heart.y-6.0F+pulse,40.0F);
        }
        if (boss.defeated && !bottle_collected) {
            const float bob=std::sin(elapsed*2.8F)*5.0F;
            coffee_bottle(renderer,coffee_bottle_texture,world_w-220.0F-camera,ground_y+bob,1.42F);
            if (std::sin(elapsed*6.0F)>0.65F) {
                rect(renderer,world_w-217.0F-camera,ground_y-190.0F+bob,6,6,255,235,169);
                rect(renderer,world_w-101.0F-camera,ground_y-156.0F+bob,5,5,255,235,169);
            }
        }

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

                for (const auto& fx:hit_effects) {
            const float sx=fx.x-camera;
            if (fx.timer<0.26F) {
                const Uint8 alpha=static_cast<Uint8>(255.0F*std::clamp(1.0F-fx.timer/0.26F,0.0F,1.0F));
                const float spread=8.0F+fx.timer*62.0F;
                for (int slash=0;slash<3;++slash) {
                    const float offset=(static_cast<float>(slash)-1.0F)*13.0F;
                    const float left=sx+offset-spread*.45F;
                    const float right=sx+offset+spread*.42F;
                    const float top=fx.y-20.0F-fx.timer*15.0F;
                    const float bottom=fx.y+18.0F-fx.timer*15.0F;
                    stroke(renderer,left,top,right,bottom,8.0F,92,22,38,alpha);
                    stroke(renderer,left,top,right,bottom,4.0F,244,55,72,alpha);
                    stroke(renderer,left+3.0F,top+2.0F,right-3.0F,bottom-3.0F,1.5F,255,190,158,alpha);
                }
            }
            if (fx.defeated && fx.timer>0.06F && fx.timer<0.82F) {
                for (int puff=0;puff<4;++puff) {
                    const float age=std::max(0.0F,fx.timer-static_cast<float>(puff)*0.055F);
                    if (age<=0.0F) continue;
                    const float rise=age*54.0F;
                    const float drift=std::sin(age*7.0F+static_cast<float>(puff))*12.0F;
                    const float cloud_x=sx+drift+(static_cast<float>(puff)-1.5F)*11.0F;
                    const float cloud_y=fx.y+10.0F-rise;
                    const Uint8 alpha=static_cast<Uint8>(170.0F*std::clamp(1.0F-age/0.78F,0.0F,1.0F));
                    rect(renderer,cloud_x+4.0F,cloud_y,12.0F,6.0F,79,71,77,alpha);
                    rect(renderer,cloud_x,cloud_y+5.0F,20.0F,9.0F,102,91,94,alpha);
                    rect(renderer,cloud_x+4.0F,cloud_y+12.0F,14.0F,5.0F,126,109,104,alpha);
                    rect(renderer,cloud_x+5.0F,cloud_y+3.0F,6.0F,3.0F,174,151,139,static_cast<Uint8>(alpha/2));
                }
            }
        }

        if (boss.active && !boss.defeated) {
            const std::size_t walk_pose=static_cast<std::size_t>(static_cast<int>(elapsed*6.0F)%2);
            const std::size_t pose=boss.flash>0.0F?3U:boss.attack_timer<0.72F?2U:boss.attack_animation>0.0F?3U:walk_pose;
            const Texture& boss_texture=boss_textures[static_cast<std::size_t>(current_stage)][pose];
            const float target_h=174.0F;
            const float target_w=boss_texture.h>0?target_h*static_cast<float>(boss_texture.w)/static_cast<float>(boss_texture.h):220.0F;
            const float spring_bob=current_stage==0?queen_spring_offset(elapsed)*0.35F:0.0F;
            const float sprite_x=boss.x-camera+140.0F-target_w*0.5F;
            draw_texture(renderer,boss_texture,sprite_x,ground_y-target_h-spring_bob,target_w,target_h,player.x<boss.x+125.0F);
            const float head_top=boss_top_y(current_stage,ground_y,elapsed);
            text(renderer,current_stage==0?"FORMIGA RAINHA":current_stage==1?"GUARDIAO DA PRACA":"BUG SUPREMO",
                 static_cast<int>(boss.x-camera+28.0F),static_cast<int>(head_top-22.0F),1,255,244,210);
            for (int part=0;part<3;++part)
                rect(renderer,boss.x-camera+63.0F+static_cast<float>(part)*21.0F,head_top-10.0F,16,7,
                     part<boss.parts?255:74,part<boss.parts?202:57,part<boss.parts?102:70);
        }

        for (const auto& shot:corn_projectiles) {
            const Texture& projectile=corn_textures[static_cast<std::size_t>(shot.kind)];
            const float projectile_h=shot.kind==1?38.0F:29.0F;
            const float projectile_w=projectile.h>0?projectile_h*static_cast<float>(projectile.w)/static_cast<float>(projectile.h):36.0F;
            const double tumble=std::sin(elapsed*7.0F+shot.x*.018F)*26.0;
            draw_texture_rotated(renderer,projectile,shot.x-camera-projectile_w*0.5F,
                                 shot.y-projectile_h*0.5F,projectile_w,projectile_h,tumble);
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
        text(renderer,"GRÃOS",999,39,3,248,210,123);
        text(renderer,std::to_string(player.beans)+"/"+std::to_string(total_beans),1124,39,3,255,248,221);
        text(renderer,"VIDA",999,76,2,212,230,198);
        for (int i=0;i<player.lives;++i) {
            const float hx=1073.0F+static_cast<float>(i)*30.0F;
            heart_sprite(renderer,life_heart_texture,hx,67.0F,27.0F);
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
            rect(renderer,0,0,screen_w,screen_h,5,16,24,celebrating?68:player.finished?52:178);
            if (celebrating) {
                celebration_fireworks(renderer,celebration_timer);
                panel(renderer,210,164,860,394);
                coffee_drink_pose(renderer,victory_sprite,292,215,280,290,celebration_timer);
                text(renderer,"BOM TRABALHO!",652,222,3,255,241,213);
                text(renderer,"CAFE ENCONTRADO!",652,278,3,255,224,151);
                text(renderer,"UM CAFE EXCELENTE ESPERA POR VOCE",652,333,2,238,245,226);
                coffee_bottle(renderer,coffee_bottle_texture,887,497,1.52F);
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
                celebration_fireworks(renderer,elapsed+1.8F);
                const float happy_bob=std::sin(elapsed*4.0F)*4.0F;
                coffee_drink_pose(renderer,victory_sprite,230,238+happy_bob,250,250,elapsed);
                text(renderer,"TAMANDUA VENCEU",493,207,3,255,244,210);
                text(renderer,"3 FASES CONCLUIDAS",493,257,2,186,222,170);
                text(renderer,"GRAOS: "+std::to_string(player.beans)+"/"+std::to_string(total_beans),493,300,3,255,218,126);
                text(renderer,"ENCONTROU UM CAFE EXCELENTE!",493,362,2,239,199,111);
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
    for (auto& row:boss_textures) for (auto& t:row) if (t.texture) SDL_DestroyTexture(t.texture);
    for (const auto& t:corn_textures) if (t.texture) SDL_DestroyTexture(t.texture);
    if (coffee_bottle_texture.texture) SDL_DestroyTexture(coffee_bottle_texture.texture);
    if (life_heart_texture.texture) SDL_DestroyTexture(life_heart_texture.texture);
    if (victory_sprite.texture) SDL_DestroyTexture(victory_sprite.texture);
    for (const auto& background:backgrounds) if (background.texture) SDL_DestroyTexture(background.texture);
    if (music_stream) SDL_DestroyAudioStream(music_stream);
    if (music_data) SDL_free(music_data);
    if (impact_stream) SDL_DestroyAudioStream(impact_stream);
    if (impact_data) SDL_free(impact_data);
    SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
#ifdef __SWITCH__
    romfsExit();
#endif
    return 0;
}
