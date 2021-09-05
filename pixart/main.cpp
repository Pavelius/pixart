#include "draw.h"

using namespace draw;

static void test_modal() {
	while(ismodal()) {
		width = 100;
		height = 100;
		caret.x = 50; caret.y = 50;
		fore = colors::form;
		rectf();
		fore = colors::border;
		rectb();
		auto pb = gres("tavern15", "art\\scene");
		image(0, 0, pb, 0, 0);
		caret = {100, 100};
		line(250, 200);
		domodal();
	}
}

int main() {
	initialize("Test Pixart Games");
	test_modal();
	return 0;
}

#ifdef _MSC_VER
int __stdcall WinMain(void* ci, void* pi, char* cmd, int sw) {
	return main();
}
#endif