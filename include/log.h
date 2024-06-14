#ifndef __LOG_H__
#define __LOG_H__
#include <stddef.h>

// Enum to define different log types
typedef enum {
	MALLOC,
	FREE_fn,
} log_type;

// Enum to define different log actions
typedef enum {
	START,
	END,
} log_action;

// Function to log a formatted message to a log file
int log_message(const char *format, ...);

// Function to log an event with details such as type, action, address, and size
int log_event(log_type type, log_action action, void *address, size_t size);

// Function to log the start of a new execution
int log_new_execution();

#endif