#pragma once

typedef void (*fnevent)();

namespace draw {
void					breakmodal(int result);
void					breakparam();
void					buttoncancel();
void					buttonok();
void					cbsetint();
void					cbsetptr();
void					clearwindow();
void					setneedupdate();
void					setnext(fnevent v);
void					startscene(fnevent fn);
}
