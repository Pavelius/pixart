#include "color.h"
#include "crt.h"
#include "draw.h"

#ifndef __GNUC__
#pragma optimize("t", on)
#endif

extern "C" void* malloc(unsigned long size);
extern "C" void* realloc(void *ptr, unsigned long size);
extern "C" void	free(void* pointer);

using namespace draw;

// Default theme colors
color				colors::active;
color				colors::button;
color				colors::form;
color				colors::window;
color				colors::text;
color				colors::border;
color				colors::h1;
color				colors::h2;
color				colors::h3;
color				colors::special;
color				colors::tips::text;
color				colors::tips::back;
// Color context and font context
fnevent				draw::domodal;
color				draw::fore;
color				draw::fore_stroke;
int					draw::width;
int					draw::height;
const sprite*		draw::font;
color*				draw::palt;
rect				draw::clipping;
char				draw::link[4096];
hoti				draw::hot;
// Locale draw variables
static draw::surface default_surface;
draw::surface*		draw::canvas = &default_surface;
point				draw::caret;
// Drag
static const void*	drag_object;
point				draw::dragmouse;
// Metrics
sprite*				metrics::font = (sprite*)loadb("art/fonts/font.pma");
sprite*				metrics::h1 = (sprite*)loadb("art/fonts/h1.pma");
sprite*				metrics::h2 = (sprite*)loadb("art/fonts/h2.pma");
sprite*				metrics::h3 = (sprite*)loadb("art/fonts/h3.pma");
// Modal
static bool			break_modal;
static long			break_result;
static fnevent		next_proc;

long distance(point p1, point p2) {
	auto dx = p1.x - p2.x;
	auto dy = p1.y - p2.y;
	return isqrt(dx * dx + dy * dy);
}

static void correct(int& x1, int& y1, int& x2, int& y2) {
	if(x1 > x2)
		iswap(x1, x2);
	if(y1 > y2)
		iswap(y1, y2);
}

static bool correct(int& x1, int& y1, int& x2, int& y2, const rect& clip, bool include_edge = true) {
	correct(x1, y1, x2, y2);
	if(x2 < clip.x1 || x1 >= clip.x2 || y2 < clip.y1 || y1 >= clip.y2)
		return false;
	if(x1 < clip.x1)
		x1 = clip.x1;
	if(y1 < clip.y1)
		y1 = clip.y1;
	if(include_edge) {
		if(x2 > clip.x2)
			x2 = clip.x2;
		if(y2 > clip.y2)
			y2 = clip.y2;
	} else {
		if(x2 >= clip.x2)
			x2 = clip.x2 - 1;
		if(y2 >= clip.y2)
			y2 = clip.y2 - 1;
	}
	return true;
}

static void set32r(color* p, unsigned count) {
	auto p2 = p + count;
	while(p < p2)
		*p++ = fore;
}

static void set32a(color* p, unsigned count) {
	auto p2 = p + count;
	if(fore.a == 255)
		set32r(p, count);
	else if(!fore.a)
		return;
	else if(fore.a == 128) {
		while(p < p2) {
			p->r = (p->r + fore.r) >> 1;
			p->g = (p->g + fore.g) >> 1;
			p->b = (p->b + fore.b) >> 1;
			p++;
		}
	} else {
		while(p < p2) {
			p->r = (p->r * (255 - fore.a) + fore.r * fore.a) >> 8;
			p->g = (p->g * (255 - fore.a) + fore.g * fore.a) >> 8;
			p->b = (p->b * (255 - fore.a) + fore.b * fore.a) >> 8;
			p++;
		}
	}
}

static void set32(unsigned char* d, int d_scan, int width, int height, void(*proc)(color*, unsigned)) {
	while(height-- > 0) {
		proc((color*)d, width);
		d += d_scan;
	}
}

static void set32h(color* p, int height, unsigned char alpha) {
	auto d_scan = canvas->scanline / sizeof(color);
	while(height-- > 0) {
		p->r = (p->r * (255 - alpha) + fore.r * alpha) >> 8;
		p->g = (p->g * (255 - alpha) + fore.g * alpha) >> 8;
		p->b = (p->b * (255 - alpha) + fore.b * alpha) >> 8;
		p += d_scan;
	}
}

static void raw832(unsigned char* d, int d_scan, unsigned char* s, int s_scan, int width, int height, const color* pallette) {
	const int cbd = 4;
	while(height-- > 0) {
		unsigned char* p1 = d;
		unsigned char* sb = s;
		unsigned char* se = s + width;
		while(sb < se) {
			*((color*)p1) = pallette[*sb++];
			p1 += cbd;
		}
		s += s_scan;
		d += d_scan;
	}
}

static void rawbit(unsigned char* d, int d_scan, unsigned char* s, int s_scan, int x1, int x2, int height) {
	const int cbd = 4;
	while(height-- > 0) {
		for(auto i = x1; i < x2; i++) {
			if(s[i >> 3] & (0x80 >> (i & 0x7)))
				((color*)d)[i] = fore;
		}
		s += s_scan;
		d += d_scan;
	}
}

static void raw832m(unsigned char* d, int d_scan, unsigned char* s, int s_scan, int width, int height, const color* pallette) {
	const int cbd = 4;
	while(height-- > 0) {
		unsigned char* p1 = d;
		unsigned char* sb = s;
		unsigned char* se = s + width;
		while(sb < se) {
			*((color*)p1) = pallette[*sb++];
			p1 -= cbd;
		}
		s += s_scan;
		d += d_scan;
	}
}

static void raw32(unsigned char* d, int d_scan, unsigned char* s, int s_scan, int width, int height) {
	const int cbs = 3;
	const int cbd = 4;
	if(width <= 0)
		return;
	while(height-- > 0) {
		unsigned char* sb = s;
		unsigned char* se = s + width * cbs;
		unsigned char* p1 = d;
		while(sb < se) {
			p1[0] = sb[0];
			p1[1] = sb[1];
			p1[2] = sb[2];
			sb += cbs;
			p1 += cbd;
		}
		s += s_scan;
		d += d_scan;
	}
}

static void raw32m(unsigned char* d, int d_scan, unsigned char* s, int s_scan, int width, int height) {
	const int cbs = 3;
	const int cbd = 4;
	if(width <= 0)
		return;
	while(height-- > 0) {
		unsigned char* sb = s;
		unsigned char* se = s + width * cbs;
		unsigned char* p1 = d;
		while(sb < se) {
			p1[0] = sb[0];
			p1[1] = sb[1];
			p1[2] = sb[2];
			sb += cbs;
			p1 -= cbd;
		}
		s += s_scan;
		d += d_scan;
	}
}

