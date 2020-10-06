#ifndef _DARLING_TESTS_MACH_SERVICE_H_
#define _DARLING_TESTS_MACH_SERVICE_H_

#include <stdio.h>
#include <mach/mach.h>

#define MACH_TEST_SERVICE_NAME "org.darlinghq.mach-test"

#if 0
	#include <libdebug/libdebug.h>
	#define log_msg(fmt, ...) libdebug_log(fmt, ## __VA_ARGS__)
#else
	#define log_msg(fmt, ...) printf(fmt "\n", ## __VA_ARGS__)
#endif

typedef struct mach_test_service_message {
	mach_msg_header_t header;
	char text[32];
#if MACH_TEST_SERVICE_SERVER
	mach_msg_trailer_t trailer;
#endif
} mach_test_service_message_t;

#endif // _DARLING_TESTS_MACH_SERVICE_H_
