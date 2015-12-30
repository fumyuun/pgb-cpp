#include "sdl_videodec.h"
#define FPS 60

void putpixel(SDL_Renderer *renderer, int x, int y, Uint8 pixel);

sdl_videodec_t::sdl_videodec_t()
: panicked(false), asleep(false), debug(false)
{
    if(SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cout << "SDL Init error " << SDL_GetError() << std::endl;
        panic();
        return;
    }

    window = SDL_CreateWindow("pgb", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, 160, 144, 0);
    if(window == NULL)
    {
        std::cout << "SDL createwindow error " << SDL_GetError() << std::endl;
        panic();
        return;
    }

    renderer = SDL_CreateRenderer(window, -1, 0);

#ifdef SHOW_TILEMAP
    tilemap_window = SDL_CreateWindow("tilemap", 0, 0, 128, 128, 0);
    tilemap_renderer = SDL_CreateRenderer(tilemap_window, -1, 0);
#endif

    PALETTE[0] = 0xFF;
    PALETTE[1] = 0x80;
    PALETTE[2] = 0x60;
    PALETTE[3] = 0x20;
    PALETTE[4] = 0x00;
    last_tick = SDL_GetTicks();
}

sdl_videodec_t::~sdl_videodec_t()
{
    if(window != NULL)
    {
        SDL_DestroyWindow(window);
    }
    SDL_Quit();
}

void sdl_videodec_t::init(membus_t *membus)
{
    this->membus = membus;
    vram = membus->get_pointer(0x8000);
    spt = membus->get_pointer(0xFE00);
    LCDC = membus->get_pointer(0xFF40);
    SCY = membus->get_pointer(0xFF42);
    SCX = membus->get_pointer(0xFF43);
    LY = membus->get_pointer(0xFF44);
    BGP = membus->get_pointer(0xFF47);
    OBP0 = membus->get_pointer(0xFF48);
    OBP1 = membus->get_pointer(0xFF49);
    WY = membus->get_pointer(0xFF4A);
    WX = membus->get_pointer(0xFF4B);
}

void tile_t::decode(uint8_t tile_n, const uint8_t LCDC, uint8_t *vram)
{
    uint16_t offset = (LCDC & 0x10) ? 0x00 : 0x800;
    for(int line_n = 0; line_n < 8; ++line_n)
    {
        uint8_t line1 = vram[offset + 16*tile_n + 2*line_n];
        uint8_t line2 = vram[offset + 16*tile_n + 2*line_n + 1];

        int i, j;
        for(i = 7, j = 0x80; i >= 0; --i)
        {
            data[line_n][7-i] = ((line1 & j) >> i) + ((line2 & j) >> i);
            j >>= 1;
        }
    }
}

void sprite_t::decode(const uint8_t sprite_n, uint8_t *spt)
{
    y_pos = spt[4*sprite_n];
    x_pos = spt[4*sprite_n + 1];
    tile_n[0] = spt[4*sprite_n + 2] & 0xFE;
    tile_n[1] = spt[4*sprite_n + 2] | 0x01;
    bg_prio = spt[4*sprite_n + 3] & 0x80;
    yflip = spt[4*sprite_n + 3] & 0x40;
    xflip = spt[4*sprite_n + 3] & 0x20;
}

void sdl_videodec_t::decode()
{
    for(int t = 0; t < 256; ++t)
    {
        tileset[t].decode(t, *LCDC, vram);
    }
    for(int y = 0; y < 32; ++y)
    {
        for(int x = 0; x < 32; ++x)
        {
            tiledata[y][x] =  vram[0x1800 + 32*y + x];
        }
    }
    for(int n = 0; n < 40; ++n)
    {
        spriteset[n].decode(n, spt);
    }
}

#define COL0(x) (x & 0x03)
#define COL1(x) ((x & 0x0C) >> 2)
#define COL2(x) ((x & 0x30) >> 4)
#define COL3(x) ((x & 0xC0) >> 6)

void sdl_videodec_t::print()
{
    bg_pal[0] = PALETTE[COL0(*BGP)];
    bg_pal[1] = PALETTE[COL1(*BGP)];
    bg_pal[2] = PALETTE[COL2(*BGP)];
    bg_pal[3] = PALETTE[COL3(*BGP)];

    sp_pal[0][1] = PALETTE[COL1(*OBP0)];
    sp_pal[0][2] = PALETTE[COL2(*OBP0)];
    sp_pal[0][3] = PALETTE[COL3(*OBP0)];

    sp_pal[1][1] = PALETTE[COL1(*OBP1)];
    sp_pal[1][2] = PALETTE[COL2(*OBP1)];
    sp_pal[1][3] = PALETTE[COL3(*OBP1)];

    // To correct the tile_n with indexes. Might be unneeded (overflows)
    uint8_t tile_offset = (*LCDC & 0x10) ? 0 : 128;
    // Print background
    if(*LCDC & 0x01)
    {
        for(int screen_y = 0; screen_y < 144; ++screen_y)
        {
            for(int screen_x = 0; screen_x < 160; ++screen_x)
            {
                int y = *SCY + screen_y % 256;
                int x = *SCX + screen_x % 256;
                uint8_t tile_n = tiledata[y/8][x/8] + tile_offset;
                uint8_t data = tileset[tile_n].data[y%8][x%8];
                putpixel(renderer, screen_x % 160, screen_y % 144, bg_pal[data]);
            }
        }
    }
    // Print window: still untested.
    if(*LCDC & 0x20)    // Window display enabled
    {
        std::cout << "printing window" << std::endl;
        for(int screen_y = 0; screen_y < 144; ++screen_y)
        {
            for(int screen_x = 0; screen_x < 160; ++screen_x)
            {
                int y = *WY + screen_y;
                int x = *WX + screen_x - 7;
                if(y > 143 || x > 166)
                    continue;
                uint8_t tile_n = tiledata[y/8][x/8] + tile_offset;
                uint8_t data = tileset[tile_n].data[y%8][x%8];
                putpixel(renderer, screen_x % 160, screen_y % 144, bg_pal[data]);
            }
        }
    }
    for(int i = 0; i < 40; ++i)
    {
        for(int j = 0; j < 1; ++j)
        {
            uint8_t tile_n = spriteset[i].tile_n[j];
            if(spriteset[i].x_pos != 0 && spriteset[i].y_pos != 0)
            {
                std::cout << "Not hidden sprite #" << i << ", " << (int)spriteset[i].tile_n[0] << ": ";
                std::cout << (int)spriteset[i].x_pos << "," << (int)spriteset[i].y_pos;
                std::cout << std::endl;
                for(int y = 0; y < 16; ++y)
                {
                    for(int x = 0; x < 8; ++x)
                    {
                        uint8_t data = tileset[tile_n].data[y][x];
                        putpixel(renderer, spriteset[i].x_pos - 8,
                                 spriteset[i].y_pos - 16, sp_pal[j][data]);
                    }
                }
            }
        }
    }
    SDL_RenderPresent(renderer);
}

