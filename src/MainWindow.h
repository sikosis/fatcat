#pragma once

#include "Preferences.h"
#include "Session.h"

#include <MessageRunner.h>
#include <Window.h>

#include <memory>

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
	void PostUpdate(const Session& session, const Preferences& preferences,
		const BString& persistenceError);

private:
	BView* _BuildTimerTab();
	BView* _BuildCatsTab(const Session& session, const Preferences& preferences);
	bool _PostApplicationMessage(const BMessage& message);
	void _SendSettings();
	void _ApplyProfile(const char* name);
	const char* _ProfileNameFor(int32 focus, int32 rest, int32 longRest,
		int32 every) const;
	void _UpdateProfileSelection();
	void _ApplyUpdate(const BMessage& message);
	void _UpdateControls();
	void _UpdateStatus();

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
	BMenuField* fProfile;
	BStringView* fCatNameLabels[4];
	BTextControl* fCatNames[4];
	BButton* fFavoriteButtons[4];
	std::unique_ptr<BMessageRunner> fTicker;
	bool fSettingsInitialized;
};
