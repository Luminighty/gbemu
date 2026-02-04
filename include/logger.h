#ifndef LOGGER_H
#define LOGGER_H

#ifdef DEBUG_ENABLED
	#define DEBUG(message, ...) printf("[DEBUG] " message, __VA_ARGS__)
#else
	#define DEBUG(message, ...)
#endif

#endif // LOGGER_H
