#include "Preferences.h"

#include <Application.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <Roster.h>

#include <algorithm>
#include <cstdio>

static BString
CleanName(const char* input, const char* fallback)
{
	BString value(input ? input : "");
	value.Trim();
	if (value.Length() > 24)
		value.Truncate(24);
	for (int32 i = value.Length() - 1; i >= 0; --i) {
		unsigned char c = value.ByteAt(i);
		if (c < 32 || c == 127)
			value.Remove(i, 1);
	}
	return value.IsEmpty() ? BString(fallback) : value;
}

bool
Preferences::IsFavorite(int32 id) const
{
	return std::find(favorites.begin(), favorites.end(), id) != favorites.end();
}

void
Preferences::ToggleFavorite(int32 id)
{
	auto found = std::find(favorites.begin(), favorites.end(), id);
	if (found == favorites.end())
		favorites.push_back(id);
	else
		favorites.erase(found);
}

void
Preferences::Archive(BMessage& into) const
{
	into.MakeEmpty();
	into.AddInt32("version", 1);
	into.AddBool("reduced_motion", reducedMotion);
	into.AddBool("blocking_break", blockingBreak);
	into.AddString("monitor", selectedMonitor);
	for (int32 i = 0; i < 4; ++i)
		into.AddString("cat_name", catNames[i]);
	for (int32 id : favorites)
		into.AddInt32("favorite", id);
}

void
Preferences::Restore(const BMessage& from)
{
	from.FindBool("reduced_motion", &reducedMotion);
	from.FindBool("blocking_break", &blockingBreak);
	const char* monitor;
	if (from.FindString("monitor", &monitor) == B_OK)
		selectedMonitor = monitor;
	static const char* defaults[] = { "Mochi", "Miso", "Patches", "Pepper" };
	for (int32 i = 0; i < 4; ++i) {
		const char* name;
		if (from.FindString("cat_name", i, &name) == B_OK)
			catNames[i] = CleanName(name, defaults[i]);
	}
	favorites.clear();
	int32 id;
	for (int32 i = 0; from.FindInt32("favorite", i, &id) == B_OK; ++i) {
		if (id >= 0 && id < 4 && !IsFavorite(id))
			favorites.push_back(id);
	}
}

BString
SettingsPath(const char* leaf)
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) != B_OK)
		return BString();
	path.Append("FatCat");
	create_directory(path.Path(), 0755);
	path.Append(leaf);
	return path.Path();
}

status_t
LoadFlatMessage(const char* leaf, BMessage& message)
{
	BString path = SettingsPath(leaf);
	BFile file(path.String(), B_READ_ONLY);
	return file.InitCheck() == B_OK ? message.Unflatten(&file) : file.InitCheck();
}

status_t
SaveFlatMessage(const char* leaf, const BMessage& message)
{
	BString finalPath = SettingsPath(leaf);
	BString tempPath(finalPath);
	tempPath << ".new";
	BFile file(tempPath.String(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();
	if (status == B_OK)
		status = message.Flatten(&file);
	file.Unset();
	if (status != B_OK) {
		remove(tempPath.String());
		return status;
	}
	return rename(tempPath.String(), finalPath.String());
}

BString
ResourcePath(const char* leaf)
{
	app_info info;
	BPath path;
	if (be_app && be_app->GetAppInfo(&info) == B_OK) {
		BEntry entry(&info.ref);
		entry.GetPath(&path);
		path.GetParent(&path);
		path.Append("assets");
		path.Append(leaf);
		if (BEntry(path.Path()).Exists())
			return path.Path();
	}
	BString installed("/boot/system/data/FatCat/assets/");
	installed << leaf;
	return installed;
}
