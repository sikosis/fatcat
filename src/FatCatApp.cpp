#include "FatCatApp.h"

#include "BreakWindow.h"
#include "Debug.h"
#include "DeskbarView.h"
#include "MainWindow.h"
#include "Messages.h"

#include <Alert.h>
#include <Bitmap.h>
#include <Deskbar.h>
#include <IconUtils.h>
#include <InterfaceDefs.h>
#include <OS.h>
#include <Resources.h>
#include <Roster.h>
#include <Screen.h>

#include <algorithm>
#include <ctime>
#include <cstdio>
#include <cstring>

static constexpr int32 kMaxScreens = 32;

FatCatApp::FatCatApp()
	:
	BApplication(kAppSignature) {
}
//---------------------------------------------------------------------------------------------------------------------------------//


void FatCatApp::ReadyToRun() {
	_Load();
	_EnsureDeskbarItem();
	fMainWindow = new MainWindow(fSession, fPreferences);
	fMainWindow->CenterOnScreen();
	// BWindow only starts its looper on the first Show(), so always call it.
	// For a command-line-only launch Hide() first: it bumps fShowLevel above
	// zero, so the follow-up Show() runs the looper without exposing the
	// settings window for a frame.
	if (fCommandLineOnly) {
		fMainWindow->Hide();
		fMainWindow->Show();
	} else {
		fMainWindow->Show();
	}
	FatCatDebug("ReadyToRun: mainHidden=%d", (int)fMainWindow->IsHidden());
	FatCatDebug("ReadyToRun: cli=%d phase=%d", (int)fCommandLineOnly,
		(int)fSession.phase);
	BMessage tick(kMsgTick);
	fTicker = std::make_unique<BMessageRunner>(BMessenger(this), &tick, 1000000);
	if (fSession.phase == Phase::Break && !fSession.paused)
		_ShowOverlay(false);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void FatCatApp::ArgvReceived(int32 argc, char** argv) {
	if (argc < 2) {
		PostMessage(kMsgShow);
		return;
	}
	fCommandLineOnly = true;
	BString command(argv[1]);
	if (command == "--start") PostMessage(kMsgStart);
	else if (command == "--pause") PostMessage(kMsgPause);
	else if (command == "--resume") PostMessage(kMsgResume);
	else if (command == "--stop") PostMessage(kMsgStop);
	else if (command == "--preview") PostMessage(kMsgPreview);
	else if (command == "--dismiss") PostMessage(kMsgDismiss);
	else if (command == "--show") { fCommandLineOnly = false; PostMessage(kMsgShow); }
	else if (command == "--status") {
		printf("Fat Cat is running. Use the Deskbar item for live status.\n");
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


void FatCatApp::_EnsureDeskbarItem() {
	BDeskbar deskbar;
	if (!deskbar.IsRunning()) {
		fPersistenceError = "Deskbar is not running.";
		return;
	}
	if (deskbar.HasItem(kDeskbarItemName))
		return;

	BView* view = CreateFatCatDeskbarView(BRect(0, 0, 112, 15));
	status_t status = deskbar.AddItem(view);
	delete view;
	if (status == B_OK) {
		if (fPersistenceError == "Deskbar is not running."
			|| fPersistenceError.StartsWith("Deskbar item could not be installed:"))
			fPersistenceError = "";
		return;
	}

	fPersistenceError = "Deskbar item could not be installed: ";
	fPersistenceError << strerror(status);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void FatCatApp::_RemoveDeskbarItem() {
	BDeskbar deskbar;
	if (deskbar.IsRunning())
		deskbar.RemoveItem(kDeskbarItemName);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void FatCatApp::AboutRequested() {
	BString aboutText("Fat Cat Pomodoro\n\nVersion: ");
	aboutText << kAppVersion << "\n";
	aboutText << kAppDescription
		<< "\n\nDesigned by Sikosis\n\n"
			"Original Fat Cat concept and Sprites ©2026 arkane\n\n"
			"Released under the MIT License";
	BAlert* about = new BAlert("About Fat Cat",
		aboutText.String(),
		"Purrfect", nullptr, nullptr, B_WIDTH_AS_USUAL, B_INFO_ALERT);

	BResources* resources = AppResources();
	if (resources != nullptr) {
		size_t size = 0;
		const uint8* data = static_cast<const uint8*>(
			resources->LoadResource('VICN', 101, &size));
		if (data != nullptr && size > 0) {
			BBitmap* icon = new BBitmap(BRect(0, 0, 63, 63), B_RGBA32);
			if (icon->InitCheck() == B_OK
				&& BIconUtils::GetVectorIcon(data, size, icon) == B_OK) {
				about->SetIcon(icon);
			} else
				delete icon;
		}
	}
	// Keep the timer service responsive while the About window is open.
	about->Go(static_cast<BInvoker*>(nullptr));
}
//---------------------------------------------------------------------------------------------------------------------------------//


void FatCatApp::_Load() {
	BMessage state;
	if (LoadFlatMessage("session", state) == B_OK)
		fSession.Restore(state, time(nullptr));
	BMessage preferences;
	if (LoadFlatMessage("preferences", preferences) == B_OK)
		fPreferences.Restore(preferences);
	_SaveSession(); // repairs a missing/corrupt/overdue snapshot immediately
}

void
FatCatApp::_SaveSession()
{
	BMessage message;
	fSession.Archive(message);
	status_t status = SaveFlatMessage("session", message);
	fPersistenceError = status == B_OK ? "" : "Session could not be saved.";
}
//---------------------------------------------------------------------------------------------------------------------------------//

void FatCatApp::_SavePreferences() {
	BMessage message;
	fPreferences.Archive(message);
	status_t status = SaveFlatMessage("preferences", message);
	if (status != B_OK)
		fPersistenceError = "Preferences could not be saved.";
}
//---------------------------------------------------------------------------------------------------------------------------------//


void FatCatApp::_StateChanged() {
	_SaveSession();
	if (fMainWindow)
		fMainWindow->PostUpdate(fSession, fPreferences, fPersistenceError);
	_UpdateBreakCountdown(fSession.Countdown(time(nullptr)));
}
//---------------------------------------------------------------------------------------------------------------------------------//


void FatCatApp::_ShowMain() {
	if (!fMainWindow)
		return;
	if (fPreviewing)
		_CloseOverlays(); // a preview must not linger behind the settings window
	fMainWindow->PostUpdate(fSession, fPreferences, fPersistenceError);
	fMainWindow->PostMessage(kMsgWindowShow);
}
//---------------------------------------------------------------------------------------------------------------------------------//

void FatCatApp::_CloseOverlays(bool restoreMain) {
	FatCatDebug("_CloseOverlays: restore=%d count=%d pending=%d",
		(int)restoreMain, (int)fBreakWindows.size(),
		(int)fOverlayRequestPending);
	bool showMain = restoreMain && fRestoreMainAfterOverlay;
	if (fOverlayRequestPending) {
		fOverlayRequestPending = false;
		fOverlayRequestId++;
	}
	fRestoreMainAfterOverlay = false;
	for (BreakWindow* window : fBreakWindows)
		window->PostMessage(kMsgBreakClose);
	fBreakWindows.clear();
	fPreviewing = false;
	if (showMain)
		_ShowMain();
}
//---------------------------------------------------------------------------------------------------------------------------------//
void
FatCatApp::_ShowOverlay(bool preview)
{
	FatCatDebug("_ShowOverlay: preview=%d pending=%d main=%p hidden=%d",
		(int)preview, (int)fOverlayRequestPending, (void*)fMainWindow,
		fMainWindow != nullptr ? (int)fMainWindow->IsHidden() : -1);
	if (fOverlayRequestPending)
		return;
	_CloseOverlays(false);
	// Only a visible settings window needs the hide/confirm round trip. When it
	// is already out of the way there is nothing to snapshot around, and no
	// reply to wait for.
	if (fMainWindow && !fMainWindow->IsHidden()) {
		BMessage hide(kMsgWindowHideForOverlay);
		hide.AddBool("preview", preview);
		hide.AddInt32("request", ++fOverlayRequestId);
		if (fMainWindow->PostMessage(&hide) == B_OK) {
			fOverlayRequestPending = true;
			return;
		}
	}
	_CreateOverlay(preview, false);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void FatCatApp::_CreateOverlay(bool preview, bool mainWasVisible) {
	fOverlayRequestPending = false;
	fRestoreMainAfterOverlay = preview && mainWasVisible;
	fPreviewing = preview;
	BScreen screen;
	int32 index = 1;
	do {
		if (!screen.IsValid())
			break;
		BRect frame = screen.Frame();
		if (!frame.IsValid())
			break;
		BString screenName("Screen ");
		screenName << index++;
		if (!fPreferences.selectedMonitor.IsEmpty()
			&& fPreferences.selectedMonitor != screenName)
			continue;
		BreakWindow* window = new BreakWindow(frame, fPreferences,
			fSession.completedBreaks, preview, fPreferences.blockingBreak && !preview);
		fBreakWindows.push_back(window);
		window->Show();
		window->Activate();
		FatCatDebug("  window shown frame=(%.0f,%.0f,%.0f,%.0f) actual=(%.0f,%.0f,%.0f,%.0f) hidden=%d ws=%d",
			frame.left, frame.top, frame.right, frame.bottom,
			window->Frame().left, window->Frame().top,
			window->Frame().right, window->Frame().bottom,
			(int)window->IsHidden(), (int)window->CurrentWorkspace());
		BMessage countdown(kMsgBreakCountdown);
		countdown.AddString("countdown", fSession.Countdown(time(nullptr)));
		window->PostMessage(&countdown);
	} while (index <= kMaxScreens && screen.SetToNext() == B_OK);
	FatCatDebug("_CreateOverlay: after loop count=%d index=%d valid=%d",
		(int)fBreakWindows.size(), (int)index, (int)screen.IsValid());
	// Disconnected selection falls back to the first connected screen.
	if (fBreakWindows.empty()) {
		BScreen first;
		FatCatDebug("_CreateOverlay: fallback valid=%d", (int)first.IsValid());
		if (!first.IsValid() || !first.Frame().IsValid())
			return;
		BreakWindow* window = new BreakWindow(first.Frame(), fPreferences,
			fSession.completedBreaks, preview, fPreferences.blockingBreak && !preview);
		fBreakWindows.push_back(window);
		window->Show();
		window->Activate();
		BMessage countdown(kMsgBreakCountdown);
		countdown.AddString("countdown", fSession.Countdown(time(nullptr)));
		window->PostMessage(&countdown);
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


void FatCatApp::_UpdateBreakCountdown(const BString& countdown) {
	for (BreakWindow* window : fBreakWindows) {
		BMessage update(kMsgBreakCountdown);
		update.AddString("countdown", countdown);
		window->PostMessage(&update);
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool FatCatApp::_Unlocked(int32 id) const {
	static const int32 thresholds[] = { 0, 1, 3, 6 };
	return id >= 0 && id < 4 && fSession.completedBreaks >= thresholds[id];
}
//---------------------------------------------------------------------------------------------------------------------------------//


void FatCatApp::_ReplyStatus(BMessage* request) {
	BMessage reply(kMsgStatusReply);
	reply.AddString("version", kAppVersion);
	reply.AddInt32("phase", (int32)fSession.phase);
	reply.AddBool("paused", fSession.paused);
	reply.AddInt32("remaining", fSession.Remaining(time(nullptr)));
	reply.AddString("countdown", fSession.Countdown(time(nullptr)));
	reply.AddBool("preview", fPreviewing);
	reply.AddInt32("completed_breaks", fSession.completedBreaks);
	reply.AddString("persistence_error", fPersistenceError);
	request->SendReply(&reply);
}
//---------------------------------------------------------------------------------------------------------------------------------//

void FatCatApp::MessageReceived(BMessage* message) {
	time_t now = time(nullptr);
	switch (message->what) {
		case kMsgWindowHidden: {
			bool preview = false;
			bool wasVisible = false;
			int32 request = 0;
			message->FindBool("preview", &preview);
			message->FindBool("was_visible", &wasVisible);
			message->FindInt32("request", &request);
			FatCatDebug("kMsgWindowHidden: request=%d want=%d pending=%d",
				(int)request, (int)fOverlayRequestId,
				(int)fOverlayRequestPending);
			if (fOverlayRequestPending && request == fOverlayRequestId)
				_CreateOverlay(preview, wasVisible);
			break;
		}
		case B_ABOUT_REQUESTED:
			AboutRequested();
			break;
		case kMsgShow:
			FatCatDebug("app got kMsgShow");
			_ShowMain();
			break;
		case kMsgStart:
			_CloseOverlays();
			fSession.Start(now);
			_StateChanged();
			break;
		case kMsgPauseResume:
			if (fSession.paused) {
				fSession.Resume(now);
				if (fSession.phase == Phase::Break) _ShowOverlay(false);
			} else {
				fSession.Pause(now);
				_CloseOverlays();
			}
			_StateChanged();
			break;
		case kMsgPause:
			if (!fSession.paused) {
				fSession.Pause(now);
				_CloseOverlays();
				_StateChanged();
			}
			break;
		case kMsgResume:
			if (fSession.paused) {
				fSession.Resume(now);
				if (fSession.phase == Phase::Break) _ShowOverlay(false);
				_StateChanged();
			}
			break;
		case kMsgStop:
			_CloseOverlays();
			fSession.Stop();
			_StateChanged();
			break;
		case kMsgPreview:
			FatCatDebug("app got kMsgPreview");
			_ShowOverlay(true);
			break;
		case kMsgDismiss:
			_CloseOverlays();
			break;
		case kMsgSkipBreak:
			_CloseOverlays();
			fSession.SkipBreak(now);
			_StateChanged();
			break;
		case kMsgTick: {
			Phase before = fSession.phase;
			bool completedBreak = false;
			if (fSession.Tick(now, completedBreak)) {
				if (before == Phase::Focus && fSession.phase == Phase::Break) {
					_CloseOverlays(false); // a real break supersedes a preview
					_ShowOverlay(false);
				} else if (before == Phase::Break) {
					_CloseOverlays();
				}
				_StateChanged();
			} else {
				_UpdateBreakCountdown(fSession.Countdown(now));
			}
			break;
		}
		case kMsgSaveSettings: {
			int32 focus = fSession.focusMinutes, rest = fSession.breakMinutes;
			int32 longRest = fSession.longBreakMinutes, every = fSession.longBreakEvery;
			message->FindInt32("focus", &focus);
			message->FindInt32("break", &rest);
			message->FindInt32("long_break", &longRest);
			message->FindInt32("long_every", &every);
			if (fSession.Configure(focus, rest, longRest, every)) {
				bool value;
				if (message->FindBool("blocking", &value) == B_OK)
					fPreferences.blockingBreak = value;
				if (message->FindBool("reduced_motion", &value) == B_OK)
					fPreferences.reducedMotion = value;
				const char* monitor;
				if (message->FindString("monitor", &monitor) == B_OK)
					fPreferences.selectedMonitor = monitor;
				_SavePreferences();
				_StateChanged();
			}
			break;
		}
		case kMsgRenameCat: {
			int32 id; const char* name;
			if (message->FindInt32("id", &id) == B_OK && _Unlocked(id)
				&& message->FindString("name", &name) == B_OK) {
				BString clean(name); clean.Trim(); if (clean.Length() > 24) clean.Truncate(24);
				for (int32 i = clean.Length() - 1; i >= 0; --i) {
					unsigned char c = clean.ByteAt(i);
					if (c < 32 || c == 127) clean.Remove(i, 1);
				}
				// Only a real change may broadcast an update: the window answers
				// every update by re-sending the name it already holds.
				if (!clean.IsEmpty() && fPreferences.catNames[id] != clean) {
					fPreferences.catNames[id] = clean;
					_SavePreferences();
					_StateChanged();
				}
			}
			break;
		}
		case kMsgToggleFavorite: {
			int32 id;
			if (message->FindInt32("id", &id) == B_OK && _Unlocked(id)) {
				fPreferences.ToggleFavorite(id); _SavePreferences(); _StateChanged();
			}
			break;
		}
		case kMsgStatus:
			_ReplyStatus(message);
			break;
		case kMsgQuit:
			PostMessage(B_QUIT_REQUESTED);
			break;
		default:
			BApplication::MessageReceived(message);
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


bool FatCatApp::QuitRequested() {
	_SaveSession();
	_SavePreferences();
	_RemoveDeskbarItem();
	return true;
}
