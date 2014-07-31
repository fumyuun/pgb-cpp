#ifndef SDL_VIDEODEC_H
#define SDL_VIDEODEC_H

#include <sys/time.h>
#include "SDL/SDL.h"
#include "membus.h"
#include <string>
#include <iostream>

struct tile_t
{
	void decode(const uint8_t tile_n, const uint8_t LCDC, uint8_t *vram);
	void print();
	uint8_t data[8][8];
};

struct sprite_t
{
	void decode(const uint8_t sprite_n, uint8_t *spt);
	uint8_t y_pos;
	uint8_t x_pos;
	uint8_t tile_n[2];
	bool bg_prio, yflip, xflip;
};

class sdl_videodec_t
{
	private:
		SDL_Surface *screen;
		SDL_Event event;
		uint8_t tiledata[32][32];
		tile_t tileset[256];
		sprite_t spriteset[40];
		Uint32 last_tick;
		Uint32 PALETTE[5];
		Uint32 bg_pal[4];
		Uint32 sp_pal[2][4];
	
		uint8_t *vram;
		uint8_t *spt;
		uint8_t *LCDC;
		uint8_t *SCY;
		uint8_t *SCX;
		uint8_t *LY;
		uint8_t *BGP;
		uint8_t *OBP0;
		uint8_t *OBP1;
		uint8_t *WY;
		uint8_t *WX;
		bool panicked;
		bool asleep;
		bool debug;
	public:
		sdl_videodec_t();
		~sdl_videodec_t();
		void init(membus_t *mem);
		void run();
		void decode();
		void print();
		void panic();
		bool is_panicked();
		bool requesting_debug();
};

#endif

