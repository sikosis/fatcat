#include "DeskbarView.h"
#include "Debug.h"
#include "Messages.h"

#include <Archivable.h>
#include <Bitmap.h>
#include <IconUtils.h>
#include <InterfaceDefs.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Resources.h>
#include <Roster.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include <algorithm>
#include <cstdio>

static const int32 kResourceAnchor = 0;
static constexpr uint32 kDeskbarPoll = 'fcdp';

class FatCatDeskbarView : public BView {
public:
	FatCatDeskbarView(BRect frame)
		:
		BView(frame, kDeskbarItemName, B_FOLLOW_NONE, B_WILL_DRAW),
		fIcon(nullptr), fRunner(nullptr), fRunning(false), fWaitingForReply(false),
		fWaitTicks(0), fPhase(0), fPaused(false), fRemaining(0)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		_LoadIcon();
	}

	FatCatDeskbarView(BMessage* archive)
		:
		BView(archive), fIcon(nullptr), fRunner(nullptr), fRunning(false),
		fWaitingForReply(false), fWaitTicks(0), fPhase(0), fPaused(false),
		fRemaining(0)
	{
		_LoadIcon();
	}

	~FatCatDeskbarView() override
	{
		delete fRunner;
		delete fIcon;
	}

	static BArchivable* Instantiate(BMessage* archive);

	status_t Archive(BMessage* archive, bool deep = true) const override
	{
		status_t status = BView::Archive(archive, deep);
		if (status == B_OK) {
			archive->AddString("class", "FatCatDeskbarView");
			// Match ClipDesk's proven pattern: the running application owns the
			// replicant class and is also its archive add-on.
			archive->AddString("add_on", kAppSignature);
		}
		return status;
	}

	void AttachedToWindow() override
	{
		BView::AttachedToWindow();
		BMessenger app(kAppSignature);
		if (!app.IsValid()) {
			const char* arguments[] = { "--background" };
			be_roster->Launch(kAppSignature, 1, arguments);
		}
		_Query();
		BMessage poll(kDeskbarPoll);
		fRunner = new BMessageRunner(BMessenger(this), &poll, 1000000);
	}

	void DetachedFromWindow() override
	{
		delete fRunner;
		fRunner = nullptr;
		BView::DetachedFromWindow();
	}

	void MessageReceived(BMessage* message) override
	{
		switch (message->what) {
			case kDeskbarPoll:
				_Query();
				break;
			case kMsgStatusReply:
				fWaitingForReply = false;
				fWaitTicks = 0;
				fRunning = true;
				message->FindInt32("phase", &fPhase);
				message->FindBool("paused", &fPaused);
				message->FindInt32("remaining", &fRemaining);
				Invalidate();
				break;
			default:
				BView::MessageReceived(message);
		}
	}

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
	FatCatDebug("DeskbarView MouseDown buttons=0x%x -> %s",
		(unsigned)buttons,
		(buttons & B_SECONDARY_MOUSE_BUTTON) ? "preview" : "show");
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
		if (app.IsValid()) {
			app.SendMessage(&message, static_cast<BHandler*>(nullptr), 100000);
		} else {
			const char* arguments[] = {
				what == kMsgPreview ? "--preview" : "--show" };
			be_roster->Launch(kAppSignature, 1, arguments);
		}
	}

	void _Query()
	{
		if (fWaitingForReply) {
			if (++fWaitTicks >= 3) {
				fWaitingForReply = false;
				fWaitTicks = 0;
				_SetUnavailable();
			}
			return;
		}

		BMessenger app(kAppSignature);
		if (!app.IsValid()) {
			_SetUnavailable();
			return;
		}
		BMessage request(kMsgStatus);
		if (app.SendMessage(&request, this, 100000) == B_OK) {
			fWaitingForReply = true;
			fWaitTicks = 0;
		} else
			_SetUnavailable();
	}

	void _SetUnavailable()
	{
		if (!fRunning)
			return;
		fRunning = false;
		fPhase = 0;
		fPaused = false;
		fRemaining = 0;
		Invalidate();
	}

	BBitmap* fIcon;
	BMessageRunner* fRunner;
	bool fRunning;
	bool fWaitingForReply;
	int32 fWaitTicks;
	int32 fPhase;
	bool fPaused;
	int32 fRemaining;
};

BArchivable*
FatCatDeskbarView::Instantiate(BMessage* archive)
{
	return validate_instantiation(archive, "FatCatDeskbarView")
		? new FatCatDeskbarView(archive) : nullptr;
}
//---------------------------------------------------------------------------------------------------------------------------------//


BView*
CreateFatCatDeskbarView(BRect frame)
{
	return new FatCatDeskbarView(frame);
}
//---------------------------------------------------------------------------------------------------------------------------------//


extern "C" _EXPORT BView*
instantiate_deskbar_item(float maxWidth, float maxHeight)
{
	float width = std::min(112.0f, maxWidth);
	return CreateFatCatDeskbarView(BRect(0, 0, width, maxHeight));
}
//---------------------------------------------------------------------------------------------------------------------------------//
