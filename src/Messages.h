#pragma once

#include <SupportDefs.h>

constexpr const char* kAppSignature = "application/x-vnd.arkane-FatCat";
constexpr const char* kDeskbarSignature
	= "application/x-vnd.arkane-FatCatDeskbar";
constexpr const char* kAppName = "Fat Cat";
constexpr const char* kAppVersion = "0.06";
constexpr const char* kAppDescription
	= "A cozy Pomodoro timer that fills your breaks with collectible animated cats.";
constexpr const char* kDeskbarItemName = "FatCatDeskbar";

enum : uint32 {
	kMsgShow = 'fcsh',
	kMsgStart = 'fcst',
	kMsgPauseResume = 'fcpp',
	kMsgStop = 'fcsp',
	kMsgPreview = 'fcpv',
	kMsgDismiss = 'fcdm',
	kMsgSkipBreak = 'fcsk',
	kMsgSaveSettings = 'fcsv',
	kMsgRenameCat = 'fcrn',
	kMsgToggleFavorite = 'fcfv',
	kMsgStatus = 'fcqs',
	kMsgTick = 'fctk',
	kMsgQuit = 'fcqt',
	kMsgPause = 'fcpa',
	kMsgResume = 'fcre'
};
