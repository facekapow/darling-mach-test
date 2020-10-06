#define PRIVATE 1

#include <stdio.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>
#include <errno.h>
#include <dispatch/dispatch.h>
#include <dispatch/private.h>

#define MACH_TEST_SERVICE_SERVER 1
#include "mach-service.h"

#define MAX_EVENTS_PER_CALL 1

void handle_mach_messages(void* context, dispatch_mach_reason_t reason, dispatch_mach_msg_t dmessage, mach_error_t error) {
	switch (reason) {
		case DISPATCH_MACH_CONNECTED: {
			log_msg("channel connected");
		} break;

		case DISPATCH_MACH_MESSAGE_RECEIVED: {
			log_msg("got a message");

			size_t size;
			mach_test_service_message_t* message;

			message = (mach_test_service_message_t*)dispatch_mach_msg_get_msg(dmessage, &size);

			log_msg("got a message with text=%s", message->text);

			mach_msg_destroy((mach_msg_header_t*)message);
		} break;
	}
};

int main(int argc, char** argv) {
	mach_port_name_t server_port;
	int result;
	dispatch_workloop_t workloop;
	dispatch_mach_t mach_channel;

	log_msg("trying to get a Mach port...");
	result = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &server_port);
	if (result != KERN_SUCCESS) {
		log_msg("mach_port_allocate() failed with %d", result);
		return 1;
	}
	log_msg("got port %d from mach_port_allocate() (with a receive right)", server_port);

	log_msg("grabbing a send right to our port...");
	result = mach_port_insert_right(mach_task_self(), server_port, server_port, MACH_MSG_TYPE_MAKE_SEND);
	if (result != KERN_SUCCESS) {
		log_msg("mach_port_insert_right() failed with %d", result);
		return 1;
	}
	log_msg("got a send right to our port");

	log_msg("registering with launchd...");
	result = bootstrap_register(bootstrap_port, MACH_TEST_SERVICE_NAME, server_port);
	if (result != KERN_SUCCESS) {
		log_msg("bootstrap_register() failed with %d", result);
		return 1;
	}
	log_msg("successfully registered with launchd");

	log_msg("creating an inactive workloop...");
	workloop = dispatch_workloop_create_inactive("org.darlinghq.mach-test.main");
	dispatch_set_qos_class_fallback(workloop, QOS_CLASS_UTILITY);
	dispatch_activate(workloop);
	log_msg("actived workloop %p", workloop);

	log_msg("creating mach channel...");
	mach_channel = dispatch_mach_create_f("org.darlinghq.mach-test.channel", workloop, NULL, handle_mach_messages);
	dispatch_set_qos_class_fallback(mach_channel, QOS_CLASS_BACKGROUND);
	log_msg("created mach channel %p", mach_channel);

	log_msg("attaching server port to mach channel...");
	dispatch_mach_connect(mach_channel, server_port, MACH_PORT_NULL, NULL);
	log_msg("attached server port to mach channel");

	log_msg("handing off to libdispatch via dispatch_main()...");
	dispatch_main();

	return 0;
};
