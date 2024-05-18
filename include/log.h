#ifndef __LOG_H__
#define __LOG_H__
#include <stddef.h>

typedef enum {
	MALLOC,
	FREE_fn,
} log_type;

typedef enum {
	START,
	END,
} log_action;

int log_message(const char *format, ...);
int log_event(log_type type, log_action action, void *address, size_t size);
int log_new_execution();

#endif
