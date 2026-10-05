#include "MainWindow.h"

#include "Messages.h"

#include <Application.h>
#include <Button.h>
#include <CheckBox.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <Screen.h>
#include <SeparatorView.h>
#include <StringView.h>
#include <TabView.h>
#include <TextControl.h>

#include <cstdlib>
#include <cstdio>

static constexpr uint32 kSave = 'save';
static constexpr uint32 kSelectMonitor = 'smon';
static constexpr uint32 kRenameBase = 'rn00';
static constexpr uint32 kFavoriteBase = 'fv00';
static const int32 kUnlocks[] = { 0, 1, 3, 6 };
static const char* kPersonalities[] = {
	"Sleepy · expert napper", "Curious · gentle explorer",
	"Playful · little zoomies", "Shy · quiet company"
};

MainWindow::MainWindow(const Session& session, const Preferences& preferences)
	:
	BWindow(BRect(120, 100, 590, 720), "Fat Cat Pomodoro", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS),
	fSession(session),
	fPreferences(preferences),
	fStatus(nullptr),
	fProgress(nullptr),
	fSaveMessage(nullptr),
	fPrimary(nullptr),
	fStop(nullptr),
	fFocus(nullptr),
	fBreak(nullptr),
	fLongBreak(nullptr),
	fEvery(nullptr),
	fBlocking(nullptr),
	fMotion(nullptr),
	fMonitor(nullptr),
	fSettingsInitialized(false)
{
	for (int32 i = 0; i < 4; ++i) {
		fCatNameLabels[i] = nullptr;
		fCatNames[i] = nullptr;
		fFavoriteButtons[i] = nullptr;
	}
	BTabView* tabs = new BTabView("tabs", B_WIDTH_FROM_LABEL);
	tabs->AddTab(_BuildTimerTab());
	tabs->TabAt(0)->SetLabel("Timer");
	tabs->AddTab(_BuildCatsTab(session, preferences));
	tabs->TabAt(1)->SetLabel("Cats");
	BLayoutBuilder::Group<>(this, B_VERTICAL).SetInsets(12).Add(tabs);
	_UpdateControls();
}

BView*
MainWindow::_BuildTimerTab()
{
	BGroupView* view = new BGroupView(B_VERTICAL, 10);
	view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fStatus = new BStringView("status", "Ready when you are.");
	fStatus->SetFont(be_bold_font);
	fPrimary = new BButton("Start focus", new BMessage(kMsgStart));
	fPrimary->SetTarget(this);
	fStop = new BButton("Stop", new BMessage(kMsgStop));
	fStop->SetTarget(this);
	BButton* preview = new BButton("Preview cats", new BMessage(kMsgPreview));
	preview->SetTarget(this);
	BButton* about = new BButton("About…", new BMessage(B_ABOUT_REQUESTED));
	about->SetTarget(this);

	fFocus = new BTextControl("Focus:", "", nullptr);
	fBreak = new BTextControl("Short break:", "", nullptr);
	fLongBreak = new BTextControl("Long break:", "", nullptr);
	fEvery = new BTextControl("Long break every:", "", nullptr);
	for (BTextControl* field : { fFocus, fBreak, fLongBreak, fEvery })
		field->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	fBlocking = new BCheckBox("Blocking break covers your work", nullptr);
	fMotion = new BCheckBox("Reduced motion", nullptr);

	BMenu* screens = new BMenu("All screens");
	BMessage* allMessage = new BMessage(kSelectMonitor);
	allMessage->AddString("monitor", "");
	screens->AddItem(new BMenuItem("All screens", allMessage));
	BScreen screen;
	int32 index = 1;
	do {
		BString label("Screen ");
		label << index++;
		BMessage* message = new BMessage(kSelectMonitor);
		message->AddString("monitor", label);
		screens->AddItem(new BMenuItem(label.String(), message));
	} while (screen.SetToNext() == B_OK);
	screens->SetTargetForItems(this);
	fMonitor = new BMenuField("Show cats on:", screens);
	fSaveMessage = new BStringView("save result", "");
	BButton* save = new BButton("Save settings", new BMessage(kSave));

	BLayoutBuilder::Group<>(view, B_VERTICAL, 10)
		.SetInsets(12)
		.Add(fStatus)
		.AddGroup(B_HORIZONTAL, 8)
			.Add(fPrimary).Add(fStop).Add(preview).AddGlue().Add(about)
		.End()
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGrid(8, 8)
			.Add(fFocus->CreateLabelLayoutItem(), 0, 0)
			.Add(fFocus->CreateTextViewLayoutItem(), 1, 0)
			.Add(new BStringView(nullptr, "minutes"), 2, 0)
			.Add(fBreak->CreateLabelLayoutItem(), 0, 1)
			.Add(fBreak->CreateTextViewLayoutItem(), 1, 1)
			.Add(new BStringView(nullptr, "minutes"), 2, 1)
			.Add(fLongBreak->CreateLabelLayoutItem(), 0, 2)
			.Add(fLongBreak->CreateTextViewLayoutItem(), 1, 2)
			.Add(new BStringView(nullptr, "minutes"), 2, 2)
			.Add(fEvery->CreateLabelLayoutItem(), 0, 3)
			.Add(fEvery->CreateTextViewLayoutItem(), 1, 3)
			.Add(new BStringView(nullptr, "sets"), 2, 3)
		.End()
		.Add(new BStringView(nullptr,
			"Intervals: 1–180 minutes. Long break every 2–12 focus sessions."))
		.Add(fBlocking)
		.Add(fMotion)
		.Add(fMonitor)
		.AddGroup(B_HORIZONTAL, 8).Add(save).Add(fSaveMessage).AddGlue().End()
		.AddGlue();
	return view;
}
//---------------------------------------------------------------------------------------------------------------------------------//


