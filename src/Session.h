#pragma once

#include <Message.h>
#include <String.h>
#include <SupportDefs.h>

#include <ctime>

enum class Phase : int32 { Idle = 0, Focus = 1, Break = 2 };

struct Session {
	Phase phase = Phase::Idle;
	bool paused = false;
	int64 deadline = 0;
	int32 remainingSeconds = 0;
	int32 durationSeconds = 0;
	int32 focusMinutes = 25;
	int32 breakMinutes = 5;
	int32 longBreakMinutes = 15;
	int32 longBreakEvery = 4;
	int32 completedFocus = 0;
	int32 completedBreaks = 0;

	int32 Remaining(time_t now) const;
	BString Countdown(time_t now) const;
	bool Tick(time_t now, bool& completedBreak);
	void Start(time_t now);
	void Stop();
	void Pause(time_t now);
	void Resume(time_t now);
	void SkipBreak(time_t now);
	bool Configure(int32 focus, int32 rest, int32 longRest, int32 every);
	void Archive(BMessage& into) const;
	bool Restore(const BMessage& from, time_t now);

private:
	void _Enter(Phase next, int32 minutes, time_t now);
};

