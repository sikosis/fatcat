#pragma once

#include <Message.h>
#include <String.h>

#include <array>
#include <vector>

struct Preferences {
	bool reducedMotion = false;
	bool blockingBreak = true;
	BString selectedMonitor;
	std::array<BString, 4> catNames { "Mochi", "Miso", "Patches", "Pepper" };
	std::vector<int32> favorites;

	bool IsFavorite(int32 id) const;
	void ToggleFavorite(int32 id);
	void Archive(BMessage& into) const;
	void Restore(const BMessage& from);
};

status_t LoadFlatMessage(const char* leaf, BMessage& message);
status_t SaveFlatMessage(const char* leaf, const BMessage& message);
BString SettingsPath(const char* leaf);
BString ResourcePath(const char* leaf);