// (00) end of line
// (01 - 7F) draw count of pixels
// (80, XX, AA) draw count of XX bytes of alpha AA pixels
// (81 - 9F, AA) draw count of (B-0xC0) bytes of alpha AA pixels
// (A0, XX) skip count of XX pixels
// A1 - FF skip count of (b-0xB0) pixels
// each pixel has b,g,r value
static void rle32(unsigned char* p1, int d1, unsigned char* s, int h, const unsigned char* s1, const unsigned char* s2, unsigned char alpha) {
	const int cbs = 3;
	const int cbd = 32 / 8;
	unsigned char* d = p1;
	if(!alpha)
		return;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			p1 += d1;
			s1 += d1;
			s2 += d1;
			d = p1;
			if(--h == 0)
				break;
		} else if(c <= 0x9F) {
			unsigned char ap, cb;
			// count
			if(c <= 0x7F) {
				cb = c;
				ap = 0xFF;
			} else if(c == 0x80) {
				cb = *s++;
				ap = *s++;
			} else {
				cb = c - 0x80;
				ap = *s++;
			}
			// clip left invisible part
			if(d + cb * cbd <= s1 || d > s2) {
				d += cb * cbd;
				s += cb * cbs;
				continue;
			} else if(d < s1) {
				unsigned char sk = (s1 - d) / cbd;
				d += sk * cbd;
				s += sk * cbs;
				cb -= sk;
			}
			// visible part
			if(ap == 0xFF && alpha == 0xFF) {
				do {
					if(d >= s2)
						break;
					d[0] = s[0];
					d[1] = s[1];
					d[2] = s[2];
					s += cbs;
					d += cbd;
				} while(--cb);
			} else {
				ap = (ap * alpha) / 256;
				do {
					if(d >= s2)
						break;
					d[0] = (((int)d[0] * (255 - ap)) + ((s[0]) * (ap))) >> 8;
					d[1] = (((int)d[1] * (255 - ap)) + ((s[1]) * (ap))) >> 8;
					d[2] = (((int)d[2] * (255 - ap)) + ((s[2]) * (ap))) >> 8;
					s += cbs;
					d += cbd;
				} while(--cb);
			}
			// right clip part
			if(cb) {
				s += cb * cbs;
				d += cb * cbd;
			}
		} else {
			if(c == 0xA0)
				d += (*s++) * cbd;
			else
				d += (c - 0xA0) * cbd;
		}
	}
}

static void rle32m(unsigned char* p1, int d1, unsigned char* s, int h, const unsigned char* s1, const unsigned char* s2, unsigned char alpha) {
	const int cbs = 3;
	const int cbd = 32 / 8;
	unsigned char* d = p1;
	if(!alpha)
		return;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			p1 += d1;
			s1 += d1;
			s2 += d1;
			d = p1;
			if(--h == 0)
				break;
		} else if(c <= 0x9F) {
			unsigned char ap, cb;
			// count
			if(c <= 0x7F) {
				cb = c;
				ap = 0xFF;
			} else if(c == 0x80) {
				cb = *s++;
				ap = *s++;
			} else {
				cb = c - 0x80;
				ap = *s++;
			}
			// clip left invisible part
			if(d - (cb * cbd) >= s2 || d < s1) {
				s += cb * cbs;
				d -= cb * cbd;
				continue;
			} else if(d >= s2) {
				unsigned char sk = 1 + (d - s2) / cbd;
				d -= sk * cbd;
				s += sk * cbs;
				cb -= sk;
				if(!cb)
					continue;
			}
			// visible part
			if(ap == 0xFF && alpha == 0xFF) {
				// no alpha or modification
				do {
					if(d < s1)
						break;
					d[0] = s[0];
					d[1] = s[1];
					d[2] = s[2];
					s += cbs;
					d -= cbd;
				} while(--cb);
			} else {
				// alpha channel
				ap = (ap * alpha) / 256;
				do {
					if(d < s1)
						break;
					d[0] = (((int)d[0] * (255 - ap)) + ((s[0]) * (ap))) >> 8;
					d[1] = (((int)d[1] * (255 - ap)) + ((s[1]) * (ap))) >> 8;
					d[2] = (((int)d[2] * (255 - ap)) + ((s[2]) * (ap))) >> 8;
					d -= cbd;
					s += cbs;
				} while(--cb);
			}
			// right clip part
			if(cb) {
				d -= cb * cbd;
				s += cb * cbs;
			}
		} else {
			if(c == 0xA0)
				d -= (*s++) * cbd;
			else
				d -= (c - 0xA0) * cbd;
		}
	}
}

static void rle832(unsigned char* p1, int d1, unsigned char* s, int h, const unsigned char* s1, const unsigned char* s2, unsigned char alpha, const color* pallette) {
	const int cbd = 32 / 8;
	unsigned char* d = p1;
	if(!alpha)
		return;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			p1 += d1;
			s1 += d1;
			s2 += d1;
			if(--h == 0)
				break;
			d = p1;
		} else if(c <= 0x9F) {
			unsigned char ap = alpha, cb;
			bool need_correct_s = false;
			// count
			if(c <= 0x7F) {
				need_correct_s = true;
				cb = c;
			} else if(c == 0x80) {
				cb = *s++;
				ap >>= 1;
			} else {
				cb = c - 0x80;
				ap >>= 1;
			}
			// clip left invisible part
			if(d + cb * cbd <= s1 || d > s2) {
				d += cb * cbd;
				if(need_correct_s)
					s += cb;
				continue;
			} else if(d < s1) {
				unsigned char sk = (s1 - d) / cbd;
				d += sk * cbd;
				if(need_correct_s)
					s += sk;
				cb -= sk;
			}
			// visible part
			if(ap == alpha) {
				if(ap == 0xFF) {
					do {
						if(d >= s2)
							break;
						*((color*)d) = pallette[*s++];
						d += cbd;
					} while(--cb);
				} else {
					do {
						if(d >= s2)
							break;
						unsigned char* s1 = (unsigned char*)&pallette[*s++];
						d[0] = (((int)d[0] * (255 - ap)) + ((s1[0]) * (ap))) >> 8;
						d[1] = (((int)d[1] * (255 - ap)) + ((s1[1]) * (ap))) >> 8;
						d[2] = (((int)d[2] * (255 - ap)) + ((s1[2]) * (ap))) >> 8;
						d += cbd;
					} while(--cb);
				}
			} else if(ap == 0x7F) {
				do {
					if(d >= s2)
						break;
					d[0] >>= 1;
					d[1] >>= 1;
					d[2] >>= 1;
					d += cbd;
				} while(--cb);
			} else {
				ap = 255 - ap;
				do {
					if(d >= s2)
						break;
					d[0] = (((int)d[0] * ap)) >> 8;
					d[1] = (((int)d[1] * ap)) >> 8;
					d[2] = (((int)d[2] * ap)) >> 8;
					d += cbd;
				} while(--cb);
			}
			// right clip part
			if(cb) {
				if(need_correct_s)
					s += cb;
				d += cb * cbd;
			}
		} else {
			if(c == 0xA0)
				d += (*s++) * cbd;
			else
				d += (c - 0xA0) * cbd;
		}
	}
}

