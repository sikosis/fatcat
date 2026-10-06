#pragma once

#include "Preferences.h"
#include "Session.h"

#include <Application.h>
#include <MessageRunner.h>

#include <memory>
#include <vector>

class BreakWindow;
class MainWindow;

class FatCatApp : public BApplication {
public:
	FatCatApp();
	void ReadyToRun() override;
	void ArgvReceived(int32 argc, char** argv) override;
	void AboutRequested() override;
	void MessageReceived(BMessage* message) override;
	bool QuitRequested() override;

private:
	void _EnsureDeskbarItem();
	void _Load();
	void _SaveSession();
	void _SavePreferences();
	void _StateChanged();
	void _ShowMain();
	void _ShowOverlay(bool preview);
	void _CloseOverlays(bool restoreMain = true);
	void _ReplyStatus(BMessage* request);
	bool _Unlocked(int32 id) const;

	Session fSession;
	Preferences fPreferences;
	BString fPersistenceError;
	MainWindow* fMainWindow = nullptr;
	std::vector<BreakWindow*> fBreakWindows;
	std::unique_ptr<BMessageRunner> fTicker;
	int32 fDeskbarCheckTicks = 0;
	bool fPreviewing = false;
	bool fRestoreMainAfterOverlay = false;
	bool fCommandLineOnly = false;
};
