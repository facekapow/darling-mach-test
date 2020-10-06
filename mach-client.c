#include <stdio.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>

#include "mach-service.h"

int main(int argc, char** argv) {
	mach_port_name_t target_port;
	int result;

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
		.text = "Hello!",
	};

	log_msg("sending a message...");
	result = mach_msg(&message.header, MACH_SEND_MSG, sizeof(message), 0, MACH_PORT_NULL, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
	if (result != KERN_SUCCESS) {
		log_msg("mach_msg() failed with %d", result);
		return 1;
	}
	log_msg("sent message");

	return 0;
};