static void rle832m(unsigned char* p1, int d1, unsigned char* s, int h, const unsigned char* s1, const unsigned char* s2, unsigned char alpha, const color* pallette) {
	const int cbd = 32 / 8;
	unsigned char* d = p1;
	if(!alpha)
		return;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			p1 += d1;
			s1 += d1;
			s2 += d1;
			d = p1;
			if(--h == 0)
				break;
		} else if(c <= 0x9F) {
			unsigned char ap = alpha, cb;
			bool need_correct_s = false;
			// count
			if(c <= 0x7F) {
				need_correct_s = true;
				cb = c;
			} else if(c == 0x80) {
				cb = *s++;
				ap >>= 1;
			} else {
				cb = c - 0x80;
				ap >>= 1;
			}
			// clip left invisible part
			if(d - (cb * cbd) >= s2 || d < s1) {
				d -= cb * cbd;
				if(need_correct_s)
					s += cb;
				continue;
			} else if(d >= s2) {
				unsigned char sk = (d - s2) / cbd;
				d -= sk * cbd;
				if(need_correct_s)
					s += sk;
				cb -= sk;
			}
			// visible part
			if(ap == alpha) {
				if(ap == 0xFF) {
					do {
						if(d < s1)
							break;
						*((color*)d) = pallette[*s++];
						d -= cbd;
					} while(--cb);
				} else {
					do {
						if(d < s1)
							break;
						unsigned char* s1 = (unsigned char*)&pallette[*s++];
						d[0] = (((int)d[0] * (255 - ap)) + ((s1[0]) * (ap))) >> 8;
						d[1] = (((int)d[1] * (255 - ap)) + ((s1[1]) * (ap))) >> 8;
						d[2] = (((int)d[2] * (255 - ap)) + ((s1[2]) * (ap))) >> 8;
						d -= cbd;
					} while(--cb);
				}
			} else if(ap == 0x7F) {
				do {
					if(d < s1)
						break;
					d[0] >>= 1;
					d[1] >>= 1;
					d[2] >>= 1;
					d -= cbd;
				} while(--cb);
			} else {
				ap = 255 - ap;
				do {
					if(d < s1)
						break;
					d[0] = (((int)d[0] * ap)) >> 8;
					d[1] = (((int)d[1] * ap)) >> 8;
					d[2] = (((int)d[2] * ap)) >> 8;
					d -= cbd;
				} while(--cb);
			}
			// right clip part
			if(cb) {
				if(need_correct_s)
					s += cb;
				d -= cb * cbd;
			}
		} else {
			if(c == 0xA0)
				d -= (*s++) * cbd;
			else
				d -= (c - 0xA0) * cbd;
		}
	}
}

static void alc32(unsigned char* d, int d_scan, const unsigned char* s, int height, const unsigned char* clip_x1, const unsigned char* clip_x2, color c1, bool italic) {
	const int cbs = 3;
	const int cbd = 4;
	unsigned char* p = d;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			d += d_scan;
			clip_x1 += d_scan;
			clip_x2 += d_scan;
			if(italic && (height & 1) != 0)
				d -= cbd;
			p = d;
			if(--height == 0)
				break;
		} else if(c <= 0x7F) {
			// clip left invisible part
			if(p + (c * cbd) <= clip_x1 || p > clip_x2) {
				p += c * cbd;
				s += c * cbs;
				continue;
			} else if(p < clip_x1) {
				unsigned char sk = (clip_x1 - p) / cbd;
				p += sk * cbd;
				s += sk * cbs;
				c -= sk;
			}
			// visible part
			do {
				if(p >= clip_x2)
					break;
				p[0] = ((p[0] * (255 - s[0])) + (c1.b * (s[0]))) >> 8;
				p[1] = ((p[1] * (255 - s[1])) + (c1.g * (s[1]))) >> 8;
				p[2] = ((p[2] * (255 - s[2])) + (c1.r * (s[2]))) >> 8;
				p += cbd;
				s += cbs;
			} while(--c);
			// right clip part
			if(c) {
				p += c * cbd;
				s += c * cbs;
			}
		} else {
			if(c == 0x80)
				p += (*s++) * cbd;
			else
				p += (c - 0x80) * cbd;
		}
	}
}

static unsigned char* skip_v3(unsigned char* s, int h) {
	const int		cbs = 1;
	if(!s || !h)
		return s;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			if(--h == 0)
				return s;
		} else if(c <= 0x9F) {
			if(c <= 0x7F)
				s += c * cbs;
			else {
				if(c == 0x80)
					c = *s++;
				else
					c -= 0x80;
				s++;
				s += c * cbs;
			}
		} else if(c == 0xA0)
			s++;
	}
}

static unsigned char* skip_rle32(unsigned char* s, int h) {
	const int cbs = 3;
	if(!s || !h)
		return s;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			if(--h == 0)
				return s;
		} else if(c <= 0x9F) {
			if(c <= 0x7F)
				s += c * cbs;
			else {
				if(c == 0x80)
					c = *s++;
				else
					c -= 0x80;
				s++;
				s += c * cbs;
			}
		} else if(c == 0xA0)
			s++;
	}
}

static unsigned char* skip_alc(unsigned char* s, int h) {
	const int cbs = 3;
	if(!s || !h)
		return s;
	while(true) {
		unsigned char c = *s++;
		if(c == 0) {
			if(--h == 0)
				return s;
		} else if(c <= 0x7F)
			s += c * cbs;
		else if(c == 0x80)
			s++;
	}
}

static void scale_line_32(unsigned char* dst, unsigned char* src, int sw, int tw) {
	const int cbd = 4;
	int NumPixels = tw;
	int IntPart = (sw / tw) * cbd;
	int FractPart = sw % tw;
	int E = 0;
	while(NumPixels-- > 0) {
		*((unsigned*)dst) = *((unsigned*)src);
		dst += cbd;
		src += IntPart;
		E += FractPart;
		if(E >= tw) {
			E -= tw;
			src += cbd;
		}
	}
}

static bool corrects(const surface& dc, int& x, int& y, int& width, int& height) {
	if(x + width > dc.width)
		width = dc.width - x;
	if(y + height > dc.height)
		height = dc.height - y;
	if(width <= 0 || height <= 0)
		return false;
	return true;
}

static bool correctb(int& x1, int& y1, int& w, int& h, int& ox) {
	int x11 = x1;
	int x2 = x1 + w;
	int y2 = y1 + h;
	if(!correct(x1, y1, x2, y2, draw::clipping))
		return false;
	ox = x1 - x11;
	w = x2 - x1;
	h = y2 - y1;
	return true;
}

static void scale32(
	unsigned char* d, int d_scan, int d_width, int d_height,
	unsigned char* s, int s_scan, int s_width, int s_height) {
	if(!d_width || !d_height || !s_width || !s_height)
		return;
	const int cbd = 4;
	int NumPixels = d_height;
	int IntPart = (s_height / d_height) * s_scan;
	int FractPart = s_height % d_height;
	int E = 0;
	unsigned char* PrevSource = 0;
	while(NumPixels-- > 0) {
		if(s == PrevSource)
			memcpy(d, d - d_scan, d_width * cbd);
		else {
			scale_line_32(d, s, s_width, d_width);
			PrevSource = s;
		}
		d += d_scan;
		s += IntPart;
		E += FractPart;
		if(E >= d_height) {
			E -= d_height;
			s += s_scan;
		}
	}
}

