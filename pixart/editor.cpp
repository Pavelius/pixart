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

static void color_picker() {
	auto x2 = caret.x + 64;
	auto y2 = caret.y + 64;
	auto push_fore = fore;
	for(auto y1 = caret.y; y1 < y2; y1++) {
		for(auto x1 = caret.x; x1 < x2; x1++) {
			fore = color(x1*4, y1*4, 0);
			pixel(x1, y1);
			if(hot.mouse.x == x1 && hot.mouse.y == y1) {
				if((hot.key == MouseLeft || hot.key == MouseMove) && hot.pressed && fore != main_color)
					execute(cbsetint, *((int*)&fore), 0, &main_color);
			}
		}
	}
	fore = push_fore;
}

static void pallette_box() {
	rectf();
}

static void pallette_view() {
	if(!palt)
		return;
	auto push_width = width;
	auto push_height = height;
	auto caret_origin = caret;
	auto push_fore = fore;
	width = 4;
	height = width;
	for(auto y = 0; y < 16; y++) {
		for(auto x = 0; x < 16; x++) {
			fore = palt[y * 16 + x];
			caret.x = caret_origin.x + x * width;
			caret.y = caret_origin.y + y * height;
			pallette_box();
		}
	}
	width = push_width;
	height = push_height;
	fore = push_fore;
	caret.x = caret_origin.x;
	caret.y = caret_origin.y + 16 * width;
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

static void test_text() {
	auto push_fore = fore;
	auto push_width = width;
	fore = colors::text;
	width = getwidth() - 4;
	textf("Длинная строка, которая будет отображена на экране монитора и будет [использовать] форматирование.");
	fore = push_fore;
	width = push_width;
}

void load_buffer() {
	buffer.resize(32, 32, 32, true);
	image(0, 0, gres("tavern15", "art/scene"), 0, 0);
	blit(buffer, 0, 0, buffer.width, buffer.height, 0, *canvas, 0, 0);
}

void zoom_editor() {
	clearwindow();
	auto push_caret = caret;
	caret = {64, 0}; zoom_view();
	caret = {0, 64}; color_picker();
	caret = {0, 128}; palt = (color*)canvas->ptr(0, 0); pallette_view();
	caret = {2, 260}; test_text();
	caret = {0, 0}; normal_view();
	caret = push_caret;
}