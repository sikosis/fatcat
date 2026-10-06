#pragma once

#include "Preferences.h"

#include <Bitmap.h>
#include <MessageRunner.h>
#include <View.h>

#include <memory>
#include <vector>

class CatView : public BView {
public:
	CatView(const Preferences& preferences, int32 completedBreaks, bool preview,
		bool reducedMotion, BBitmap* backdrop);
	~CatView() override;
	void AttachedToWindow() override;
	void Draw(BRect update) override;
	void FrameResized(float width, float height) override;
	void MessageReceived(BMessage* message) override;

private:
	struct Cat {
		int32 variant;
		float x;
		float lane;
		float direction;
		float speed;
		int32 activity;
		float remaining;
		float greetingCooldown;
		float hop;
		int32 frame;
		float frameElapsed;
	};

	void _Reset();
	void _Advance(float seconds);
	void _ChooseActivity(Cat& cat);
	BRect _CatFrame(const Cat& cat) const;
	float _CatSize() const;

	Preferences fPreferences;
	int32 fCompletedBreaks;
	bool fPreview;
	bool fReducedMotion;
	std::vector<Cat> fCats;
	std::unique_ptr<BBitmap> fSheets[4];
	std::unique_ptr<BBitmap> fBackdrop;
	std::unique_ptr<BMessageRunner> fRunner;
	bigtime_t fLastTick = 0;
	int fDrawLogCount = 0;
};
