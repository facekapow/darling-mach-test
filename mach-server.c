#define PRIVATE 1

#include <stdio.h>
#include <mach/mach.h>
#include <servers/bootstrap.h>
#include <sys/event.h>
#include <errno.h>

#define MACH_TEST_SERVICE_SERVER 1
#include "mach-service.h"

#define MAX_EVENTS_PER_CALL 1

int main(int argc, char** argv) {
	mach_port_name_t server_port;
	int result;
	int kq;
	struct kevent_qos_s request = {
		.ident  = 0, // set later
		.filter = EVFILT_MACHPORT,
		.flags  = EV_ADD | EV_ENABLE,
	};
	struct kevent_qos_s events[MAX_EVENTS_PER_CALL] = {0};
	size_t message_count = 0;

	log_msg("trying to get a Mach port...");
	result = mach_port_allocate(mach_task_self(), MACH_PORT_RIGHT_RECEIVE, &server_port);
	if (result != KERN_SUCCESS) {
		log_msg("mach_port_allocate() failed with %d", result);
		return 1;
	}
	log_msg("got port %d from mach_port_allocate() (with a receive right)", server_port);
	request.ident = server_port;

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
	log_msg("registered with launchd");

	log_msg("grabbing a kqueue...");
	kq = kqueue();
	if (kq < 0) {
		result = errno;
		log_msg("kqueue() failed with %d", result);
		return 1;
	}
	log_msg("grabbed kqueue %d", kq);

	log_msg("queueing up the desired events...");
	result = kevent_qos(kq, &request, 1, NULL, 0, NULL, NULL, 0);
	if (result < 0) {
		result = errno;
		log_msg("kevent_qos() failed with %d", result);
		return 1;
	}
	log_msg("queued up an EVFILT_MACHPORT listener successfully");

	while (1) {
		log_msg("waiting for events...");
		result = kevent_qos(kq, NULL, 0, events, sizeof(events) / sizeof(*events), NULL, NULL, 0);
		if (result < 0) {
			result = errno;
			log_msg("kevent_qos() failed with %d", result);
			return 1;
		}
		log_msg("got %d event(s)", result);

		for (int i = 0; i < result; ++i) {
			struct kevent_qos_s* event = &events[i];

			log_msg("event #%d: ident=%llu, filter=%d", i, event->ident, event->filter);

			if (event->ident == server_port && event->filter == EVFILT_MACHPORT) {
				mach_test_service_message_t message = {0};

				result = mach_msg(&message.header, MACH_RCV_MSG, 0, sizeof(message), server_port, MACH_MSG_TIMEOUT_NONE, MACH_PORT_NULL);
				if (result != KERN_SUCCESS) {
					log_msg("mach_msg() failed with %d", result);
					return 1;
				}
				log_msg("got message %zu with text=\"%s\"", message_count++, message.text);
			}
		}
	}

	return 0;
};
