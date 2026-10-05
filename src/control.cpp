#include "Messages.h"

#include <Application.h>
#include <Message.h>
#include <Messenger.h>
#include <OS.h>
#include <Roster.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

static void
Usage()
{
	fprintf(stderr, "usage: fatcat-cli {about|start|pause|resume|stop|preview|dismiss|status|quit|configure FOCUS BREAK}\n");
}
//---------------------------------------------------------------------------------------------------------------------------------//


static bool
FindOrLaunchApplication(BMessenger& target)
{
	target = BMessenger(kAppSignature);
	if (target.IsValid())
		return true;

	const char* arguments[] = { "--background" };
	status_t status = be_roster->Launch(kAppSignature, 1, arguments);
	if (status != B_OK && status != B_ALREADY_RUNNING)
		return false;

	for (int32 attempt = 0; attempt < 40; attempt++) {
		snooze(50000);
		target = BMessenger(kAppSignature);
		if (target.IsValid())
			return true;
	}
	return false;
}
//---------------------------------------------------------------------------------------------------------------------------------//


int
main(int argc, char** argv)
{
	BApplication application("application/x-vnd.arkane-FatCatControl");
	if (argc < 2) { Usage(); return 2; }
	if (strcmp(argv[1], "about") == 0 || strcmp(argv[1], "--about") == 0) {
		printf("Fat Cat Pomodoro v%s for Haiku\n%s\n", kAppVersion,
			kAppDescription);
		return 0;
	}
	BMessage message;
	if (strcmp(argv[1], "start") == 0) message.what = kMsgStart;
	else if (strcmp(argv[1], "pause") == 0) message.what = kMsgPause;
	else if (strcmp(argv[1], "resume") == 0) message.what = kMsgResume;
	else if (strcmp(argv[1], "stop") == 0) message.what = kMsgStop;
	else if (strcmp(argv[1], "preview") == 0) message.what = kMsgPreview;
	else if (strcmp(argv[1], "dismiss") == 0) message.what = kMsgDismiss;
	else if (strcmp(argv[1], "quit") == 0) message.what = kMsgQuit;
	else if (strcmp(argv[1], "configure") == 0 && argc == 4) {
		char* end = nullptr;
		long focus = strtol(argv[2], &end, 10);
		if (*end || focus < 1 || focus > 180) { Usage(); return 2; }
		long rest = strtol(argv[3], &end, 10);
		if (*end || rest < 1 || rest > 180) { Usage(); return 2; }
		message.what = kMsgSaveSettings;
		message.AddInt32("focus", focus);
		message.AddInt32("break", rest);
	} else if (strcmp(argv[1], "status") == 0)
		message.what = kMsgStatus;
	else {
		Usage();
		return 2;
	}

	BMessenger target(kAppSignature);
	if (!target.IsValid() && message.what == kMsgQuit)
		return 0;
	if (!FindOrLaunchApplication(target)) {
		fprintf(stderr, "Fat Cat could not be started. Run make install and try again.\n");
		return 1;
	}

	if (message.what == kMsgStatus) {
		BMessage reply;
		status_t result = target.SendMessage(&message, &reply, 1000000, 1000000);
		if (result != B_OK) {
			fprintf(stderr, "Fat Cat did not respond to the status request.\n");
			return 1;
		}
		const char* version = "", *countdown = "", *error = "";
		int32 phase = 0, remaining = 0, breaks = 0;
		bool paused = false, preview = false;
		reply.FindString("version", &version);
		reply.FindInt32("phase", &phase);
		reply.FindBool("paused", &paused);
		reply.FindInt32("remaining", &remaining);
		reply.FindString("countdown", &countdown);
		reply.FindBool("preview", &preview);
		reply.FindInt32("completed_breaks", &breaks);
		reply.FindString("persistence_error", &error);
		const char* phaseName[] = { "idle", "focus", "break" };
		printf("{\"version\":\"%s\",\"phase\":\"%s\",\"paused\":%s,"
			"\"remaining\":%ld,\"countdown\":\"%s\",\"preview\":%s,"
			"\"completedBreaks\":%ld,\"persistenceError\":\"%s\"}\n",
			version, phase >= 0 && phase <= 2 ? phaseName[phase] : "unknown",
			paused ? "true" : "false", (long)remaining, countdown,
			preview ? "true" : "false", (long)breaks, error);
		return 0;
	}

	status_t result = target.SendMessage(&message, (BHandler*)nullptr, 1000000);
	if (result != B_OK)
		fprintf(stderr, "Fat Cat did not respond to the command.\n");
	return result == B_OK ? 0 : 1;
}
//---------------------------------------------------------------------------------------------------------------------------------//
