#pragma once

namespace log {
int				geterrors();
void			error(const char* format, ...);
void			errorv(const char* format);
}
