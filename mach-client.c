#include <stdio.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>
#include <stdlib.h>

#include "mach-service.h"

int main(int argc, char** argv) {
	mach_port_name_t target_port;
	int result;
	unsigned long long times = 1;

	if (argc > 1) {
		times = atoll(argv[1]);
	}

	log_msg("looking up server port...");
	result = bootstrap_look_up(bootstrap_port, MACH_TEST_SERVICE_NAME, &target_port);
	if (result != KERN_SUCCESS) {
		log_msg("bootstrap_look_up() failed with %d", result);
		return 1;
	}
	log_msg("found server with port %d", target_port);

	mach_test_service_message_t message = {
		.header = {
			.msgh_bits        = MACH_MSGH_BITS(MACH_MSG_TYPE_COPY_SEND, 0),
			.msgh_remote_port = target_port,
			.msgh_local_port  = MACH_PORT_NULL,
		},
	};

	for (unsigned long long i = 0; i < times; ++i) {
		log_msg("--- message %llu ---", i);

		log_msg("customizing message...");
		result = snprintf(message.text, sizeof(message.text) / sizeof(char), "Hi from iteration %llu", i);
		if (result < 0) {
			log_msg("snprintf() failed with %d", result);
			return 1;
		}
		log_msg("customized message");

		log_msg("sending message...");
		result = mach_msg(&message.header, MACH_SEND_MSG, sizeof(message), 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
		if (result != KERN_SUCCESS) {
			log_msg("mach_msg() failed with %d", result);
			return 1;
		}
		log_msg("sent message");
	}

	return 0;
};