static void cpy(unsigned char* d, int d_scan, unsigned char* s, int s_scan, int width, int height, int bytes_per_pixel) {
	if(height <= 0 || width <= 0)
		return;
	int width_bytes = width * bytes_per_pixel;
	do {
		memcpy(d, s, width_bytes);
		s += s_scan;
		d += d_scan;
	} while(--height);
}

static void cpy32t(unsigned char* d, int d_scan, unsigned char* s, int s_scan, int width, int height) {
	if(height <= 0 || width <= 0)
		return;
	do {
		color* d2 = (color*)d;
		color* sb = (color*)s;
		color* se = sb + width;
		while(sb < se) {
			if(!sb->a) {
				d2++;
				sb++;
			} else if(sb->a == 0xFF)
				*d2++ = *sb++;
			else {
				auto ap = sb->a;
				d2->r = (((int)d2->r * (255 - ap)) + ((sb->r) * (ap))) >> 8;
				d2->g = (((int)d2->g * (255 - ap)) + ((sb->g) * (ap))) >> 8;
				d2->b = (((int)d2->b * (255 - ap)) + ((sb->b) * (ap))) >> 8;
				d2++; sb++;
			}
		}
		s += s_scan;
		d += d_scan;
	} while(--height);
}

void draw::dragbegin(const void* p) {
	drag_object = p;
	dragmouse = hot.mouse;
}

bool draw::dragactive() {
	return drag_object != 0;
}

bool draw::dragactive(const void* p) {
	if(drag_object == p) {
		if(!hot.pressed || hot.key == KeyEscape) {
			drag_object = 0;
			hot.key = InputUpdate;
			hot.cursor = cursor::Arrow;
			return false;
		}
		return true;
	}
	return false;
}

int draw::getbpp() {
	return canvas ? canvas->bpp : 1;
}

int draw::getwidth() {
	return canvas ? canvas->width : 0;
}

int draw::getheight() {
	return canvas ? canvas->height : 0;
}

unsigned char* draw::ptr(int x, int y) {
	return canvas ? (canvas->bits + y * canvas->scanline + x * canvas->bpp / 8) : 0;
}

void draw::pixel(int x, int y) {
	if(x >= clipping.x1 && x < clipping.x2 && y >= clipping.y1 && y < clipping.y2) {
		if(!canvas)
			return;
		*((color*)((char*)canvas->bits + y * canvas->scanline + x * 4)) = fore;
	}
}

void draw::pixel(int x, int y, unsigned char a) {
	if(x < clipping.x1 || x >= clipping.x2 || y < clipping.y1 || y >= clipping.y2 || a == 0xFF)
		return;
	color* p = (color*)ptr(x, y);
	if(a == 0)
		*p = fore;
	else {
		p->b = (((unsigned)p->b * (a)) + (fore.b * (255 - a))) >> 8;
		p->g = (((unsigned)p->g * (a)) + (fore.g * (255 - a))) >> 8;
		p->r = (((unsigned)p->r * (a)) + (fore.r * (255 - a))) >> 8;
		p->a = 0;
	}
}

void draw::line(int x, int y) {
	if(!canvas)
		return;
	if(caret.y == y) {
		int x0 = caret.x, y0 = caret.y, x1 = x, y1 = y;
		if(correct(x0, y0, x1, y1, clipping, false))
			set32(canvas->ptr(x0, y0), canvas->scanline, x1 - x0 + 1, 1, set32r);
	} else if(caret.x == x) {
		int x0 = caret.x, y0 = caret.y, x1 = x, y1 = y;
		if(correct(x0, y0, x1, y1, clipping, false))
			set32(canvas->ptr(x0, y0), canvas->scanline, 1, y1 - y0 + 1, set32r);
	} else {
		int x1 = x, y1 = y;
		int x0 = caret.x, y0 = caret.y;
		int dx = iabs(x1 - x0), sx = x0 < x1 ? 1 : -1;
		int dy = -iabs(y1 - y0), sy = y0 < y1 ? 1 : -1;
		int err = dx + dy, e2;
		for(;;) {
			pixel(x0, y0);
			e2 = 2 * err;
			if(e2 >= dy) {
				if(x0 == x1) break;
				err += dy; x0 += sx;
			}
			if(e2 <= dx) {
				if(y0 == y1) break;
				err += dx; y0 += sy;
			}
		}
	}
	caret.x = x;
	caret.y = y;
}

void draw::linet(int x1, int y1) {
	int x0 = caret.x, y0 = caret.y;
	int dx = iabs(x1 - x0), sx = x0 < x1 ? 1 : -1;
	int dy = -iabs(y1 - y0), sy = y0 < y1 ? 1 : -1;
	int err = dx + dy, e2; // error value e_xy
	for(;;) {
		if((x0 + y0) & 1)
			pixel(x0, y0);
		if(x0 == x1 && y0 == y1) break;
		e2 = 2 * err;
		if(e2 >= dy) { err += dy; x0 += sx; } // e_xy+e_x > 0
		if(e2 <= dx) { err += dx; y0 += sy; } // e_xy+e_y < 0
	}
	caret.x = x1;
	caret.y = y1;
}

void draw::rectb() {
	line(caret.x, caret.y + height);
	line(caret.x + width, caret.y);
	line(caret.x, caret.y - height);
	line(caret.x - width, caret.y);
}

void draw::rectf() {
	if(!canvas)
		return;
	int x1 = caret.x, y1 = caret.y, x2 = caret.x + width, y2 = caret.y + height;
	if(!correct(x1, y1, x2, y2, clipping))
		return;
	if(x1 == x2)
		return;
	set32(ptr(x1, y1), canvas->scanline, x2 - x1, y2 - y1, (fore.a) ? set32a : set32r);
}

void draw::rectx() {
	linet(caret.x, caret.y + (height - 1));
	linet(caret.x + (width - 1), caret.y);
	linet(caret.x, caret.y - (height - 1));
	linet(caret.x - (width - 1), caret.y);
}

void draw::circlef(int r) {
	int xm = caret.x, ym = caret.y;
	if(xm - r >= clipping.x2 || xm + r < clipping.x1 || ym - r >= clipping.y2 || ym + r < clipping.y1)
		return;
	int x = -r, y = 0, err = 2 - 2 * r, y2 = -1000;
	auto push_caret = caret;
	auto push_width = width;
	auto push_height = height;
	height = 1;
	do {
		caret.y = ym + y;
		if(y2 != y) {
			y2 = y;
			caret.x = xm + x;
			width = x * 2;
			rectf();
			if(y != 0) {
				caret.y = ym - y;
				rectf();
			}
		}
		r = err;
		if(r <= y)
			err += ++y * 2 + 1;
		if(r > x || err > y)
			err += ++x * 2 + 1;
	} while(x < 0);
	height = push_height;
	width = push_width;
	caret = push_caret;
}

