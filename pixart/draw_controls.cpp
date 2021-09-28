#include "draw.h"

using namespace draw;

static void borderxx(color c1, color c2) {
	auto push_fore = fore;
	fore = c1;
	line(caret.x, caret.y + height);
	line(caret.x + width, caret.y);
	fore = c2;
	line(caret.x, caret.y - height);
	line(caret.x - width, caret.y);
	fore = push_fore;
}

static void border_up() {
	borderxx(colors::border, colors::border.lighten());
}

static void border_down() {
	borderxx(colors::border.lighten(), colors::border);
}

static bool buttonrf(const void* focus, unsigned focus_bits) {
	auto focused = addfocused(focus, focus_bits);
	if(focused) {
		auto push_fore = fore;
		fore = colors::form;
		rectf();
		fore = push_fore;
	}
	else
		border_up();
	return focused;
}

bool draw::buttonf(const char* title, const void* focus, unsigned focus_bits) {
	const int padding = 2;
	height = texth() + padding*2;
	auto focused = buttonrf(focus, focus_bits);
	if(title) {
		auto push_caret = caret;
		caret.x += padding;
		caret.y += padding - 1;
		text(title);
		caret = push_caret;
	}
	return focused;
}