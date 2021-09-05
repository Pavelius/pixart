#include "io_stream.h"
#include "log.h"
#include "stringbuilder.h"

static int error_count;

void log::errorv(const char* format) {
	static io::file file("errors.txt", StreamWrite|StreamText);
	if(!file)
		return;
	error_count++;
	file << format << "\n";
}

void log::error(const char* format, ...) {
	char temp[4096]; stringbuilder sb(temp);
	sb.addv(format, xva_start(format));
	errorv(temp);
}

int log::geterrors() {
	return error_count;
}