BView*
MainWindow::_BuildCatsTab(const Session&, const Preferences&)
{
	BGroupView* view = new BGroupView(B_VERTICAL, 8);
	view->SetViewUIColor(B_PANEL_BACKGROUND_COLOR);
	fProgress = new BStringView("progress", "");

	BLayoutBuilder::Group<> layout(view, B_VERTICAL, 8);
	layout.SetInsets(12)
		.Add(new BStringView(nullptr, "Your little sanctuary"))
		.Add(fProgress);
	for (int32 i = 0; i < 4; ++i) {
		fCatNameLabels[i] = new BStringView(nullptr, "");
		fCatNameLabels[i]->SetFont(be_bold_font);
		fCatNames[i] = new BTextControl("Name:", "", new BMessage(kRenameBase + i));
		fCatNames[i]->SetModificationMessage(new BMessage(kRenameBase + i));
		fCatNames[i]->SetTarget(this);
		fFavoriteButtons[i] = new BButton("☆ Favorite", new BMessage(kFavoriteBase + i));
		layout.Add(new BSeparatorView(B_HORIZONTAL))
			.Add(fCatNameLabels[i])
			.Add(new BStringView(nullptr, kPersonalities[i]))
			.AddGroup(B_HORIZONTAL, 8)
				.Add(fCatNames[i]).Add(fFavoriteButtons[i]).AddGlue()
			.End();
	}
	layout.Add(new BStringView(nullptr,
		"Favorites visit during breaks. Leave favorites empty for any unlocked cat."))
		.AddGlue();
	return view;
}

bool
MainWindow::QuitRequested()
{
	fSettingsInitialized = false;
	Hide();
	return false;
}

void
MainWindow::_SendSettings()
{
	auto parse = [](const char* text, long minimum, long maximum, long& value) {
		char* end = nullptr;
		value = strtol(text, &end, 10);
		return end != text && *end == '\0' && value >= minimum && value <= maximum;
	};
	long focus, rest, longRest, every;
	if (parse(fFocus->Text(), 1, 180, focus)
		&& parse(fBreak->Text(), 1, 180, rest)
		&& parse(fLongBreak->Text(), 1, 180, longRest)
		&& parse(fEvery->Text(), 2, 12, every)) {
		BMessage save(kMsgSaveSettings);
		save.AddInt32("focus", focus);
		save.AddInt32("break", rest);
		save.AddInt32("long_break", longRest);
		save.AddInt32("long_every", every);
		save.AddBool("blocking", fBlocking->Value() == B_CONTROL_ON);
		save.AddBool("reduced_motion", fMotion->Value() == B_CONTROL_ON);
		save.AddString("monitor", fPreferences.selectedMonitor);
		be_app_messenger.SendMessage(&save);
		fSaveMessage->SetText("Saved · applies to the next interval.");
		return;
	}
	fSaveMessage->SetText("Enter valid intervals before saving.");
}

