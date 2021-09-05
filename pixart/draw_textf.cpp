#include "crt.h"
#include "draw.h"

using namespace draw;

int draw::tab_pixels = 0;

static bool match(const char** string, const char* name) {
	int n = zlen(name);
	if(memcmp(*string, name, n) != 0)
		return false;
	(*string) += n;
	return true;
}

static const char* glink(const char* p, char* result, unsigned result_maximum) {
	result[0] = 0;
	if(*p == '\"') {
		auto sym = *p++;
		p = psstr(p, result, sym);
	} else if(*p == '(') {
		auto ps = result;
		auto pe = ps + result_maximum;
		while(*p && *p != ')') {
			if(ps < pe)
				*ps++ = *p;
			p++;
		}
		*ps++ = 0;
		if(*p == ')')
			p++;
	}
	return p;
}

static int gettabwidth() {
	return draw::tab_pixels ? draw::tab_pixels : draw::textw(' ') * 4;
}

static const char* textspc(const char* p, int x0, short& x) {
	int tb;
	while(true) {
		switch(p[0]) {
		case ' ':
			p++;
			x += draw::textw(' ');
			continue;
		case '\t':
			p++;
			tb = gettabwidth();
			x = x0 + ((x - x0 + tb) / tb) * tb;
			continue;
		}
		break;
	}
	return p;
}

static const char* word(const char* text) {
	while(((unsigned char)*text) > 0x20 && *text != '*' && *text != '[' && *text != ']')
		text++;
	return text;
}

static const char* textfln(const char* p, color c1, int* max_width) {
	char temp[4096];
	int x0 = caret.x;
	int x2 = caret.x + width;
	unsigned flags = 0;
	draw::fore = c1;
	if(max_width)
		*max_width = 0;
	temp[0] = 0;
	while(true) {
		if(p[0] == '*' && p[1] == '*') {
			p += 2;
			if(flags & TextBold)
				flags &= ~TextBold;
			else
				flags |= TextBold;
			continue;
		} else if(p[0] == '*') {
			p++;
			if(flags & TextItalic)
				flags &= ~TextItalic;
			else {
				if((flags & TextItalic) == 0)
					caret.x += draw::texth() / 3;
				flags |= TextItalic;
			}
			continue;
		} else if(p[0] == '[' && p[1] == '[')
			p++;
		else if(p[0] == ']' && p[1] == ']')
			p++;
		else if(p[0] == '[') {
			p++;
			switch(*p) {
			case '~':
				p++;
				draw::fore = colors::text.mix(colors::window, 64);
				break;
			case '+':
				p++;
				draw::fore = colors::green;
				break;
			case '-':
				p++;
				draw::fore = colors::red;
				break;
			case '!':
				p++;
				draw::fore = colors::yellow;
				break;
			case '#':
				p++;
				flags |= TextUscope;
				draw::fore = colors::special;
				break;
			default:
				draw::fore = colors::special;
				break;
			}
			p = glink(p, temp, sizeof(temp) / sizeof(temp[0]) - 1);
		} else if(p[0] == ']') {
			p++;
			draw::fore = c1;
			temp[0] = 0;
			flags &= ~TextUscope;
		}
		// Обработаем пробелы и табуляцию
		p = textspc(p, x0, caret.x);
		int w;
		auto pre_caret = caret;
		if(p[0] == ':' && p[1] >= 'a' && p[1] <= 'z') {
			p++;
			char temp[128];
			p = stringbuilder::read(p, temp, temp + sizeof(temp) - 1);
			if(*p == ':')
				p++;
			w = 0;
		} else {
			const char* p2 = word(p);
			w = draw::textw(p, p2 - p);
			if(caret.x + w > x2) {
				if(max_width)
					*max_width = imax(*max_width, caret.x - x0);
				caret.x = x0;
				caret.y += draw::texth();
			}
			text(p, p2 - p, flags);
			p = p2;
		}
		p = textspc(p, x0, caret.x);
		if(temp[0] || (flags & TextUscope) != 0) {
			int x3 = caret.x;
			if(x3 > x2)
				x3 = x2;
			int y2 = caret.y + draw::texth();
			if(flags & TextUscope) {
				auto push_caret = caret;
				caret.x = pre_caret.x;
				caret.y = y2;
				line(x3, y2);
			}
			if(ishilite({pre_caret.x, caret.y, x3, y2})) {
				if(flags & TextUscope) {
					hot.cursor = cursor::Hand;
					if(temp[0] && hot.key == MouseLeft && !hot.pressed)
						zcpy(draw::link, temp, sizeof(draw::link) - 1);
				} else
					zcpy(draw::link, temp, sizeof(draw::link) - 1);
			}
		}
		// Отметим перевод строки и окончание строки
		if(p[0] == 0 || p[0] == 10 || p[0] == 13) {
			caret.x = x0;
			caret.y += draw::texth();
			p = skipcr(p);
			break;
		}
	}
	if(max_width)
		*max_width = imax(*max_width, caret.x - x0);
	return p;
}

