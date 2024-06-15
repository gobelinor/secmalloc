#ifndef LOG_H
#define LOG_H

#include <stddef.h>

/**
 * @file log.h
 * @brief Header file for logging functions.
 *
 * This file contains the declarations for logging functions used
 * throughout the project.
 */

/**
 * @brief Function to log a formatted message to a log file.
 *
 * This function logs a formatted message to the specified log file.
 *
 * @param format The format string (printf-like).
 * @param ... Additional arguments for the format string.
 * @return int Return 0 on success, or a negative error code on failure.
 */
int my_log_message(const char *format, ...);

#endif // LOG_H
