#include "crt.h"
#include "draw.h"
#include "draw_interface.h"

static bool			break_modal;
static long			break_result;
static fnevent		next_proc;

static bool button(const char* title, const char* tips, unsigned key) {
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

bool draw::ismodal() {
	hot.cursor = cursor::Arrow;
	hot.hilite.clear();
	if(hot.key == InputNeedUpdate)
		hot.key = InputUpdate;
	else
		domodal = standart_domodal;
	//before_modal->execute();
	if(!next_proc && !break_modal)
		return true;
	break_modal = false;
	//leave_modal->execute();
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
	draw::create(400, 300, title);
}