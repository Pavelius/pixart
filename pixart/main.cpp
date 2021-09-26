#include "draw.h"

void load_buffer();
void zoom_editor();
void main_util();
void sprite_viewer();

int main() {
#ifdef _DEBUG
	main_util();
#endif // _DEBUG
	draw::initialize("Test Pixart Games");
	load_buffer();
	draw::startscene(zoom_editor);
	return 0;
}

#ifdef _MSC_VER
int __stdcall WinMain(void* ci, void* pi, char* cmd, int sw) {
	return main();
}
#endif