void
MainWindow::MessageReceived(BMessage* message)
{
	if (message->what == kMsgStart || message->what == kMsgPauseResume
		|| message->what == kMsgStop || message->what == kMsgPreview
		|| message->what == B_ABOUT_REQUESTED) {
		be_app_messenger.SendMessage(message);
		return;
	}
	if (message->what == kSave) { _SendSettings(); return; }
	if (message->what == kSelectMonitor) {
		const char* monitor;
		if (message->FindString("monitor", &monitor) == B_OK)
			fPreferences.selectedMonitor = monitor;
		return;
	}
	if (message->what >= kRenameBase && message->what < kRenameBase + 4) {
		int32 id = message->what - kRenameBase;
		BMessage rename(kMsgRenameCat);
		rename.AddInt32("id", id);
		rename.AddString("name", fCatNames[id]->Text());
		be_app_messenger.SendMessage(&rename);
		return;
	}
	if (message->what >= kFavoriteBase && message->what < kFavoriteBase + 4) {
		BMessage favorite(kMsgToggleFavorite);
		favorite.AddInt32("id", message->what - kFavoriteBase);
		be_app_messenger.SendMessage(&favorite);
		return;
	}
	BWindow::MessageReceived(message);
}

void
MainWindow::Update(const Session& session, const Preferences& preferences,
	const BString& persistenceError)
{
	fSession = session;
	fPreferences = preferences;
	fError = persistenceError;
	_UpdateControls();
}

void
MainWindow::_UpdateControls()
{
	time_t now = time(nullptr);
	BString status;
	if (fSession.phase == Phase::Idle)
		status = "Ready when you are.";
	else {
		if (fSession.paused) status << "Paused · ";
		else status << (fSession.phase == Phase::Focus ? "Focus · " : "Break · ");
		status << fSession.Countdown(now);
	}
	if (!fError.IsEmpty()) status << "  ⚠ " << fError;
	fStatus->SetText(status);
	fPrimary->SetLabel(fSession.phase == Phase::Idle ? "Start focus"
		: fSession.paused ? "Resume" : "Pause");
	fPrimary->SetMessage(new BMessage(fSession.phase == Phase::Idle ? kMsgStart : kMsgPauseResume));
	fPrimary->SetTarget(this);
	fStop->SetEnabled(fSession.phase != Phase::Idle);

	if (!fSettingsInitialized) {
		char number[16];
		snprintf(number, sizeof(number), "%ld", (long)fSession.focusMinutes);
		fFocus->SetText(number);
		snprintf(number, sizeof(number), "%ld", (long)fSession.breakMinutes);
		fBreak->SetText(number);
		snprintf(number, sizeof(number), "%ld", (long)fSession.longBreakMinutes);
		fLongBreak->SetText(number);
		snprintf(number, sizeof(number), "%ld", (long)fSession.longBreakEvery);
		fEvery->SetText(number);
		fBlocking->SetValue(fPreferences.blockingBreak);
		fMotion->SetValue(fPreferences.reducedMotion);
		BString label = fPreferences.selectedMonitor.IsEmpty()
			? "All screens" : fPreferences.selectedMonitor;
		if (BMenuItem* item = fMonitor->Menu()->FindItem(label.String()))
			item->SetMarked(true);
		fSettingsInitialized = true;
	}

	BString progress;
	progress << fSession.completedBreaks
		<< " completed breaks. New friends arrive as you rest. No streaks to lose.";
	fProgress->SetText(progress);
	for (int32 i = 0; i < 4; ++i) {
		bool unlocked = fSession.completedBreaks >= kUnlocks[i];
		BString labelText = fPreferences.catNames[i];
		if (!unlocked) labelText << " — arrives after " << kUnlocks[i] << " completed breaks";
		fCatNameLabels[i]->SetText(labelText);
		if (!fCatNames[i]->TextView()->IsFocus())
			fCatNames[i]->SetText(fPreferences.catNames[i]);
		fCatNames[i]->SetEnabled(unlocked);
		fFavoriteButtons[i]->SetEnabled(unlocked);
		fFavoriteButtons[i]->SetLabel(fPreferences.IsFavorite(i) ? "★ Favorite" : "☆ Favorite");
		fFavoriteButtons[i]->SetTarget(this);
	}
}
