#include "Messages.h"

#include <Archivable.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <Roster.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include <algorithm>
#include <cstdio>

class FatCatDeskbarView : public BView {
public:
	FatCatDeskbarView(BRect frame)
		:
		BView(frame, kDeskbarItemName, B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED),
		fPhase(0), fPaused(false), fRemaining(0)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
	}

	FatCatDeskbarView(BMessage* archive)
		:
		BView(archive), fPhase(0), fPaused(false), fRemaining(0)
	{
	}

	static BArchivable* Instantiate(BMessage* archive)
	{
		return validate_instantiation(archive, "FatCatDeskbarView")
			? new FatCatDeskbarView(archive) : nullptr;
	}

	status_t Archive(BMessage* archive, bool deep = true) const override
	{
		status_t status = BView::Archive(archive, deep);
		if (status == B_OK)
			archive->AddString("add_on", "application/x-vnd.arkane-FatCatDeskbar");
		return status;
	}

	void AttachedToWindow() override
	{
		BView::AttachedToWindow();
		if (Window()) Window()->SetPulseRate(1000000);
		BMessenger app(kAppSignature);
		if (!app.IsValid()) {
			const char* arguments[] = { "fatcat.app", "--background" };
			be_roster->Launch(kAppSignature, 2, arguments);
		}
		_Query();
	}

	void Pulse() override { _Query(); }

	void Draw(BRect) override
	{
		SetHighUIColor(B_CONTROL_TEXT_COLOR);
		SetLowColor(ViewColor());
		BString text("🐈 ");
		if (fPhase == 0)
			text << "Pomodoro";
		else {
			if (fPaused) text << "Ⅱ ";
			char timer[16];
			snprintf(timer, sizeof(timer), "%02ld:%02ld", (long)(fRemaining / 60),
				(long)(fRemaining % 60));
			text << timer;
		}
		font_height height;
		GetFontHeight(&height);
		DrawString(text.String(), BPoint(4, (Bounds().Height() + height.ascent - height.descent) / 2));
	}

	void MouseDown(BPoint) override
	{
		uint32 buttons = B_PRIMARY_MOUSE_BUTTON;
		if (Window() && Window()->CurrentMessage())
			Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons);
		_Send(buttons & B_SECONDARY_MOUSE_BUTTON ? kMsgPreview : kMsgShow);
	}

	private:
	void _Send(uint32 what)
	{
		BMessage message(what);
		BMessenger app(kAppSignature);
		if (app.IsValid())
			app.SendMessage(&message);
		else {
			const char* arguments[] = { "fatcat.app",
				what == kMsgPreview ? "--preview" : "--show" };
			be_roster->Launch(kAppSignature, 2, arguments);
		}
	}

	void _Query()
	{
		BMessenger app(kAppSignature);
		if (!app.IsValid()) {
			fPhase = 0; fPaused = false; fRemaining = 0; Invalidate();
			return;
		}
		BMessage request(kMsgStatus), reply;
		if (app.SendMessage(&request, &reply, 50000, 50000) == B_OK) {
			reply.FindInt32("phase", &fPhase);
			reply.FindBool("paused", &fPaused);
			reply.FindInt32("remaining", &fRemaining);
			Invalidate();
		}
	}

	int32 fPhase;
	bool fPaused;
	int32 fRemaining;
};

extern "C" _EXPORT BView*
instantiate_deskbar_item(float maxWidth, float maxHeight)
{
	float width = std::min(112.0f, maxWidth);
	return new FatCatDeskbarView(BRect(0, 0, width, maxHeight));
}
