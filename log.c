// Log function to save log messages to a file in the same directory as the executable
#include "log.h"
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <alloca.h>
#include <string.h>

int my_log_message(const char *format, ...)	 
{
	/* write(2, "logmessage\n", 11); */
	// Check if the environment variable MSM_OUTPUT is set
	if (getenv("MSM_OUTPUT") == NULL)
		return 0;

	va_list args, args_copy;
	va_start(args, format);
	
    // Determine the required buffer size for the formatted string
	va_copy(args_copy, args);
    size_t size = vsnprintf(NULL, 0, format, args_copy) + 1; // +1 for the null terminator
	va_end(args_copy);
		
    // Allocate buffer on the stack
    char *buffer = (char *)alloca(size);

    // Write formatted data to the buffer
    vsnprintf(buffer, size, format, args);
	va_end(args);

    // Open log file with appropriate flags and permissions
	int fd = open(getenv("MSM_OUTPUT"), O_CREAT | O_APPEND | O_WRONLY, 0600); // 600 - rw for owner
	if (fd == -1) {
		return -1;
	}
	
	// Write log message to log file
	int ret = write(fd, buffer, strlen(buffer));
	close(fd);

	return ret == -1 ? -1 : 0;	
}