void draw::circle(int r) {
	int x = r, y = 0; // II. quadrant from bottom left to top right
	int x2, e2, err = 2 - 2 * r; // error of 1.step
	r = 1 - err;
	int xm = caret.x, ym = caret.y;
	for(;;) {
		int i = 255 * iabs(err + 2 * (x + y) - 2) / r; // get blend value of pixel
		pixel(xm + x, ym - y, i); // I. Quadrant
		pixel(xm + y, ym + x, i); // II. Quadrant
		pixel(xm - x, ym + y, i); // III. Quadrant
		pixel(xm - y, ym - x, i); // IV. Quadrant
		if(x == 0)
			break;
		e2 = err; x2 = x; // remember values
		if(err > y) {
			/* x step */
			int i = 255 * (err + 2 * x - 1) / r; // outward pixel
			if(i < 255) {
				pixel(xm + x, ym - y + 1, i);
				pixel(xm + y - 1, ym + x, i);
				pixel(xm - x, ym + y - 1, i);
				pixel(xm - y + 1, ym - x, i);
			}
			err -= --x * 2 - 1;
		}
		if(e2 <= x2--) {
			/* y step */
			int i = 255 * (1 - 2 * y - e2) / r;
			if(i < 255) {
				pixel(xm + x2, ym - y, i);
				pixel(xm + y, ym + x2, i);
				pixel(xm - x2, ym + y, i);
				pixel(xm - y, ym - x2, i);
			}
			err -= --y * 2 - 1;
		}
	}
}

void draw::setclipdf() {
	setclip({caret.x, caret.y, caret.x + width, caret.y + height});
}

void draw::setclip(rect rcn) {
	rect rc = draw::clipping;
	if(rc.x1 < rcn.x1)
		rc.x1 = rcn.x1;
	if(rc.y1 < rcn.y1)
		rc.y1 = rcn.y1;
	if(rc.x2 > rcn.x2)
		rc.x2 = rcn.x2;
	if(rc.y2 > rcn.y2)
		rc.y2 = rcn.y2;
	draw::clipping = rc;
}

static void intersect_rect(rect& r1, const rect& r2) {
	if(!r1.intersect(r2))
		return;
	if(hot.mouse.in(r2)) {
		if(r2.y1 > r1.y1)
			r1.y1 = r2.y1;
		if(r2.x1 > r1.x1)
			r1.x1 = r2.x1;
		if(r2.y2 < r1.y2)
			r1.y2 = r2.y2;
		if(r2.x2 < r1.x2)
			r1.x2 = r2.x2;
	} else {
		if(hot.mouse.y > r2.y2 && r2.y2 > r1.y1)
			r1.y1 = r2.y2;
		else if(hot.mouse.y < r2.y1 && r2.y1 < r1.y2)
			r1.y2 = r2.y1;
		else if(hot.mouse.x > r2.x2 && r2.x2 > r1.x1)
			r1.x1 = r2.x2;
		else if(hot.mouse.x < r2.x1 && r2.x1 < r1.x2)
			r1.x2 = r2.x1;
	}
}

bool draw::ishilite(const rect& rc) {
	if(hot.key == InputNoUpdate)
		return false;
	if(dragactive())
		return false;
	if(!hot.mouse.in(clipping))
		return false;
	if(hot.mouse.in(rc)) {
		hot.hilite = rc;
		return true;
	}
	return false;
}

int	draw::aligned(unsigned flags, int dx) {
	switch(flags & AlignMask) {
	case AlignRightBottom:
	case AlignRightCenter:
	case AlignRight: return width - dx;
	case AlignCenterBottom:
	case AlignCenterCenter:
	case AlignCenter: return (width - dx) / 2;
	default: return 0;
	}
}

int draw::alignedh(const char* string, unsigned state) {
	int ty;
	switch(state & AlignMask) {
	case AlignCenterCenter:
	case AlignRightCenter:
	case AlignLeftCenter:
		if(state & TextSingleLine)
			ty = texth();
		else
			ty = texth(string, width);
		return (height - ty) / 2;
	case AlignCenterBottom:
	case AlignRightBottom:
	case AlignLeftBottom:
		if(state & TextSingleLine)
			ty = texth();
		else
			ty = texth(string, width);
		return height - ty;
	default:
		return 0;
	}
}

int draw::textw(const char* string, int count) {
	if(!font)
		return 0;
	int x1 = 0;
	if(count == -1) {
		const char *s1 = string;
		while(*s1)
			x1 += textw(szget(&s1));
	} else {
		const char *s1 = string;
		const char *s2 = string + count;
		while(s1 < s2)
			x1 += textw(szget(&s1));
	}
	return x1;
}

const char* draw::skiptr(const char* p) {
	for(; *p && *p == 0x20; p++);
	if(*p == 13) {
		p++;
		if(*p == 10)
			p++;
	} else if(*p == 10) {
		p++;
		if(*p == 13)
			p++;
	}
	return p;
}

int draw::textw(rect& rc, const char* string) {
	int w1 = rc.width();
	rc.y2 = rc.y1;
	rc.x2 = rc.x1;
	while(string[0]) {
		int c = textbc(string, w1);
		if(!c)
			break;
		int m = textw(string, c);
		if(rc.width() < m)
			rc.x2 = rc.x1 + m;
		rc.y2 += texth();
		string = skiptr(string + c);
	}
	return rc.height();
}

int draw::texth(const char* string, int width) {
	int dy = texth();
	int y1 = 0;
	while(*string) {
		int c = textbc(string, width);
		if(!c)
			break;
		y1 += dy;
		string = skiptr(string + c);
	}
	return y1;
}

void draw::text(const char* string, int count, unsigned flags) {
	if(!font)
		return;
	auto dy = texth();
	if(caret.y >= clipping.y2 || caret.y + dy < clipping.y1)
		return;
	if(count == -1)
		count = zlen(string);
	const char *s1 = string;
	const char *s2 = string + count;
	while(s1 < s2) {
		int sm = szget(&s1);
		if(sm >= 0x21)
			glyph(caret.x, caret.y, sm, flags);
		caret.x += textw(sm);
	}
}

void draw::textln(const char* string, int count, unsigned flags) {
	auto push_caret = caret;
	text(string, count, flags);
	caret = push_caret;
	caret.y += texth();
}

void draw::text(const char* string, int count, unsigned flags, int maximum_width, bool* clipped) {
	if(clipped)
		*clipped = false;
	if(!font)
		return;
	auto dy = texth();
	if(caret.y >= clipping.y2 || caret.y + dy < clipping.y1)
		return;
	if(count == -1)
		count = zlen(string);
	const char *s1 = string;
	const char *s2 = string + count;
	auto x2 = caret.x + maximum_width - textw('.') * 3;
	while(s1 < s2) {
		int sm = szget(&s1);
		int x1 = caret.x + textw(sm);
		if(x1 >= x2) {
			text("...", -1, flags);
			if(clipped)
				*clipped = true;
			break;
		}
		if(sm >= 0x21)
			glyph(caret.x, caret.y, sm, flags);
		caret.x = x1;
	}
}

