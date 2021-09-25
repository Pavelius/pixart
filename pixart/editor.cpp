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
			fore = color(x1 * 4, y1 * 4, 0);
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
				if((hot.key == MouseLeft || hot.key == MouseMove) && hot.pressed && fore != main_color)
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

typedef void(*fnwidget)(const char* format);

static void button(const char* format) {
	const int padding = 2;
	auto push_caret = caret;
	auto push_fore = fore;
	height = texth() + padding * 2;
	width = textw(format) + 3 * 2;
	fore = colors::button;
	rectf();
	fore = colors::border;
	rectb();
	fore = push_fore;
	caret.x += 3;
	caret.y += padding;
	if(format)
		text(format);
	caret = push_caret;
}

static void horizontal(fnwidget proc, const char* format, void* source, int bit) {
	auto push_width = width;
	auto push_height = height;
	proc(format);
	caret.x += width;
	caret.x += 2;
	height = push_height;
	width = push_width;
}

static void test_text() {
	auto push_fore = fore;
	auto push_width = width;
	auto push_height = height;
	width = getwidth() - 4;
	height = texth() * 3;
	fore = colors::button;
	rectf();
	fore = colors::text;
	fore_stroke = colors::black;
	//textf("Длинная строка, которая будет отображена на экране монитора и будет [использовать] форматирование.");
	textln("Руфус: Что здесь нарисовано? Кто-то знает?", -1, TextStroke);
	textln("Белла: Я знаю. Это легендарный длинный меч +5", -1, TextStroke);
	fore = push_fore;
	height = push_height;
	width = push_width;
}

void load_buffer() {
	buffer.resize(32, 32, 32, true);
	image(0, 0, gres("tavern15", "art/scene"), 0, 0);
	blit(buffer, 0, 0, buffer.width, buffer.height, 0, *canvas, 0, 0);
}

static void test_widgets() {
	auto push_fore = fore;
	fore = colors::text;
	horizontal(button, "Атаковать", 0, 0);
	horizontal(button, "Подкупить", 0, 0);
	horizontal(button, "Отпугнуть", 0, 0);
	horizontal(button, "Усыпить", 0, 0);
	push_fore = fore;
}

static void test_fonts() {
	auto push_fore = fore;
	auto push_caret = caret;
	fore = colors::text;
	auto count = font->count;
	for(auto i = 0; i < count; i++) {
		auto x = push_caret.x + (i % 16) * 16 + font->width;
		auto y = push_caret.y + (i / 16) * 16 + font->height;
		image(x, y, font, i, 0);
	}
	push_fore = fore;
}

void zoom_editor() {
	clearwindow();
	//test_fonts();
	//return;
	auto push_caret = caret;
	caret = {64, 0}; zoom_view();
	caret = {0, 64}; color_picker();
	caret = {0, 128}; palt = (color*)canvas->ptr(0, 0); pallette_view();
	caret = {2, 260}; test_text();
	caret = {0, 0}; normal_view();
	caret = push_caret;
}