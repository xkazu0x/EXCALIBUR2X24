#include "ex_logger.h"

#include <stdio.h>
#include <stdarg.h>

void ex::logger::log_output(log_level level, const char *message, ...) {
	const char *levels[5] = {
		"[FATAL]: ",
		"[ERROR]: ",
		"[WARN]: ",
		"[INFO]: ",
		"[DEBUG]: "
	};
	
	va_list arg_ptr;
	char text[1024];

	va_start(arg_ptr, message);
	vsnprintf(text, sizeof(text), message, arg_ptr);
	va_end(arg_ptr);

	char out_message[1024];
	sprintf_s(out_message, "%s%s\n", levels[level], text);
	fputs(out_message, stdout);
}
