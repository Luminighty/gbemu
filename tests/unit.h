#ifndef UNIT_H
#define UNIT_H

#include <stdio.h>

#define SUCCESS 1
#define FAIL 0

#define TEST_SETUP() int _status = SUCCESS
#define TEST_FINISH() return _status == FAIL
#define TEST_RUN(fn) do { if (fn() == FAIL) _status = false; } while(0)


#define assert(value, message, ...) \
	do { if (!(value)) { \
		fprintf( \
			stderr, __FILE__":%d"  " [ASSERT] "message"\n", \
			__LINE__, ##__VA_ARGS__ \
		); \
		return FAIL; \
	} } while(0)

#define assertm_eq(got, expected, formatter, message, ...) \
	assert((expected) == (got), message "\n\tExpected " formatter ", but got " formatter , ##__VA_ARGS__, (expected), (got))

#define assert_eq(got, expected, formatter) \
	assert((expected) == (got), "Expected " formatter ", but got " formatter,(expected), (got))


#endif // UNIT_H