void draw::textf(const char* string, int* max_width, int min_height, int* cashe_height, const char** cashe_string) {
	auto push_fore = fore;
	auto push_font = font;
	color color_text = fore;
	const char* p = string;
	auto y0 = caret.y;
	if(cashe_height) {
		*cashe_string = p;
		*cashe_height = 0;
	}
	if(max_width)
		*max_width = 0;
	while(p[0]) {
		int mw2 = 0;
		if(cashe_height && (caret.y - y0) <= min_height) {
			*cashe_string = p;
			*cashe_height = caret.y - y0;
		}
		if(match(&p, "###")) { // Header 3
			font = metrics::h3;
			p = skipsp(p);
			p = textfln(p, colors::h3, &mw2);
		} else if(match(&p, "##")) { // Header 2
			font = metrics::h2;
			p = skipsp(p);
			p = textfln(p, colors::h2, &mw2);
		} else if(match(&p, "#")) { // Header 1
			font = metrics::h1;
			p = skipsp(p);
			p = textfln(p, colors::h1, &mw2);
		} else if(match(&p, "...")) {// Без форматирования
			font = metrics::font;
			p = skipcr(p);
			color c1 = colors::window.mix(colors::border, 256 - 32);
			caret.y += texth() / 2;
			auto push_height = height;
			height = texth();
			while(p[0]) {
				int c = textbc(p, width);
				if(!c)
					break;
				auto push_fore = fore;
				auto push_caret = caret;
				fore = c1; rectf();
				fore = push_fore;
				text(p, c);
				caret = push_caret;
				caret.y += texth();
				p += c;
				if(match(&p, "...")) {
					p = skipcr(p);
					caret.y += texth() / 2;
					break;
				}
			}
			height = push_height;
		} else if(match(&p, "* ")) {
			// Список
			int dx = texth() / 2;
			int rd = texth() / 6;
			auto push_caret = caret;
			caret.x += dx + 2;
			caret.y += dx;
			circlef(rd);
			circle(rd);
			caret = push_caret;
			caret.x += texth();
			auto push_width = width;
			width -= texth();
			p = textfln(p, color_text, &mw2);
			width = push_width;
		} else
			p = textfln(p, color_text, &mw2);
		font = metrics::font;
		fore = color_text;
		if(max_width) {
			if(*max_width < mw2)
				*max_width = mw2;
		}
	}
	fore = push_fore;
	font = push_font;
}

int draw::textf(rect& rc, const char* string) {
	auto push_clipping = clipping; clipping.clear();
	auto push_caret = caret; caret.clear();
	auto push_width = width;
	width = rc.width();
	draw::textf(string, &rc.x2, 0, 0, 0);
	rc.y2 = rc.y1 + caret.y;
	rc.x2 += rc.x1;
	caret = push_caret;
	width = push_width;
	clipping = push_clipping;
	return rc.height();
}