#include "draw.h"

using namespace draw;

static surface	buffer;
static color	main_color;

static void select_rect() {
	line(caret.x, caret.y + (height - 1));
	line(caret.x + (width - 1), caret.y);
	line(caret.x, caret.y - (height - 1));
	line(caret.x - (width - 1), caret.y);
}

static void normal_view() {
	blit(*canvas, caret.x, caret.y, buffer.width, buffer.height, 0, buffer, 0, 0);
}

static void zoom_view() {
	auto push_caret = caret;
	auto push_width = width;
	auto push_height = height;
	width = height = 8;
	for(auto y = 0; y < buffer.height; y++) {
		for(auto x = 0; x < buffer.width; x++) {
			fore = *((color*)buffer.ptr(x, y));
			rectf();
			if(ishilite()) {
				auto push_fore = fore;
				if(fore.isdark())
					fore = colors::white;
				else
					fore = colors::black;
				select_rect();
				fore = push_fore;
				if((hot.key == MouseLeft || hot.key == MouseMove) && hot.pressed && fore!=main_color)
					execute(cbsetint, *((int*)&main_color), 0, buffer.ptr(x, y));
			}
			caret.x += width;
		}
		caret.x = push_caret.x;
		caret.y += height;
	}
	height = push_height;
	width = push_width;
	caret = push_caret;
}

void load_buffer() {
	buffer.resize(32, 32, 32, true);
	image(0, 0, gres("tavern15", "art/scene"), 0, 0);
	blit(buffer, 0, 0, buffer.width, buffer.height, 0, *canvas, 0, 0);
}

void zoom_editor() {
	clearwindow();
	auto push_caret = caret;
	caret = {100, 8}; zoom_view();
	caret = {0, 0}; normal_view();
	caret = push_caret;
}