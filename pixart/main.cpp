#include "draw.h"
#include "draw_interface.h"

using namespace draw;

static void test_modal() {
	clearwindow();
	width = 200;
	caret = {10, 10};
	textf("»спользуй вс€ческие уведомлени€ чтобы получить нужный эффект.");
}

int main() {
	initialize("Test Pixart Games");
	startscene(test_modal);
	return 0;
}

#ifdef _MSC_VER
int __stdcall WinMain(void* ci, void* pi, char* cmd, int sw) {
	return main();
}
#endif