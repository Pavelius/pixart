#include "crt.h"
#include "draw.h"

using namespace draw;

static sprite* res_items = (sprite*)loadb("art/objects/items.pma");

static void avatar(int i) {
	auto push_fore = fore;
	fore = colors::border;
	width = height = 32;
	rectb();
	image(caret.x + 16, caret.y + 16, res_items, i, 0);
	fore = push_fore;
}

static void avatars(int i) {
	for(auto y = 0; y < 8; y++) {
		auto push_caret = caret;
		for(auto x = 0; x < 8; x++) {
			avatar(i++);
			caret.x += width;
		}
		caret = push_caret;
		caret.y += height;
	}
}

void sprite_viewer() {
	static int current_frame;
	clearwindow();
	if(res_items && res_items->count>0) {
		if(current_frame < 0)
			current_frame = 0;
		if(current_frame > res_items->count)
			current_frame = res_items->count-1;
		caret = {4, 4}; avatars(current_frame);
		switch(hot.key) {
		case KeyUp: execute(cbsetint, current_frame - 8, 0, &current_frame); break;
		case KeyDown: execute(cbsetint, current_frame + 8, 0, &current_frame); break;
		}
	}
}