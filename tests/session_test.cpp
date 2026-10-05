#include "Session.h"

#include <cassert>

int
main()
{
	const time_t now = 100000;
	Session session;
	assert(session.phase == Phase::Idle);
	session.Start(now);
	assert(session.phase == Phase::Focus);
	assert(session.Remaining(now) == 25 * 60);

	session.Pause(now + 12);
	assert(session.paused && session.Remaining(now + 500) == 25 * 60 - 12);
	session.Resume(now + 500);
	assert(!session.paused && session.Remaining(now + 500) == 25 * 60 - 12);

	bool reward = false;
	assert(session.Tick(session.deadline, reward));
	assert(session.phase == Phase::Break && session.completedFocus == 1 && !reward);
	session.SkipBreak(now + 1000);
	assert(session.phase == Phase::Focus && session.completedBreaks == 0);

	// Four focus completions choose the long break. Skips never earn cats.
	for (int i = 1; i < 4; ++i) {
		session.Tick(session.deadline, reward);
		assert(session.phase == Phase::Break);
		session.SkipBreak(session.deadline);
	}
	assert(session.completedFocus == 4);
	assert(session.durationSeconds == 25 * 60); // skip entered focus
	session.Tick(session.deadline, reward);
	assert(session.completedFocus == 5); // next cycle is ordinary

	// Completing a real break earns exactly one reward and starts full focus.
	session.Tick(session.deadline, reward);
	assert(reward && session.completedBreaks == 1 && session.phase == Phase::Focus);

	// Restore advances one overdue phase, never replays a backlog.
	Session saved;
	saved.Start(now);
	BMessage archive;
	saved.Archive(archive);
	Session restored;
	assert(restored.Restore(archive, now + 100000));
	assert(restored.phase == Phase::Break && restored.completedFocus == 1);
	assert(restored.Remaining(now + 100000) == restored.breakMinutes * 60);

	// A backwards wall clock adjustment is capped to one full interval.
	int64 oldDeadline = restored.deadline;
	restored.Tick(now - 100000, reward);
	assert(restored.deadline < oldDeadline);
	assert(restored.Remaining(now - 100000) == restored.durationSeconds);
	return 0;
}
