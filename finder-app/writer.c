#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

int main(int argc, char *argv[]) {
	openlog("writer", LOG_PID, LOG_USER);

	// Parameter check
	if (argc != 3) {
		syslog(LOG_ERR, "Invalid number of arguments. Expected 2, got %d.", argc - 1);
		printf("Usage: %s <write_file> <write_string>\n", argv[0]);
		closelog();
		return 1;
	}

	const char *write_file = argv[1];
	const char *write_string = argv[2];
	size_t string_len = strlen(write_string);

	// Log the debug message
	syslog(LOG_DEBUG, "Writing %s to %s", write_string, write_file);

	// Open the file using POSIX system calls
	int fd = open(write_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if (fd == -1) {
		syslog(LOG_ERR, "Error opening or creating file '%s': %s", write_file, strerror(errno));
		printf("Error: Could not open file '%s'.\n", write_file);
		closelog();
		return 1;
	}

	// Write the string to the file
	ssize_t bytes_written = write(fd, write_string, string_len);
	if (bytes_written == -1) {
		syslog(LOG_ERR, "Error writing to file '%s': %s", write_file, strerror(errno));
		printf("Error: Could not write to file '%s'.\n", write_file);
		close(fd);
		closelog();
		return 1;
	} else if ((size_t)bytes_written != string_len) {
		syslog(LOG_ERR, "Partial write error on file '%s'. Attempted %zu bytes, wrote %zd bytes.", write_file, string_len, bytes_written);
		close(fd);
		closelog();
		return 1;
	}

	// Clean up
	if (close(fd) == -1) {
		syslog(LOG_ERR, "Error closing file '%s': %s", write_file, strerror(errno));
	}

	closelog();
	return 0;
}
