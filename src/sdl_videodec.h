#ifndef SDL_VIDEODEC_H
#define SDL_VIDEODEC_H

#define SHOW_TILEMAP

#include <sys/time.h>
#include <string>
#include <iostream>
#include <SDL2/SDL.h>

#include "membus.h"

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
		membus_t *membus;
		SDL_Window *window;
		SDL_Renderer *renderer;
	#ifdef SHOW_TILEMAP
		SDL_Window *tilemap_window;
		SDL_Renderer *tilemap_renderer;
	#endif
		SDL_Event event;
		uint8_t tiledata[32][32];
		tile_t tileset[256];
		sprite_t spriteset[40];
		Uint32 last_tick;
		Uint8 PALETTE[5];
		Uint8 bg_pal[4];
		Uint8 sp_pal[2][4];

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
	#ifdef SHOW_TILEMAP
		void show_tilemap();
	#endif
	#ifdef SHOW_SPRITEMAP
		void show_spritemap();
	#endif
};

#endif

