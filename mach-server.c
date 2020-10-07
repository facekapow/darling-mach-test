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

#if 0
void handle_notification_messages(void* context, dispatch_mach_reason_t reason, dispatch_mach_msg_t dmessage, mach_error_t error) {
	switch (reason) {
		case DISPATCH_MACH_CONNECTED: {
			log_msg("notification channel connected");
		} break;

		case DISPATCH_MACH_MESSAGE_RECEIVED: {
			log_msg("got a notification message");

			size_t size;
			mach_test_service_message_t* message;

			message = (mach_test_service_message_t*)dispatch_mach_msg_get_msg(dmessage, &size);

			// TODO: print the message info

			mach_msg_destroy((mach_msg_header_t*)message);
		} break;
	};
};
#endif

void handle_mach_messages(void* context, dispatch_mach_reason_t reason, dispatch_mach_msg_t dmessage, mach_error_t error) {
	switch (reason) {
		case DISPATCH_MACH_CONNECTED: {
			log_msg("server channel connected");
		} break;

		case DISPATCH_MACH_MESSAGE_RECEIVED: {
			log_msg("got a server message");

			size_t size;
			mach_test_service_message_t* message;

			message = (mach_test_service_message_t*)dispatch_mach_msg_get_msg(dmessage, &size);

			log_msg("got a server message with text=%s", message->text);

			mach_msg_destroy((mach_msg_header_t*)message);
		} break;
	}
};

#if 0
void handle_sigusr1(void* context) {
	log_msg("SIGUSR1 fired (with context %p)", context);
};

void handle_sigusr2(void* context) {
	log_msg("SIGUSR2 fired (with context %p)", context);
};

void handle_sigwinch(void* context) {
	log_msg("SIGWINCH fired (with context %p)", context);
};

void handle_timer(void* context) {
	log_msg("timer fired (with context %p)", context);
};
#endif

int main(int argc, char** argv) {
	mach_port_name_t server_port;
	mach_port_name_t notification_port;
	int result;
	dispatch_workloop_t workloop;
	dispatch_mach_t notification_channel;
	dispatch_mach_t mach_channel;
	dispatch_source_t sigusr1_source;
	dispatch_source_t sigusr2_source;
	dispatch_source_t sigwinch_source;
	dispatch_source_t timer_source;
	mach_port_options_t opts = {
		.flags = MPO_STRICT | MPO_CONTEXT_AS_GUARD,
	};
	dispatch_time_t timer_start;

	log_msg("allocating a server port...");
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

#if 0
	log_msg("constructing a notification port...");
	result = mach_port_construct(current_task(), &opts, NULL, &notification_port);
	if (result != KERN_SUCCESS) {
		log_msg("mach_port_insert_right() failed with %d", result);
		return 1;
	}
	log_msg("got port %d from mach_port_construct()", notification_port);
#endif

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

#if 0
	log_msg("creating mach notification channel...");
	notification_channel = dispatch_mach_create_f("org.darlinghq.mach-test.notification-channel", workloop, NULL, handle_notification_messages);
	dispatch_set_qos_class_fallback(notification_channel, QOS_CLASS_USER_INITIATED);
	log_msg("created mach notification channel %p", mach_channel);

	log_msg("attaching notification port to mach notification channel...");
	dispatch_mach_connect(notification_channel, notification_port, MACH_PORT_NULL, NULL);
	log_msg("attached notification port to mach notification channel");
#endif

	log_msg("creating mach server channel...");
	mach_channel = dispatch_mach_create_f("org.darlinghq.mach-test.server-channel", workloop, NULL, handle_mach_messages);
	dispatch_set_qos_class_fallback(mach_channel, QOS_CLASS_BACKGROUND);
	log_msg("created mach server channel %p", mach_channel);

	log_msg("attaching server port to mach server channel...");
	dispatch_mach_connect(mach_channel, server_port, MACH_PORT_NULL, NULL);
	log_msg("attached server port to mach server channel");

#if 0
	log_msg("setting up SIGUSR1...");
	sigusr1_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, (uintptr_t)SIGUSR1, 0, workloop);
	dispatch_source_set_event_handler_f(sigusr1_source, handle_sigusr1);
	dispatch_activate(sigusr1_source);
	log_msg("SIGUSR1 set up");

	log_msg("setting up SIGUSR2...");
	sigusr2_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, (uintptr_t)SIGUSR2, 0, workloop);
	dispatch_source_set_event_handler_f(sigusr2_source, handle_sigusr2);
	dispatch_activate(sigusr2_source);
	log_msg("SIGUSR2 set up");

	log_msg("setting up SIGWINCH...");
	sigwinch_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_SIGNAL, (uintptr_t)SIGWINCH, 0, workloop);
	dispatch_source_set_event_handler_f(sigwinch_source, handle_sigwinch);
	dispatch_activate(sigwinch_source);
	log_msg("SIGWINCH set up");

	log_msg("setting up timer...");
	timer_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, workloop);
	timer_start = dispatch_walltime(NULL, 0);
	dispatch_source_set_timer(timer_source, timer_start, 1 * NSEC_PER_SEC, 0);
	dispatch_source_set_event_handler_f(timer_source, handle_timer);
	dispatch_activate(timer_source);
	log_msg("timer set up");
#endif

	log_msg("handing off to libdispatch via dispatch_main()...");
	dispatch_main();

	return 0;
};