void draw::textc(const char* string, int count, unsigned flags, bool* clipped) {
	auto push_caret = caret;
	auto push_clip = clipping;
	setclip({caret.x, caret.y, caret.x + width, caret.y + texth()});
	auto w = textw(string, count);
	if(clipped)
		*clipped = w > width;
	caret.x += aligned(flags, w);
	text(string, count, flags);
	clipping = push_clip;
	caret = push_caret;
	caret.y += texth();
}

int draw::textbc(const char* string, int width) {
	if(!font)
		return 0;
	int p = -1;
	int w = 0;
	const char* s1 = string;
	while(true) {
		unsigned s = szget(&s1);
		if(s == 0x20 || s == 9)
			p = s1 - string;
		else if(s == 0) {
			p = s1 - string - 1;
			break;
		} else if(s == 10 || s == 13) {
			p = s1 - string;
			break;
		}
		w += textw(s);
		if(w > width)
			break;
	}
	if(p == -1)
		p = s1 - string;
	return p;
}

void draw::textb(const char* string, unsigned state, int* max_width) {
	if(!string || string[0] == 0)
		return;
	caret.y += alignedh(string, state);
	int dy = texth();
	if(max_width)
		*max_width = 0;
	if(state & TextSingleLine) {
		auto push_clip = clipping;
		setclipdf();
		caret.x += aligned(state, draw::textw(string));
		text(string, -1, state);
		clipping = push_clip;
		caret.y += dy;
	} else {
		int y2 = caret.y + height;
		while(caret.y < y2) {
			int c = textbc(string, width);
			if(!c)
				break;
			int w = textw(string, c);
			if(max_width && *max_width < w)
				*max_width = w;
			auto push_caret = caret;
			caret.x += aligned(state, w);
			text(string, c, state);
			caret = push_caret;
			caret.y += dy;
			string = skiptr(string + c);
		}
	}
}

void draw::image(int x, int y, const sprite* e, int id, int flags, unsigned char alpha) {
	int x2, y2;
	color* pal;
	if(!e)
		return;
	const sprite::frame& f = e->get(id);
	if(!f.offset)
		return;
	if(!canvas)
		return;
	if(flags & ImageMirrorH) {
		x2 = x;
		if((flags & ImageNoOffset) == 0)
			x2 += f.ox;
		x = x2 - f.sx;
	} else {
		if((flags & ImageNoOffset) == 0)
			x -= f.ox;
		x2 = x + f.sx;
	}
	if(flags & ImageMirrorV) {
		y2 = y;
		if((flags & ImageNoOffset) == 0)
			y2 += f.oy;
		y = y2 - f.sy;
	} else {
		if((flags & ImageNoOffset) == 0)
			y -= f.oy;
		y2 = y + f.sy;
	}
	unsigned char* s = (unsigned char*)e + f.offset;
	if(y2<clipping.y1 || y>clipping.y2 || x2<clipping.x1 || x>clipping.x2)
		return;
	if(y < clipping.y1) {
		if((flags & ImageMirrorV) == 0) {
			switch(f.encode) {
			case sprite::ALC: s = skip_alc(s, clipping.y1 - y); break;
			case sprite::RAW: s += (clipping.y1 - y) * f.sx * 3; break;
			case sprite::RAW8: s += (clipping.y1 - y) * f.sx; break;
			case sprite::RLE8: s = skip_v3(s, clipping.y1 - y); break;
			case sprite::RLE: s = skip_rle32(s, clipping.y1 - y); break;
			default: break;
			}
		}
		y = clipping.y1;
	}
	if(y2 > clipping.y2) {
		if(flags & ImageMirrorV) {
			switch(f.encode) {
			case sprite::ALC: s = skip_alc(s, y2 - clipping.y2); break;
			case sprite::RAW: s += (y2 - clipping.y2) * f.sx * 3; break;
			case sprite::RAW8: s += (y2 - clipping.y2) * f.sx; break;
			case sprite::RLE8: s = skip_v3(s, y2 - clipping.y2); break;
			case sprite::RLE: s = skip_rle32(s, y2 - clipping.y2); break;
			default: break;
			}
		}
		y2 = clipping.y2;
	}
	if(y >= y2)
		return;
	int x1;
	int wd = (flags & ImageMirrorV) ? -canvas->scanline : canvas->scanline;
	int sy = (flags & ImageMirrorV) ? y2 - 1 : y;
	switch(f.encode) {
	case sprite::RAW:
		if(x < clipping.x1) {
			if((flags & ImageMirrorH) == 0)
				s += (clipping.x1 - x) * 3;
			x = clipping.x1;
		}
		if(x2 > clipping.x2) {
			if(flags & ImageMirrorH)
				s += (x2 - clipping.x2) * 3;
			x2 = clipping.x2;
		}
		if(x >= x2)
			return;
		if(flags & ImageMirrorH)
			raw32m(ptr(x2 - 1, sy), wd, s, f.sx * 3,
				x2 - x,
				y2 - y);
		else
			raw32(ptr(x, sy), wd, s, f.sx * 3,
				x2 - x,
				y2 - y);
		break;
	case sprite::RAW8:
		if(x < clipping.x1) {
			s += clipping.x1 - x;
			x = clipping.x1;
		}
		if(x2 > clipping.x2)
			x2 = clipping.x2;
		if(x >= x2)
			return;
		if(!f.pallette || (flags & ImagePallette))
			pal = draw::palt;
		else
			pal = (color*)e->ptr(f.pallette);
		if(!pal)
			return;
		if(flags & ImageMirrorH)
			raw832m(ptr(x2 - 1, y), wd, s, f.sx,
				x2 - x,
				y2 - y,
				pal);
		else
			raw832(ptr(x, y), wd, s, f.sx, x2 - x, y2 - y, pal);
		break;
	case sprite::BIT:
		x1 = x;
		if(x < clipping.x1) {
			s += clipping.x1 - x;
			x1 = clipping.x1;
		}
		if(x2 > clipping.x2)
			x2 = clipping.x2;
		if(x1 >= x2)
			return;
		rawbit(ptr(x, y), wd, s, (f.sx + 7) / 8, x1 - x, x2 - x, y2 - y);
		break;
	case sprite::RLE8:
		if(!f.pallette || (flags & ImagePallette))
			pal = draw::palt;
		else
			pal = (color*)e->ptr(f.pallette);
		if(!pal)
			return;
		if(flags & ImageMirrorH)
			rle832m(ptr(x2 - 1, sy), wd, s, y2 - y,
				ptr(clipping.x1, sy),
				ptr(clipping.x2, sy),
				alpha, pal);
		else
			rle832(ptr(x, sy), wd, s, y2 - y,
				ptr(clipping.x1, sy),
				ptr(clipping.x2, sy),
				alpha, pal);
		break;
	case sprite::RLE:
		if(flags & ImageMirrorH)
			rle32m(ptr(x2 - 1, sy), wd, s, y2 - y,
				ptr(clipping.x1, sy),
				ptr(clipping.x2, sy),
				alpha);
		else
			rle32(ptr(x, sy), wd, s, y2 - y,
				ptr(clipping.x1, sy),
				ptr(clipping.x2, sy),
				alpha);
		break;
	case sprite::ALC:
		if(flags & TextBold)
			alc32(ptr(x, sy - 1), wd, s, y2 - y,
				ptr(clipping.x1, sy - 1),
				ptr(clipping.x2, sy - 1),
				fore, (flags & TextItalic) != 0);
		if(flags & TextBold)
			alc32(ptr(x, sy - 1), wd, s, y2 - y,
				ptr(clipping.x1, sy - 1),
				ptr(clipping.x2, sy - 1),
				fore, (flags & TextItalic) != 0);
		alc32(ptr(x, sy), wd, s, y2 - y,
			ptr(clipping.x1, sy), ptr(clipping.x2, sy),
			fore, (flags & TextItalic) != 0);
		break;
	default:
		break;
	}
}

