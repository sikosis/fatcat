#pragma once

#include "Preferences.h"
#include "Session.h"

#include <Window.h>

class BButton;
class BCheckBox;
class BMenuField;
class BStringView;
class BTabView;
class BTextControl;

class MainWindow : public BWindow {
public:
	MainWindow(const Session& session, const Preferences& preferences);
	bool QuitRequested() override;
	void MessageReceived(BMessage* message) override;
	void Update(const Session& session, const Preferences& preferences,
		const BString& persistenceError);

private:
	BView* _BuildTimerTab();
	BView* _BuildCatsTab(const Session& session, const Preferences& preferences);
	void _SendSettings();
	void _UpdateControls();

	Session fSession;
	Preferences fPreferences;
	BString fError;
	BStringView* fStatus;
	BStringView* fProgress;
	BStringView* fSaveMessage;
	BButton* fPrimary;
	BButton* fStop;
	BTextControl* fFocus;
	BTextControl* fBreak;
	BTextControl* fLongBreak;
	BTextControl* fEvery;
	BCheckBox* fBlocking;
	BCheckBox* fMotion;
	BMenuField* fMonitor;
	BStringView* fCatNameLabels[4];
	BTextControl* fCatNames[4];
	BButton* fFavoriteButtons[4];
	bool fSettingsInitialized;
};
