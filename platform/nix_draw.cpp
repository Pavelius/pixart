#include<X11/keysym.h>
#include<X11/Xlib.h>
#include<X11/Xutil.h>
#include<X11/XKBlib.h>
#include "crt.h"
#include "draw.h"

#define KZOOM 2

void scale2x(void* void_dst, unsigned dst_slice, const void* void_src, unsigned src_slice, unsigned width, unsigned height);

using namespace draw;

static Window       hwnd;
static Display*     dpy;
static GC           gc;
static int          scr;
static Window       rootwin;

static int tokey(int vk) {
    switch(vk) {
    case XK_Control_L:
    case XK_Control_R:
        return Ctrl;
    case XK_Alt_L:
    case XK_Alt_R:
        return Alt;
    case XK_Shift_L:
    case XK_Shift_R:
        return Shift;
    case XK_Left:
        return KeyLeft;
    case XK_Right:
        return KeyRight;
    case XK_Up:
        return KeyUp;
    case XK_Down:
        return KeyDown;
    case XK_Prior:
        return KeyPageUp;
    case XK_Next:
        return KeyPageDown;
    case XK_Home:
        return KeyHome;
    case XK_End:
        return KeyEnd;
    case XK_BackSpace:
        return KeyBackspace;
    case XK_Delete:
        return KeyDelete;
    case XK_Return:
        return KeyEnter;
    case XK_Escape:
        return KeyEscape;
    case XK_space:
        return KeySpace;
    case XK_Tab:
        return KeyTab;
    case XK_F1:
        return F1;
    case XK_F2:
        return F2;
    case XK_F3:
        return F3;
    case XK_F4:
        return F4;
    case XK_F5:
        return F5;
    case XK_F6:
        return F6;
    case XK_F7:
        return F7;
    case XK_F8:
        return F8;
    case XK_F9:
        return F9;
    case XK_F10:
        return F10;
    case XK_F11:
        return F11;
    case XK_F12:
        return F12;
    //case VK_MULTIPLY:
    //    return Alpha+(unsigned)'*';
    //case VK_DIVIDE:
    //    return Alpha+(unsigned)'/';
    case XK_plus:
        return '+';
    case XK_minus :
        return '-';
    case XK_comma:
        return ',';
    case XK_period:
        return '.';
    default:
        return vk-XK_A;
    }
}

const char* key2str(int key) {
    static char temp[128];
    temp[0] = 0;
    return temp;
}

static void widget_cleanup() {
    XCloseDisplay(dpy);
}

void draw::create(int canvas_width, int canvas_height) {
    if(hwnd)
        return;
    int width = canvas_width*KZOOM;
    int height = canvas_height*KZOOM;
    XSetWindowAttributes attr = {};
    attr.background_pixmap = None;
    attr.backing_store = NotUseful;
    dpy = XOpenDisplay(0);
    scr	= DefaultScreen(dpy);
    rootwin = RootWindow(dpy,scr);
    atexit(widget_cleanup);
    Visual* myVisual = DefaultVisual(dpy, scr);
    if(canvas)
        canvas->resize(canvas_width, canvas_height, 32, true);
    setclip();
    hwnd = XCreateWindow(dpy, rootwin, 100, 100, width, height, 0, 24, InputOutput, myVisual, CWBackPixmap|CWBackingStore, &attr);
    gc = XCreateGC(dpy, hwnd, 0, NULL);
    XSelectInput(dpy, hwnd,
                 ExposureMask
                 |ButtonPressMask
                 |ButtonReleaseMask
                 |PointerMotionMask
                 |KeyPressMask
                 |StructureNotifyMask
                 |VisibilityChangeMask
                 |LeaveWindowMask);
    XMapWindow(dpy, hwnd);
}

static void update_ui_window() {
    if(!canvas)
        return;
    static surface video_buffer;
	if(video_buffer.height != draw::canvas->height
		|| video_buffer.width != draw::canvas->width)
		video_buffer.resize(draw::canvas->width * KZOOM, draw::canvas->height * KZOOM, 32, true);
	scale2x(video_buffer.ptr(0, 0), video_buffer.scanline,
		draw::canvas->ptr(0, 0), draw::canvas->scanline,
		draw::canvas->width, draw::canvas->height);
    XImage img = {0};
    img.width = video_buffer.width;
    img.height = video_buffer.height;
    img.format = ZPixmap;
    img.data = (char*)video_buffer.ptr(0,0);
    img.bitmap_unit = 32;
    img.bitmap_pad = 32;
    img.depth = 24;
    img.bits_per_pixel = 32;
    if(XInitImage(&img))
        XPutImage(dpy, hwnd, gc, &img, 0,0,0,0, img.width,img.height);
}

static int mousekey(int k, bool dbl = false) {
    switch(k) {
    case Button1: return dbl ? MouseLeftDBL : MouseLeft;
    case Button3: return MouseRight;
    case Button4: return MouseWheelUp;
    case Button5: return MouseWheelDown;
    default: return 0;
    }
}

static int read_keyboard(char* result, unsigned maximum) {
    //Xutf8LookupString(ic, &e.xkey, temp, sizeof(temp)-1, 0, 0);
}

static void apply_keyboard_mask(unsigned mask) {
    if((mask&ShiftMask)!=0)
        hot.key |= Shift;
    if((mask&ControlMask)!=0)
        hot.key |= Ctrl;
    if((mask&LockMask)!=0)
        hot.key |= Alt;
}

static bool handle(XEvent& e) {
    XEvent e1;
    static unsigned long last_mouse_press;
    switch(e.type) {
    case Expose:
        hot.key = InputUpdate;
        break;
    case ButtonPress:
        hot.key = mousekey(e.xbutton.button, (e.xbutton.time-last_mouse_press) < 300);
        apply_keyboard_mask(e.xbutton.state);
        hot.pressed = true;
        last_mouse_press = e.xbutton.time;
        break;
    case ButtonRelease:
        hot.key = mousekey(e.xbutton.button);
        apply_keyboard_mask(e.xbutton.state);
        hot.pressed = false;
        break;
    case KeyPress:
        hot.key = tokey(XkbKeycodeToKeysym(dpy, e.xkey.keycode, 0, e.xkey.state & ShiftMask ? 1 : 0));
        apply_keyboard_mask(e.xkey.state);
        break;
    case MotionNotify:
        while(XCheckTypedWindowEvent(dpy, e.xmotion.window, e.type, &e1));
        hot.key = MouseMove;
        hot.mouse.x = e.xmotion.x / KZOOM;
        hot.mouse.y = e.xmotion.y / KZOOM;
        break;
    case VisibilityNotify:
        hot.key = InputUpdate;
        break;
    case LeaveNotify:
        hot.key = MouseMove;
        hot.pressed = false;
        hot.mouse.x = -1000;
        hot.mouse.y = -1000;
        break;
    case DestroyNotify:
        hot.key = 0;
        break;
    default: return false;
    }
    return true;
}

void draw::sysredraw() {
}

int draw::rawinput() {
    if(!hwnd)
        return 0;
    update_ui_window();
    while(true) {
        XEvent e;
        XNextEvent(dpy,&e);
        if(XFilterEvent(&e, (Window)hwnd))
            continue;
        if(!handle(e))
            continue;
        break;
    }
	return hot.key;
}

void draw::setcaption(const char* format) {
    Xutf8SetWMProperties(dpy, hwnd, format, 0, 0, 0, 0, 0, 0);
}

void draw::settimer(unsigned milleseconds) {
}