void draw::image(int x, int y, const sprite* e, int id, int flags, unsigned char alpha, color* pal) {
	auto pal_push = draw::palt;
	draw::palt = pal;
	image(x, y, e, id, flags | ImagePallette, alpha);
	draw::palt = pal_push;
}

void draw::stroke(int x, int y, const sprite* e, int id, int flags, unsigned char thin, unsigned char* koeff) {
	auto& fr = e->get(id);
	rect rc = fr.getrect(x, y, flags);
	surface sf(rc.width() + 2, rc.height() + 2, 32);
	x--; y--;
	auto push_fore = fore;
	fore.a = 0; fore.r = fore.g = fore.b = 255;
	auto push_clip = clipping;
	auto push_canvas = canvas; canvas = &sf;
	setclip();
	auto push_width = width;
	auto push_height = height;
	auto push_caret = caret;
	caret.x = caret.y = 0;
	width = sf.width; height = sf.height;
	rectf();
	caret = push_caret;
	width = push_width;
	height = push_height;
	image(1, 1, e, id, ImageNoOffset);
	canvas = push_canvas;
	clipping = push_clip;
	for(int y1 = 0; y1 < sf.height; y1++) {
		bool inside = false;
		for(int x1 = 0; x1 < sf.width; x1++) {
			auto m = (color*)sf.ptr(x1, y1);
			if(!inside) {
				if(*m == fore)
					continue;
				auto px = rc.x1 + x1 - 1;
				auto py = rc.y1 + y1 - 1;
				for(auto n = 0; n < thin; n++, px--) {
					if(koeff)
						draw::pixel(px, py, koeff[n]);
					else
						draw::pixel(px, py);
				}
				inside = true;
			} else {
				if(*m != fore)
					continue;
				auto px = rc.x1 + x1 - 2;
				auto py = rc.y1 + y1 - 1;
				for(auto n = 0; n < thin; n++, px++) {
					if(koeff)
						draw::pixel(px, py, koeff[n]);
					else
						draw::pixel(px, py);
				}
				inside = false;
			}
		}
	}
	for(int x1 = 0; x1 < sf.width; x1++) {
		bool inside = false;
		for(int y1 = 0; y1 < sf.height; y1++) {
			auto m = (color*)sf.ptr(x1, y1);
			if(!inside) {
				if(*m == fore)
					continue;
				auto px = rc.x1 + x1 - 1;
				auto py = rc.y1 + y1 - 1;
				for(auto n = 0; n < thin; n++, py--) {
					if(koeff)
						draw::pixel(px, py, koeff[n]);
					else
						draw::pixel(px, py);
				}
				inside = true;
			} else {
				if(*m != fore)
					continue;
				auto px = rc.x1 + x1 - 1;
				auto py = rc.y1 + y1 - 2;
				for(auto n = 0; n < thin; n++, py++) {
					if(koeff)
						draw::pixel(px, py, koeff[n]);
					else
						draw::pixel(px, py);
				}
				inside = false;
			}
		}
	}
	fore = push_fore;
}

void draw::blit(surface& ds, int x1, int y1, int w, int h, unsigned flags, const surface& ss, int xs, int ys) {
	if(ss.bpp != ds.bpp)
		return;
	int ox;
	if(!correctb(x1, y1, w, h, ox))
		return;
	if(h > ss.height)
		h = ss.height;
	if(w > ss.width)
		w = ss.width;
	if(flags & ImageTransparent)
		cpy32t(
			ds.ptr(x1, y1), ds.scanline,
			const_cast<surface&>(ss).ptr(xs, ys) + ox * 4, ss.scanline,
			w, h);
	else
		cpy(
			ds.ptr(x1, y1), ds.scanline,
			const_cast<surface&>(ss).ptr(xs, ys) + ox * 4, ss.scanline,
			w, h, 4);
}

void draw::blit(surface& dest, int x, int y, int width, int height, unsigned flags, const surface& source, int x_source, int y_source, int width_source, int height_source) {
	if(width == width_source && height == height_source) {
		blit(dest, x, y, width, height, flags, source, x_source, y_source);
		return;
	}
	if(source.bpp != dest.bpp)
		return;
	if(!corrects(dest, x, y, width, height))
		return;
	if(!corrects(source, x_source, y_source, width_source, height_source))
		return;
	int ox;
	if(!correctb(x_source, y_source, width, height, ox))
		return;
	scale32(
		dest.ptr(x, y), dest.scanline, width, height,
		const_cast<surface&>(source).ptr(x_source, y_source) + ox * 4, source.scanline, width_source, height_source);
}

const pma* pma::getheader(const char* id) const {
	auto p = this;
	while(p->name[0]) {
		if(p->name[0] == id[0]
			&& p->name[1] == id[1]
			&& p->name[2] == id[2])
			return p;
		p = (pma*)((char*)p + p->size);
	}
	return 0;
}

const char* pma::getstring(int id) const {
	auto p = getheader("STR");
	if(!p || id > p->count)
		return "";
	return (char*)p + ((unsigned*)((char*)p + sizeof(*p)))[id];
}

int pma::find(const char* name) const {
	for(int i = 1; i <= count; i++) {
		if(strcmp(getstring(i), name) == 0)
			return i;
	}
	return 0;
}

int sprite::ganim(int index, int tick) {
	if(!cicles)
		return 0;
	cicle* c = gcicle(index);
	if(!c->count)
		return 0;
	return c->start + tick % c->count;
}

int sprite::glyph(unsigned sym) const {
	// First interval (latin plus number plus ASCII)
	unsigned* pi = (unsigned*)edata();
	unsigned* p2 = pi + esize() / sizeof(unsigned);
	unsigned n = 0;
	while(pi < p2) {
		if(sym >= pi[0] && sym <= pi[1])
			return sym - pi[0] + n;
		n += pi[1] - pi[0] + 1;
		pi += 2;
	}
	return 't' - 0x21; // Unknown symbol is question mark
}