void sdl_videodec_t::run()
{
    Uint32 start = SDL_GetTicks();
    debug = false;

    while(SDL_PollEvent(&event))
    {
        switch(event.type)
        {
            case SDL_WINDOWEVENT:
                if(event.window.event == SDL_WINDOWEVENT_CLOSE)
                    exit(0);
                break;
            case SDL_QUIT:      exit(0);        break;
            case SDL_KEYDOWN:
                switch(event.key.keysym.sym)
                {
                    case SDLK_UP:           membus->set_keydown(KEY_UP);     break;
                    case SDLK_LEFT:         membus->set_keydown(KEY_LEFT);   break;
                    case SDLK_RIGHT:        membus->set_keydown(KEY_RIGHT);  break;
                    case SDLK_DOWN:         membus->set_keydown(KEY_DOWN);   break;
                    case SDLK_z:            membus->set_keydown(KEY_A);      break;
                    case SDLK_x:            membus->set_keydown(KEY_B);      break;
                    case SDLK_RETURN:       membus->set_keydown(KEY_START);  break;
                    case SDLK_BACKSPACE:    membus->set_keydown(KEY_SELECT); break;
                }
                break;
            case SDL_KEYUP:
                switch(event.key.keysym.sym)
                {
                    case SDLK_v:
                    #ifdef SHOW_TILEMAP
                        show_tilemap();
                    #endif
                        debug = true;
                        break;
                    case SDLK_UP:           membus->set_keyup(KEY_UP);     break;
                    case SDLK_LEFT:         membus->set_keyup(KEY_LEFT);   break;
                    case SDLK_RIGHT:        membus->set_keyup(KEY_RIGHT);  break;
                    case SDLK_DOWN:         membus->set_keyup(KEY_DOWN);   break;
                    case SDLK_z:            membus->set_keyup(KEY_A);      break;
                    case SDLK_x:            membus->set_keyup(KEY_B);      break;
                    case SDLK_RETURN:       membus->set_keyup(KEY_START);  break;
                    case SDLK_BACKSPACE:    membus->set_keyup(KEY_SELECT); break;
                }
                break;
        }
    }

    if(*LCDC & 0x80)
    {
        asleep = false;
        decode();
        print();
    }
    else
    {
        if(!asleep)
        {
            asleep = true;
            SDL_SetRenderDrawColor(renderer, (PALETTE[4] & 0xFF), (PALETTE[4] & 0xFF), (PALETTE[4] & 0xFF), SDL_ALPHA_OPAQUE);
            SDL_RenderFillRect(renderer, NULL);
            SDL_RenderPresent(renderer);
        }
    }
    Uint32 delta = SDL_GetTicks() - start;
    Uint32 sleep = (1000/FPS) - delta;
    if(sleep < 1000/FPS){
        SDL_Delay(sleep);
    }
}

void sdl_videodec_t::panic()
{
    std::cout << "Videodecoder panicked\n";
    panicked = true;
}

bool sdl_videodec_t::is_panicked()
{
    return panicked;
}

bool sdl_videodec_t::requesting_debug()
{
    return debug;
}

#ifdef SHOW_TILEMAP
void sdl_videodec_t::show_tilemap()
{
    for(int ty = 0; ty < 16; ++ty)
    {
        for(int tx = 0; tx < 16; ++tx)
        {
            uint8_t tile_n = 16 * ty + tx;
            for(int y = 0; y < 8; ++y)
            {
                for(int x = 0; x < 8; ++x)
                {
                    uint8_t data = tileset[tile_n].data[y][x];
                    putpixel(tilemap_renderer, 8 * tx + x, 8 * ty + y, bg_pal[data]);
                }
            }
        }
    }
    SDL_RenderPresent(tilemap_renderer);
}
#endif /* SHOW_TILEMAP */

/*
 * Set the pixel at (x, y) to the given value
 * NOTE: The surface must be locked before calling this!
 */
inline void putpixel(SDL_Renderer *renderer, int x, int y, Uint8 pixel)
{
    //*((Uint8 *)surface->pixels + y * surface->pitch + x) = pixel;
    SDL_SetRenderDrawColor(renderer, (pixel & 0xFF), (pixel & 0xFF), (pixel & 0xFF), SDL_ALPHA_OPAQUE);
    SDL_RenderDrawPoint(renderer, x, y);
}

