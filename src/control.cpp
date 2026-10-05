#include "Messages.h"

#include <Application.h>
#include <Message.h>
#include <Messenger.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

static void
Usage()
{
	fprintf(stderr, "usage: fatcat-cli {about|start|pause|resume|stop|preview|dismiss|status|quit|configure FOCUS BREAK}\n");
}
//---------------------------------------------------------------------------------------------------------------------------------//


int
main(int argc, char** argv)
{
	BApplication application("application/x-vnd.arkane-FatCatControl");
	if (argc < 2) { Usage(); return 2; }
	if (strcmp(argv[1], "about") == 0 || strcmp(argv[1], "--about") == 0) {
		printf("Fat Cat Pomodoro %s for Haiku\n%s\n", kAppVersion,
			kAppDescription);
		return 0;
	}
	BMessenger target(kAppSignature);
	if (!target.IsValid()) {
		fprintf(stderr, "Fat Cat is not running. Start it from Deskbar first.\n");
		return 1;
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
	} else if (strcmp(argv[1], "status") == 0) {
		message.what = kMsgStatus;
		BMessage reply;
		status_t result = target.SendMessage(&message, &reply, 1000000, 1000000);
		if (result != B_OK) return 1;
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
	} else {
		Usage();
		return 2;
	}
	return target.SendMessage(&message) == B_OK ? 0 : 1;
}
//---------------------------------------------------------------------------------------------------------------------------------//
