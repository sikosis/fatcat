#include "Messages.h"

#include <Archivable.h>
#include <Bitmap.h>
#include <IconUtils.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <Resources.h>
#include <Roster.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include <algorithm>
#include <cstdio>

static const int32 kResourceAnchor = 0;

class FatCatDeskbarView : public BView {
public:
	FatCatDeskbarView(BRect frame)
		:
		BView(frame, kDeskbarItemName, B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED),
		fIcon(nullptr), fRunning(false), fPhase(0), fPaused(false), fRemaining(0)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		_LoadIcon();
	}

	FatCatDeskbarView(BMessage* archive)
		:
		BView(archive), fIcon(nullptr), fRunning(false), fPhase(0), fPaused(false),
		fRemaining(0)
	{
		_LoadIcon();
	}

	~FatCatDeskbarView() override { delete fIcon; }

	static BArchivable* Instantiate(BMessage* archive)
	{
		return validate_instantiation(archive, "FatCatDeskbarView")
			? new FatCatDeskbarView(archive) : nullptr;
	}

	status_t Archive(BMessage* archive, bool deep = true) const override
	{
		status_t status = BView::Archive(archive, deep);
		if (status == B_OK) {
			archive->AddString("class", "FatCatDeskbarView");
			archive->AddString("add_on", kDeskbarSignature);
		}
		return status;
	}

	void AttachedToWindow() override
	{
		BView::AttachedToWindow();
		if (Window()) Window()->SetPulseRate(1000000);
		BMessenger app(kAppSignature);
		if (!app.IsValid()) {
			const char* arguments[] = { "--background" };
			be_roster->Launch(kAppSignature, 1, arguments);
		}
		_Query();
	}

	void Pulse() override { _Query(); }

	void Draw(BRect) override
	{
		if (fIcon != nullptr) {
			SetDrawingMode(B_OP_ALPHA);
			SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
			DrawBitmap(fIcon, BPoint(2, std::max(0.0f,
				(Bounds().Height() - 16.0f) / 2.0f)));
			SetDrawingMode(B_OP_COPY);
		}
		SetHighColor(fRunning ? rgb_color { 52, 199, 89, 255 }
			: rgb_color { 130, 130, 130, 255 });
		FillEllipse(BPoint(16, Bounds().Height() - 4), 2, 2);

		SetHighUIColor(B_CONTROL_TEXT_COLOR);
		SetLowColor(ViewColor());
		BString text;
		if (!fRunning)
			text = "Off";
		else if (fPhase == 0)
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
		DrawString(text.String(), BPoint(23,
			(Bounds().Height() + height.ascent - height.descent) / 2));
	}

	void MouseDown(BPoint) override
	{
		uint32 buttons = B_PRIMARY_MOUSE_BUTTON;
		if (Window() && Window()->CurrentMessage())
			Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons);
		_Send(buttons & B_SECONDARY_MOUSE_BUTTON ? kMsgPreview : kMsgShow);
	}

	private:
	void _LoadIcon()
	{
		BResources resources;
		if (resources.SetToImage(&kResourceAnchor) != B_OK)
			return;
		size_t size = 0;
		const uint8* data = static_cast<const uint8*>(
			resources.LoadResource('VICN', 101, &size));
		if (data == nullptr || size == 0)
			return;

		BBitmap* icon = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32);
		if (icon->InitCheck() == B_OK
			&& BIconUtils::GetVectorIcon(data, size, icon) == B_OK) {
			fIcon = icon;
		} else
			delete icon;
	}

	void _Send(uint32 what)
	{
		BMessage message(what);
		BMessenger app(kAppSignature);
		if (app.IsValid())
			app.SendMessage(&message);
		else {
			const char* arguments[] = {
				what == kMsgPreview ? "--preview" : "--show" };
			be_roster->Launch(kAppSignature, 1, arguments);
		}
	}

	void _Query()
	{
		BMessenger app(kAppSignature);
		if (!app.IsValid()) {
			fRunning = false;
			fPhase = 0; fPaused = false; fRemaining = 0; Invalidate();
			return;
		}
		BMessage request(kMsgStatus), reply;
		if (app.SendMessage(&request, &reply, 50000, 50000) == B_OK) {
			fRunning = true;
			reply.FindInt32("phase", &fPhase);
			reply.FindBool("paused", &fPaused);
			reply.FindInt32("remaining", &fRemaining);
			Invalidate();
		} else if (fRunning) {
			fRunning = false;
			Invalidate();
		}
	}

	BBitmap* fIcon;
	bool fRunning;
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
