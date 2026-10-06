#pragma once

#include "Preferences.h"

#include <MessageRunner.h>
#include <Window.h>

#include <memory>

class BButton;
class BStringView;
class CatView;

class BreakWindow : public BWindow {
public:
	BreakWindow(BRect frame, const Preferences& preferences, int32 completedBreaks,
		bool preview, bool blocking);
	bool QuitRequested() override;
	void MessageReceived(BMessage* message) override;

private:
	void _SendAction(uint32 what);

	bool fPreview;
	bool fActionSent;
	int32 fPreviewSeconds;
	bigtime_t fPreviewDeadline;
	BStringView* fCountdown;
	std::unique_ptr<BMessageRunner> fRunner;
};
