#include "color.h"
#include "point.h"
#include "stringbuilder.h"

#pragma once

typedef void(*fnevent)();

enum draw_event_s : unsigned {
	KeyBackspace = 8, KeyTab = 9, KeyEnter = 10, KeyEscape = 0x1B, KeySpace = 0x20, KeyDelete = 0x7F,
	// input events
	InputSymbol = 0x80, InputTimer, InputKeyUp, InputIdle, InputNeedUpdate, InputUpdate, InputNoUpdate,
	// Keyboard and mouse input (can be overrided by flags)
	MouseLeft, MouseLeftDBL, MouseRight,
	MouseMove, MouseWheelUp, MouseWheelDown,
	KeyLeft, KeyRight, KeyUp, KeyDown, KeyPageUp, KeyPageDown, KeyHome, KeyEnd,
	F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
	// support
	CommandMask = 0x000000FF,
	// misc events can be combination with previous
	Ctrl = 0x00000100,
	Alt = 0x00000200,
	Shift = 0x00000400,
};
enum image_flag_s {
	ImageMirrorV = 0x0001,
	ImageMirrorH = 0x0002,
	ImageGrayscale = 0x0004,
	ImageNoOffset = 0x0008,
	ImageTransparent = 0x0010,
	ImageColor = 0x0020,
	ImagePallette = 0x0040,
	TextStroke = 0x0080,
	TextItalic = 0x0100,
	TextBold = 0x0200,
	TextUscope = 0x0400,
	TextSingleLine = 0x0800, // Text would be showed as single line
	AlignLeft = 0x0000,
	AlignCenter = 0x1000,
	AlignRight = 0x2000,
	AlignLeftCenter = 0x3000,
	AlignCenterCenter = 0x4000,
	AlignRightCenter = 0x5000,
	AlignLeftBottom = 0x6000,
	AlignCenterBottom = 0x7000,
	AlignRightBottom = 0x8000,
	AlignWidth = 0xE000,
	AlignMask = 0xF000,
};
enum window_flags {
	WFResize = 0x0010,
	WFMinmax = 0x0020,
	WFMaximized = 0x0040,
	WFAbsolutePos = 0x0080,
};
enum class cursor : unsigned char {
	Arrow, Hand, LeftRight, UpDown, All, No, Edit, Wait,
};
struct pma {
	char				name[4]; // Identifier of current block
	int					size; // Size of all block
	int					count; // Count of records in this block
	int					find(const char* name) const;
	const pma*			getheader(const char* name) const;
	const char*			getstring(int id) const;
};
struct sprite : pma {
	enum encodes { Auto, RAW, RLE, ALC, RAW8, RLE8, BIT };
	struct frame {
		short 			sx, sy;
		short			ox, oy;
		encodes			encode;
		unsigned		pallette;
		unsigned		offset;
		rect			getrect(int x, int y, unsigned flags) const;
	};
	struct cicle {
		short unsigned	start;
		short unsigned	count;
	};
	short int			width, height; // common size of all frames (if applicable)
	short int			ascend, descend; // top or down ascend
	short unsigned		flags; // must be zero
	unsigned			cicles; // count of anim structure
	unsigned			cicles_offset;
	frame				frames[1];
	int					esize() const { return frames[0].offset - (sizeof(sprite) + sizeof(frame)*(count - 1)); }
	const unsigned char* edata() const { return (const unsigned char*)this + sizeof(sprite) + sizeof(frame)*(count - 1); }
	int					ganim(int index, int tick);
	const frame&		get(int id) const { return frames[(id >= count) ? 0 : id]; }
	cicle*				gcicle(int index) { return (cicle*)ptr(cicles_offset) + index; }
	int					glyph(unsigned sym) const;
	const unsigned char* ptr(unsigned o) const { return (unsigned char*)this + o; }
};
namespace colors {
extern color			button, form, window;
extern color			border, active;
extern color			text, h1, h2, h3, special;
namespace tips {
extern color			back;
extern color			text;
}
}
namespace metrics {
extern sprite*			font;
extern sprite*			h1;
extern sprite*			h2;
extern sprite*			h3;
}
namespace draw {
struct hoti {
	enum cursor			cursor; // set this mouse cursor
	unsigned			key; // if pressed key or mouse this field has key
	point				mouse; // current mouse coordinates
	bool				pressed; // flag if any of mouse keys is pressed
	long				param, param2; // command context or parameters
	const void*			object; // command object
	rect				hilite, focus;
	explicit operator bool() const { return key != 0; }
};
extern hoti				hot;
struct surface {
	struct plugin {
		const char*		name;
		const char*		filter;
		plugin*			next;
		static plugin*	first;
		plugin(const char* name, const char* filter);
		virtual bool	decode(unsigned char* output, int output_bpp, const unsigned char* input, unsigned input_size) = 0;
		virtual bool	inspect(int& w, int& h, int& bpp, const unsigned char* input, unsigned size) = 0;
	};
	int					width;
	int					height;
	int					scanline;
	int					bpp;
	unsigned char*		bits;
	surface();
	surface(int width, int height, int bpp);
	surface(const char* url, color* pallette = 0);
	~surface();
	constexpr explicit operator bool() const { return bits != 0; }
	static unsigned char* allocator(unsigned char* bits, unsigned size);
	void				clear() { resize(0, 0, 0, true); }
	void				convert(int bpp, color* pallette);
	void				flipv();
	unsigned char*		ptr(int x, int y);
	bool				read(const char* url, color* pallette = 0, int need_bpp = 0);
	void				resize(int width, int height, int bpp, bool alloc_memory);
	void				write(const char* url, color* pallette);
};
extern surface*			canvas;
extern rect				clipping; // Clipping area
extern fnevent			domodal;
extern point			dragmouse, caret;
extern color			fore, fore_stroke;
extern int				width, height;
extern const sprite*	font; // Currently selected font
extern double			linw;
extern char				link[4096];
extern color*			palt;
extern int				tab_pixels;
int						aligned(unsigned state, int string_width);
int						alignedh(const char* string, unsigned state);
void					blit(surface& dest, int x, int y, int width, int height, unsigned flags, const surface& source, int x_source, int y_source);
void					blit(surface& dest, int x, int y, int width, int height, unsigned flags, const surface& source, int x_source, int y_source, int width_source, int height_source);
void					breakmodal(int result);
void					breakparam();
void					buttoncancel();
void					buttonok();
void					cbsetint();
void					cbsetptr();
void					circle(int radius);
void					circlef(int radius);
void					clearwindow();
void					create(int width, int height, const char* caption);
bool					dragactive(const void* p);
bool					dragactive();
void					dragbegin(const void* p);
void					execute(fnevent proc, long value = 0, long value2 = 0, const void* object = 0);
int						getbpp();
int						getresult();
int						getheight();
int						getwidth();
void					glyph(int x, int y, int sym, unsigned flags);
const sprite*			gres(const char* name, const char* folder = 0, point size = {});
bool					ishilite(const rect& rc);
inline bool				ishilite() { return ishilite({caret.x, caret.y, caret.x + width - 1, caret.y + height - 1}); }
bool					ismodal();
void					image(int x, int y, const sprite* e, int id, int flags, unsigned char alpha = 0xFF);
void					image(int x, int y, const sprite* e, int id, int flags, unsigned char alpha, color* pal);
void					initialize(const char* title);
void					key2str(stringbuilder& sb, int key);
void					line(int x2, int y2); // Draw line
void					linet(int x2, int y2);
void					pixel(int x, int y);
void					pixel(int x, int y, unsigned char alpha);
unsigned char*			ptr(int x, int y);
int						rawinput();
void					rectb(); // Draw rectangle border
void					rectf(); // Draw rectangle area. Right and bottom side is one pixel less.
void					rectx(); // Draw pointed rectangle border
void					setclip(rect rc);
inline void				setclip() { clipping.set(0, 0, getwidth(), getheight()); }
void					setclipdf();
void					setneedupdate();
void					setnext(fnevent v);
void					settimer(unsigned milleseconds);
const char*				skiptr(const char* string);
void					start();
void					startscene(fnevent fn);
void					stroke(int x, int y, const sprite* e, int id, int flags, unsigned char thin = 1, unsigned char* koeff = 0);
void					syscursor(bool enable);
void					sysredraw();
void					text(const char* string, int count = -1, unsigned flags = 0);
void					text(const char* string, int count, unsigned flags, int maximum_width, bool* clipped = 0);
void					textln(const char* string, int count = -1, unsigned flags = 0);
void					textb(const char* string, unsigned state, int* max_width = 0);
void					textc(const char* string, int count = -1, unsigned flags = AlignLeft, bool* clipped = 0);
int						textbc(const char* string, int width);
void					textf(const char* text, int* max_width = 0, int min_height = 0, int* cashe_height = 0, const char** cashe_string = 0);
int						textf(rect& rc, const char* string);
int						texth();
int						texth(const char* string, int width);
int						textw(int sym);
int						textw(const char* string, int count = -1);
int						textw(rect& rc, const char* string);
int						textw(const sprite* font);
void					updatewindow();
void					write(const char* url, unsigned char* bits, int width, int height, int bpp, int scanline, color* pallette);
}