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
	BreakWindow(BRect frame, Preferences* preferences, int32 completedBreaks,
		bool preview, bool blocking);
	bool QuitRequested() override;
	void MessageReceived(BMessage* message) override;
	void SetCountdown(const BString& value);

private:
	bool fPreview;
	int32 fPreviewSeconds;
	bigtime_t fPreviewDeadline;
	BStringView* fCountdown;
	std::unique_ptr<BMessageRunner> fRunner;
};
