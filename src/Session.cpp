#include "Session.h"

#include <algorithm>
#include <cstdio>

int32
Session::Remaining(time_t now) const
{
	if (phase == Phase::Idle)
		return 0;
	if (paused)
		return remainingSeconds;
	return std::clamp<int64>(deadline - now, 0, durationSeconds);
}

BString
Session::Countdown(time_t now) const
{
	int32 seconds = Remaining(now);
	char value[16];
	snprintf(value, sizeof(value), "%02ld:%02ld", (long)(seconds / 60),
		(long)(seconds % 60));
	return value;
}

void
Session::_Enter(Phase next, int32 minutes, time_t now)
{
	phase = next;
	paused = false;
	durationSeconds = minutes * 60;
	remainingSeconds = durationSeconds;
	deadline = now + durationSeconds;
}

bool
Session::Tick(time_t now, bool& completedBreak)
{
	completedBreak = false;
	if (phase == Phase::Idle || paused)
		return false;

	// A backwards clock adjustment must not extend an interval.
	if (deadline - now > durationSeconds) {
		deadline = now + durationSeconds;
		return true;
	}
	if (Remaining(now) > 0)
		return false;

	if (phase == Phase::Focus) {
		completedFocus = std::min<int32>(1000000000, completedFocus + 1);
		_Enter(Phase::Break,
			completedFocus % longBreakEvery == 0 ? longBreakMinutes : breakMinutes,
			now);
	} else {
		completedBreaks = std::min(completedFocus, completedBreaks + 1);
		completedBreak = true;
		_Enter(Phase::Focus, focusMinutes, now);
	}
	return true;
}

void Session::Start(time_t now) { if (phase == Phase::Idle) _Enter(Phase::Focus, focusMinutes, now); }

void
Session::Stop()
{
	phase = Phase::Idle;
	paused = false;
	deadline = 0;
	remainingSeconds = 0;
	durationSeconds = 0;
}

void
Session::Pause(time_t now)
{
	bool ignored;
	Tick(now, ignored);
	if (phase == Phase::Idle || paused)
		return;
	remainingSeconds = Remaining(now);
	deadline = 0;
	paused = true;
}

void
Session::Resume(time_t now)
{
	if (!paused)
		return;
	paused = false;
	deadline = now + remainingSeconds;
}

void
Session::SkipBreak(time_t now)
{
	if (phase == Phase::Break)
		_Enter(Phase::Focus, focusMinutes, now);
}

bool
Session::Configure(int32 focus, int32 rest, int32 longRest, int32 every)
{
	if (focus < 1 || focus > 180 || rest < 1 || rest > 180
		|| longRest < 1 || longRest > 180 || every < 2 || every > 12)
		return false;
	focusMinutes = focus;
	breakMinutes = rest;
	longBreakMinutes = longRest;
	longBreakEvery = every;
	return true;
}

void
Session::Archive(BMessage& into) const
{
	into.MakeEmpty();
	into.AddInt32("version", 1);
	into.AddInt32("phase", (int32)phase);
	into.AddBool("paused", paused);
	into.AddInt64("deadline", deadline);
	into.AddInt32("remaining", remainingSeconds);
	into.AddInt32("duration", durationSeconds);
	into.AddInt32("focus", focusMinutes);
	into.AddInt32("break", breakMinutes);
	into.AddInt32("long_break", longBreakMinutes);
	into.AddInt32("long_every", longBreakEvery);
	into.AddInt32("completed_focus", completedFocus);
	into.AddInt32("completed_breaks", completedBreaks);
}

bool
Session::Restore(const BMessage& from, time_t now)
{
	int32 version, phaseValue, focus, rest, longRest, every, completedF,
		completedB, remaining, duration;
	int64 savedDeadline;
	bool isPaused;
	if (from.FindInt32("version", &version) != B_OK || version != 1
		|| from.FindInt32("phase", &phaseValue) != B_OK || phaseValue < 0 || phaseValue > 2
		|| from.FindBool("paused", &isPaused) != B_OK
		|| from.FindInt64("deadline", &savedDeadline) != B_OK
		|| from.FindInt32("remaining", &remaining) != B_OK
		|| from.FindInt32("duration", &duration) != B_OK
		|| from.FindInt32("focus", &focus) != B_OK
		|| from.FindInt32("break", &rest) != B_OK
		|| from.FindInt32("long_break", &longRest) != B_OK
		|| from.FindInt32("long_every", &every) != B_OK
		|| from.FindInt32("completed_focus", &completedF) != B_OK
		|| from.FindInt32("completed_breaks", &completedB) != B_OK
		|| focus < 1 || focus > 180 || rest < 1 || rest > 180
		|| longRest < 1 || longRest > 180 || every < 2 || every > 12
		|| completedF < 0 || completedB < 0 || completedB > completedF
		|| duration < 0 || duration > 10800 || remaining < 0 || remaining > duration
		|| savedDeadline < 0) {
		*this = Session();
		return false;
	}

	phase = (Phase)phaseValue;
	paused = isPaused;
	deadline = savedDeadline;
	remainingSeconds = remaining;
	durationSeconds = duration;
	focusMinutes = focus;
	breakMinutes = rest;
	longBreakMinutes = longRest;
	longBreakEvery = every;
	completedFocus = completedF;
	completedBreaks = completedB;

	if (phase == Phase::Idle) {
		if (paused || deadline != 0 || remaining != 0 || duration != 0) {
			*this = Session();
			return false;
		}
	} else if (duration < 60 || (paused ? deadline != 0 || remaining == 0 : deadline == 0)) {
		*this = Session();
		return false;
	}
	bool ignored;
	Tick(now, ignored); // Exactly one overdue transition; never replay missed cycles.
	return true;
}
