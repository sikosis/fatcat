#include "BreakWindow.h"

#include "CatView.h"
#include "Messages.h"

#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <LayoutBuilder.h>
#include <StringView.h>

#include <algorithm>
#include <cstdio>

static constexpr uint32 kPreviewTick = 'pvtk';

BreakWindow::BreakWindow(BRect frame, Preferences* preferences, int32 completedBreaks,
	bool preview, bool blocking)
	:
	BWindow(blocking ? frame : BRect(0, 0, 620, 470), "Fat Cat break",
		blocking ? B_NO_BORDER_WINDOW : B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS
			| (blocking ? B_NOT_CLOSABLE | B_NOT_ZOOMABLE | B_NOT_MINIMIZABLE : 0)),
	fPreview(preview),
	fPreviewSeconds(15),
	fPreviewDeadline(system_time() + 15000000),
	fCountdown(nullptr)
{
	if (!blocking)
		MoveTo(frame.left + (frame.Width() - Bounds().Width()) / 2,
			frame.top + (frame.Height() - Bounds().Height()) / 2);
	SetFeel(blocking ? B_MODAL_APP_WINDOW_FEEL : B_FLOATING_APP_WINDOW_FEEL);
	SetWorkspaces(B_ALL_WORKSPACES);
	AddShortcut(B_ESCAPE, 0, new BMessage(preview ? kMsgDismiss : kMsgSkipBreak), be_app);

	CatView* cats = new CatView(preferences, completedBreaks, preview,
		preferences->reducedMotion);
	cats->SetExplicitMinSize(BSize(420, 230));

	BStringView* title = new BStringView("title", preview ? "MEET YOUR CATS" : "CAT BREAK");
	title->SetAlignment(B_ALIGN_CENTER);
	title->SetFont(be_bold_font);
	fCountdown = new BStringView("countdown", preview ? "00:15" : "");
	fCountdown->SetAlignment(B_ALIGN_CENTER);
	BFont large(be_bold_font);
	large.SetSize(34);
	fCountdown->SetFont(&large);
	BStringView* prompt = new BStringView("prompt", preview
		? "A short visit. Your timer keeps its place."
		: "Stretch your legs. The cats are stretching theirs.");
	prompt->SetAlignment(B_ALIGN_CENTER);
	BButton* close = new BButton(preview ? "Close preview" : "Skip break",
		new BMessage(preview ? kMsgDismiss : kMsgSkipBreak));
	close->SetTarget(be_app);
	BButton* pause = new BButton("Pause timer", new BMessage(kMsgPauseResume));
	pause->SetTarget(be_app);
	if (preview)
		pause->Hide();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 10)
		.SetInsets(18)
		.Add(title)
		.Add(fCountdown)
		.Add(prompt)
		.Add(cats, 1)
		.AddGroup(B_HORIZONTAL, 8)
			.AddGlue()
			.Add(close)
			.Add(pause)
			.AddGlue()
		.End()
		.Add(new BStringView("hint", preview
			? "Preview closes automatically · No progress is earned"
			: "Esc to skip · Your next focus session starts after this break"));

	if (preview) {
		BMessage tick(kPreviewTick);
		fRunner = std::make_unique<BMessageRunner>(BMessenger(this), &tick, 200000);
	}
}

bool
BreakWindow::QuitRequested()
{
	BMessage message(fPreview ? kMsgDismiss : kMsgSkipBreak);
	be_app_messenger.SendMessage(&message);
	return false;
}

void
BreakWindow::MessageReceived(BMessage* message)
{
	if (message->what == kPreviewTick) {
		fPreviewSeconds = std::max(0, (int32)((fPreviewDeadline - system_time()
			+ 999999) / 1000000));
		char text[16];
		snprintf(text, sizeof(text), "00:%02ld", (long)fPreviewSeconds);
		fCountdown->SetText(text);
		if (fPreviewSeconds == 0) {
			BMessage dismiss(kMsgDismiss);
			be_app_messenger.SendMessage(&dismiss);
		}
		return;
	}
	BWindow::MessageReceived(message);
}

void
BreakWindow::SetCountdown(const BString& value)
{
	if (!fPreview && Lock()) {
		fCountdown->SetText(value.String());
		Unlock();
	}
}
