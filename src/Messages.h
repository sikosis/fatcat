#pragma once

#include <SupportDefs.h>

constexpr const char* kAppSignature = "application/x-vnd.sikosis-fatcat";
constexpr const char* kDeskbarSignature
	= "application/x-vnd.sikosis-fatcatDeskbar";
constexpr const char* kAppName = "Fat Cat";
constexpr const char* kAppVersion = "0.16";
constexpr const char* kAppDescription
	= "A cozy Pomodoro timer that fills your breaks with collectible animated cats.";
constexpr const char* kDeskbarItemName = "fatcatDeskbar";

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
	kMsgStatusReply = 'fcqr',
	kMsgTick = 'fctk',
	kMsgQuit = 'fcqt',
	kMsgPause = 'fcpa',
	kMsgResume = 'fcre',
	kMsgWindowUpdate = 'fcwu',
	kMsgWindowShow = 'fcws',
	kMsgWindowHideForOverlay = 'fcwh',
	kMsgWindowHidden = 'fcwd',
	kMsgBreakCountdown = 'fcbc',
	kMsgBreakClose = 'fcbq'
};
