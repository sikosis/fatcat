#include "BreakWindow.h"

#include "CatView.h"
#include "Messages.h"

#include <Application.h>
#include <Button.h>
#include <Catalog.h>
#include <GroupView.h>
#include <InterfaceDefs.h>
#include <LayoutBuilder.h>
#include <Screen.h>
#include <StringView.h>

#include <algorithm>
#include <cstdio>

static constexpr uint32 kPreviewTick = 'pvtk';

static uint32 CurrentWorkspaceMask() {
	int32 workspace = current_workspace();
	if (workspace >= 0 && workspace < 32)
		return uint32(1) << workspace;
	return B_ALL_WORKSPACES;
}
//---------------------------------------------------------------------------------------------------------------------------------//

BreakWindow::BreakWindow(BRect frame, const Preferences& preferences, int32 completedBreaks,
	bool preview, bool blocking)
	:
	BWindow(frame, "Fat Cat break", B_NO_BORDER_WINDOW_LOOK,
		blocking ? B_MODAL_APP_WINDOW_FEEL : B_FLOATING_ALL_WINDOW_FEEL,
		B_WILL_ACCEPT_FIRST_CLICK | B_NOT_CLOSABLE | B_NOT_ZOOMABLE
			| B_NOT_MINIMIZABLE | B_NOT_MOVABLE | B_NOT_RESIZABLE,
		CurrentWorkspaceMask()),
	fPreview(preview),
	fActionSent(false),
	fPreviewSeconds(15),
	fPreviewDeadline(system_time() + 15000000),
	fCountdown(nullptr)
{
	AddShortcut(B_ESCAPE, B_NO_COMMAND_KEY,
		new BMessage(preview ? kMsgDismiss : kMsgSkipBreak), this);

	BBitmap* backdrop = nullptr;
	BScreen screen(this);
	if (screen.GetBitmap(&backdrop, false) != B_OK) {
		delete backdrop;
		backdrop = nullptr;
	}
	CatView* cats = new CatView(preferences, completedBreaks, preview,
		preferences.reducedMotion, backdrop);
	cats->ResizeTo(Bounds().Width(), Bounds().Height());
	cats->SetResizingMode(B_FOLLOW_ALL);
	AddChild(cats);

	BGroupView* panel = new BGroupView(B_VERTICAL, 8);
	panel->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);

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
	close->SetTarget(this);
	BButton* pause = new BButton("Pause timer", new BMessage(kMsgPauseResume));
	pause->SetTarget(this);
	if (preview)
		pause->Hide();

	BLayoutBuilder::Group<>(panel, B_VERTICAL, 8)
		.SetInsets(14)
		.Add(title)
		.Add(fCountdown)
		.Add(prompt)
		.AddGroup(B_HORIZONTAL, 8)
			.AddGlue()
			.Add(close)
			.Add(pause)
			.AddGlue()
		.End()
		.Add(new BStringView("hint", preview
			? "Preview closes automatically · No progress is earned"
			: "Esc to skip · Your next focus session starts after this break"));
	panel->ResizeTo(std::min(430.0f, Bounds().Width() - 24), 205);
	panel->MoveTo((Bounds().Width() - panel->Bounds().Width()) / 2, 48);
	cats->AddChild(panel);

	if (preview) {
		BMessage tick(kPreviewTick);
		fRunner = std::make_unique<BMessageRunner>(BMessenger(this), &tick, 200000);
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


void BreakWindow::_SendAction(uint32 what) {
	if (fActionSent)
		return;
	fActionSent = true;
	Hide();
	BMessage message(what);
	be_app->PostMessage(&message);
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool BreakWindow::QuitRequested() {
	return true;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void BreakWindow::MessageReceived(BMessage* message) {
	if (message->what == kMsgBreakCountdown) {
		const char* value;
		if (!fPreview && message->FindString("countdown", &value) == B_OK)
			fCountdown->SetText(value);
		return;
	}
	if (message->what == kMsgBreakClose) {
		Quit();
		return;
	}
	if (message->what == kMsgDismiss || message->what == kMsgSkipBreak
		|| message->what == kMsgPauseResume) {
		_SendAction(message->what);
		return;
	}
	if (message->what == kPreviewTick) {
		fPreviewSeconds = std::max(0, (int32)((fPreviewDeadline - system_time()
			+ 999999) / 1000000));
		char text[16];
		snprintf(text, sizeof(text), "00:%02ld", (long)fPreviewSeconds);
		fCountdown->SetText(text);
		if (fPreviewSeconds == 0)
			_SendAction(kMsgDismiss);
		return;
	}
	BWindow::MessageReceived(message);
}
//---------------------------------------------------------------------------------------------------------------------------------//
