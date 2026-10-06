#include "BreakWindow.h"

#include "CatView.h"
#include "Debug.h"
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

BreakWindow::BreakWindow(BRect frame, const Preferences& preferences, int32 completedBreaks,
	bool preview, bool blocking)
	:
	BWindow(frame, "Fat Cat break", B_NO_BORDER_WINDOW_LOOK,
		blocking ? B_MODAL_APP_WINDOW_FEEL : B_FLOATING_APP_WINDOW_FEEL,
		B_WILL_ACCEPT_FIRST_CLICK | B_NOT_CLOSABLE | B_NOT_ZOOMABLE
			| B_NOT_MINIMIZABLE | B_NOT_MOVABLE | B_NOT_RESIZABLE),
	fPreview(preview),
	fActionSent(false),
	fPreviewSeconds(15),
	fPreviewDeadline(system_time() + 15000000),
	fCountdown(nullptr)
{
	int32 workspace = current_workspace();
	FatCatDebug("BreakWindow ctor this=%p preview=%d blocking=%d frame=(%.0f,%.0f,%.0f,%.0f) workspace=%d",
		(void*)this, (int)preview, (int)blocking,
		frame.left, frame.top, frame.right, frame.bottom, (int)workspace);
	if (workspace >= 0 && workspace < 32)
		SetWorkspaces(uint32(1) << workspace);
	AddShortcut(B_ESCAPE, 0, new BMessage(preview ? kMsgDismiss : kMsgSkipBreak), this);

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
	panel->MoveTo(std::max(12.0f, Bounds().Width() - panel->Bounds().Width() - 12),
		48);
	cats->AddChild(panel);

	if (preview) {
		BMessage tick(kPreviewTick);
		fRunner = std::make_unique<BMessageRunner>(BMessenger(this), &tick, 200000);
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


void
BreakWindow::_SendAction(uint32 what)
{
	FatCatDebug("BreakWindow action what=0x%x already=%d",
		(unsigned)what, (int)fActionSent);
	if (fActionSent)
		return;
	fActionSent = true;
	Hide();
	BMessage message(what);
	be_app->PostMessage(&message);
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool
BreakWindow::QuitRequested()
{
	FatCatDebug("BreakWindow QuitRequested");
	return true;
}
//---------------------------------------------------------------------------------------------------------------------------------//


void
BreakWindow::MessageReceived(BMessage* message)
{
	if (message->what == B_WINDOW_ACTIVATED)
		FatCatDebug("BreakWindow: WINDOW_ACTIVATED");
	if (message->what == B_WORKSPACE_ACTIVATED)
		FatCatDebug("BreakWindow: WORKSPACE_ACTIVATED");
	if (message->what == B_MOUSE_DOWN)
		FatCatDebug("BreakWindow: MOUSE_DOWN");
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