rect sprite::frame::getrect(int x, int y, unsigned flags) const {
	int x2, y2;
	if(!offset)
		return{0, 0, 0, 0};
	if(flags & ImageMirrorH) {
		x2 = x;
		if((flags & ImageNoOffset) == 0)
			x2 += ox;
		x = x2 - sx;
	} else {
		if((flags & ImageNoOffset) == 0)
			x -= ox;
		x2 = x + sx;
	}
	if(flags & ImageMirrorV) {
		y2 = y;
		if((flags & ImageNoOffset) == 0)
			y2 += oy;
		y = y2 - sy;
	} else {
		if((flags & ImageNoOffset) == 0)
			y -= oy;
		y2 = y + sy;
	}
	return{x, y, x2, y2};
}

surface::plugin* surface::plugin::first;

surface::surface() : width(0), height(0), scanline(0), bpp(32), bits(0) {
}

surface::surface(int width, int height, int bpp) : surface() {
	resize(width, height, bpp, true);
}

surface::plugin::plugin(const char* name, const char* filter) : name(name), filter(filter), next(0) {
	seqlink(this);
}

unsigned char* surface::ptr(int x, int y) {
	return bits + y * scanline + x * (bpp / 8);
}

surface::~surface() {
	resize(0, 0, 0, true);
}

unsigned char* surface::allocator(unsigned char* bits, unsigned size) {
	if(!size) {
		free(bits);
		bits = 0;
	}
	if(!bits)
		bits = (unsigned char*)malloc(size);
	else
		bits = (unsigned char*)realloc(bits, size);
	return bits;
}

void surface::resize(int width, int height, int bpp, bool alloc_memory) {
	if(this->width == width && this->height == height && this->bpp == bpp)
		return;
	this->bpp = bpp;
	this->width = width;
	this->height = height;
	this->scanline = color::scanline(width, bpp);
	if(alloc_memory)
		bits = allocator(bits, (height + 1) * scanline);
}

void surface::flipv() {
	color::flipv(bits, scanline, height);
}

void surface::convert(int new_bpp, color* pallette) {
	if(bpp == new_bpp) {
		bpp = iabs(new_bpp);
		return;
	}
	auto old_scanline = scanline;
	scanline = color::scanline(width, new_bpp);
	if(iabs(new_bpp) <= bpp)
		color::convert(bits, width, height, new_bpp, 0, bits, bpp, pallette, old_scanline);
	else {
		unsigned char* new_bits = (unsigned char*)allocator(0, (height + 1) * scanline);
		color::convert(
			new_bits, width, height, new_bpp, pallette,
			bits, bpp, pallette, old_scanline);
		allocator(bits, 0);
		bits = new_bits;
	}
	bpp = iabs(new_bpp);
}

void draw::key2str(stringbuilder& sb, int key) {
	if(key & Ctrl)
		sb.add("Ctrl+");
	if(key & Alt)
		sb.add("Alt+");
	if(key & Shift)
		sb.add("Shift+");
	key = key & CommandMask;
	switch(key) {
	case KeyDown: sb.add("Down"); break;
	case KeyDelete: sb.add("Del"); break;
	case KeyEnd: sb.add("End"); break;
	case KeyEnter: sb.add("Enter"); break;
	case KeyHome: sb.add("Home"); break;
	case KeyLeft: sb.add("Left"); break;
	case KeyPageDown: sb.add("Page Down"); break;
	case KeyPageUp: sb.add("Page Up"); break;
	case KeyRight: sb.add("Right"); break;
	case KeyUp: sb.add("Up"); break;
	case F1: sb.add("F1"); break;
	case F2: sb.add("F2"); break;
	case F3: sb.add("F3"); break;
	case F4: sb.add("F4"); break;
	case F5: sb.add("F5"); break;
	case F6: sb.add("F6"); break;
	case F7: sb.add("F7"); break;
	case F8: sb.add("F8"); break;
	case F9: sb.add("F9"); break;
	case F10: sb.add("F10"); break;
	case F11: sb.add("F11"); break;
	case F12: sb.add("F12"); break;
	case KeySpace: sb.add("Space"); break;
	case KeyEscape: sb.add("Esc"); break;
	default:
		if(key >= 0x20) {
			char temp[2] = {(char)szupper(key), 0};
			sb.add(temp);
		}
		break;
	}
}

void draw::clearwindow() {
	auto push_caret = caret;
	auto push_width = width;
	auto push_height = height;
	auto push_fore = fore;
	caret = {0, 0};
	fore = colors::window;
	width = getwidth();
	height = getheight();
	rectf();
	fore = push_fore;
	caret = push_caret;
	width = push_width;
	height = push_height;
}

void draw::startscene(fnevent visualize) {
	if(!visualize)
		return;
	while(ismodal()) {
		visualize();
		domodal();
	}
}

void draw::breakmodal(int result) {
	break_modal = true;
	break_result = result;
}

void draw::buttoncancel() {
	breakmodal(0);
}

void draw::buttonok() {
	breakmodal(1);
}

void draw::breakparam() {
	breakmodal(hot.param);
}

int draw::getresult() {
	return break_result;
}

void draw::cbsetint() {
	auto p = (int*)hot.object;
	*p = hot.param;
}

void draw::cbsetptr() {
	auto p = (void**)hot.object;
	*p = (void*)hot.param;
}

void draw::setnext(fnevent v) {
	next_proc = v;
}

void draw::start() {
	while(next_proc) {
		auto p = next_proc;
		next_proc = 0; p();
	}
}

void draw::setneedupdate() {
	hot.key = InputNeedUpdate;
}

void draw::execute(fnevent proc, long value, long value2, const void* object) {
	domodal = proc;
	hot.key = 0;
	hot.param = value;
	hot.param2 = value2;
	hot.object = object;
}

static void standart_domodal() {
	//before_input->execute();
	draw::hot.key = draw::rawinput();
	if(!draw::hot.key)
		exit(0);
	//after_input->execute();
}

void focus_before_modal();
void focus_leave_modal();

bool draw::ismodal() {
	hot.cursor = cursor::Arrow;
	hot.hilite.clear();
	if(hot.key == InputNeedUpdate)
		hot.key = InputUpdate;
	else
		domodal = standart_domodal;
	focus_before_modal();
	if(!next_proc && !break_modal)
		return true;
	break_modal = false;
	focus_leave_modal();
	return false;
}

static void set_dark_theme() {
	colors::window = color(32, 32, 32);
	colors::active = color(172, 128, 0);
	colors::border = color(73, 73, 80);
	colors::button = color(0, 122, 204);
	colors::form = color(45, 45, 48);
	colors::text = color(255, 255, 255);
	colors::special = color(255, 244, 32);
	colors::border = color(63, 63, 70);
	colors::tips::text = color(255, 255, 255);
	colors::tips::back = color(100, 100, 120);
	colors::h1 = colors::text.mix(colors::button, 64);
	colors::h2 = colors::text.mix(colors::button, 96);
	colors::h3 = colors::text.mix(colors::button, 128);
}

void draw::initialize(const char* title) {
	set_dark_theme();
	draw::width = 120;
	draw::font = metrics::font;
	draw::fore = colors::text;
	draw::fore_stroke = colors::blue;
	draw::create(400, 300);
	draw::setcaption(title);
}