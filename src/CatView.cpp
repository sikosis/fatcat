#include "CatView.h"

#include "Debug.h"

#include <Message.h>
#include <TranslationUtils.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

static constexpr uint32 kAnimate = 'anim';
static const char* kFiles[] = {
	"cat-orange.png", "cat-gray.png", "cat-calico.png", "cat-tuxedo.png"
};
static const int32 kUnlocks[] = { 0, 1, 3, 6 };

static float RandomUnit() {
	return (float)rand() / (float)RAND_MAX;
}
//---------------------------------------------------------------------------------------------------------------------------------//


CatView::CatView(const Preferences& preferences, int32 completedBreaks, bool preview,
	bool reducedMotion, BBitmap* backdrop)
	:
	BView("cat playground", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	fPreferences(preferences),
	fCompletedBreaks(completedBreaks),
	fPreview(preview),
	fReducedMotion(reducedMotion),
	fBackdrop(backdrop)
{
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	for (int32 i = 0; i < 4; ++i)
		fSheets[i].reset(BTranslationUtils::GetBitmap(ResourcePath(kFiles[i]).String()));
	int loaded = 0;
	for (int32 i = 0; i < 4; ++i)
		if (fSheets[i])
			loaded++;
	FatCatDebug("CatView ctor this=%p backdrop=%d sheets=%d/4",
		(void*)this, (int)(fBackdrop != nullptr), (int)loaded);
}
//---------------------------------------------------------------------------------------------------------------------------------//


CatView::~CatView() = default;
//---------------------------------------------------------------------------------------------------------------------------------//


void CatView::AttachedToWindow() {
	FatCatDebug("CatView::AttachedToWindow this=%p window=%p",
		(void*)this, (void*)Window());
	_Reset();
	if (!fReducedMotion) {
		BMessage message(kAnimate);
		fRunner = std::make_unique<BMessageRunner>(BMessenger(this), &message, 33333);
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


float CatView::_CatSize() const {
	float count = std::max<size_t>(1, fCats.size());
	return std::max(96.0f, std::min({ 208.0f, Bounds().Width() / count,
		Bounds().Height() * 0.42f }));
}
//---------------------------------------------------------------------------------------------------------------------------------//


void CatView::_Reset() {
	std::vector<int32> available;
	for (int32 id = 0; id < 4; ++id) {
		if (fPreview || fCompletedBreaks >= kUnlocks[id])
			available.push_back(id);
	}
	if (!fPreview && !fPreferences.favorites.empty()) {
		std::vector<int32> favorites;
		for (int32 id : available) {
			if (fPreferences.IsFavorite(id))
				favorites.push_back(id);
		}
		if (!favorites.empty())
			available = favorites;
	}
	fCats.clear();
	float size = _CatSize();
	float maxX = std::max(0.0f, Bounds().Width() - size);
	for (size_t i = 0; i < available.size(); ++i) {
		Cat cat;
		cat.variant = available[i];
		cat.x = std::clamp((maxX + size) * (i + 0.5f) / available.size() - size / 2,
			0.0f, maxX);
		cat.lane = available.size() > 1 ? (float)i / (available.size() - 1) : 0.6f;
		cat.direction = RandomUnit() < 0.5f ? -1 : 1;
		const float speeds[] = { 30, 52, 72, 36 };
		cat.speed = speeds[cat.variant] * (0.85f + RandomUnit() * 0.3f);
		cat.activity = fReducedMotion ? 4 : 0;
		cat.remaining = 2 + RandomUnit() * 4;
		cat.greetingCooldown = 0;
		cat.hop = 0;
		cat.frame = 0;
		cat.frameElapsed = 0;
		fCats.push_back(cat);
	}
	fLastTick = system_time();
	Invalidate();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void CatView::FrameResized(float, float) {
	_Reset();
}
//---------------------------------------------------------------------------------------------------------------------------------//


void CatView::_ChooseActivity(Cat& cat) {
	static const float weights[4][6] = {
		{ .30, .08, .08, .14, .15, .25 }, { .55, .10, .14, .06, .10, .05 },
		{ .62, .12, .10, .05, .07, .04 }, { .35, .10, .18, .08, .19, .10 }
	};
	float pick = RandomUnit(), cumulative = 0;
	cat.activity = 5;
	for (int32 i = 0; i < 6; ++i) {
		cumulative += weights[cat.variant][i];
		if (pick < cumulative) { cat.activity = i; break; }
	}
	cat.remaining = cat.activity == 5 ? 7 + RandomUnit() * 8
		: cat.activity == 0 ? 3 + RandomUnit() * 5 : 2 + RandomUnit() * 3;
	if (cat.activity == 0) {
		cat.direction = RandomUnit() < 0.5f ? -1 : 1;
		if (cat.variant == 2 && RandomUnit() < .25f)
			cat.hop = .55f;
	}
	cat.frame = 0;
	cat.frameElapsed = 0;
}
//---------------------------------------------------------------------------------------------------------------------------------//


BRect CatView::_CatFrame(const Cat& cat) const {
	float size = _CatSize();
	float groundTop = Bounds().Height() * .55f;
	float floorY = Bounds().Height() - size - 16;
	float baseY = std::min(floorY, groundTop)
		+ std::max(0.0f, floorY - groundTop) * cat.lane;
	float hop = cat.hop > 0 ? sinf(3.14159265358979323846f
		* (1 - cat.hop / .55f)) * 24 : 0;
	return BRect(cat.x, baseY - hop, cat.x + size, baseY - hop + size);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void CatView::_Advance(float seconds) {
	float dt = std::clamp(seconds, 0.0f, 0.1f);
	float size = _CatSize();
	float maxX = std::max(0.0f, Bounds().Width() - size);
	BRect dirty;
	bool hasDirty = false;
	auto include = [&dirty, &hasDirty](BRect frame) {
		dirty = hasDirty ? dirty | frame : frame;
		hasDirty = true;
	};
	for (Cat& cat : fCats) {
		include(_CatFrame(cat));
		cat.remaining -= dt;
		cat.hop = std::max(0.0f, cat.hop - dt);
		cat.greetingCooldown = std::max(0.0f, cat.greetingCooldown - dt);
		if (cat.remaining <= 0)
			_ChooseActivity(cat);
		if (cat.activity == 0) {
			for (Cat& peer : fCats) {
				if (&peer != &cat && std::abs(peer.lane - cat.lane) < .4f
					&& std::abs(peer.x - cat.x) < size * .75f
					&& cat.greetingCooldown <= 0) {
					cat.greetingCooldown = 12;
					if (cat.variant == 3)
						cat.direction = peer.x >= cat.x ? -1 : 1;
					else {
						cat.activity = 2;
						cat.remaining = 2.5f;
						cat.hop = 0;
						cat.frame = 0;
						cat.frameElapsed = 0;
					}
					break;
				}
			}
			if (cat.activity == 0) {
				cat.x += cat.direction * cat.speed * dt;
				if (cat.x >= maxX) { cat.x = maxX; cat.direction = -1; }
				else if (cat.x <= 0) { cat.x = 0; cat.direction = 1; }
			}
		} else
			cat.hop = 0;

		static const float frameIntervals[] = { .15f, .30f, .25f, .42f, .90f, 1.30f };
		cat.frameElapsed += dt;
		while (cat.frameElapsed >= frameIntervals[cat.activity]) {
			cat.frameElapsed -= frameIntervals[cat.activity];
			cat.frame = (cat.frame + 1) % 4;
		}
		include(_CatFrame(cat));
	}
	if (hasDirty) {
		dirty.InsetBy(-2, -2);
		Invalidate(dirty);
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//


void CatView::MessageReceived(BMessage* message) {
	if (message->what == kAnimate) {
		bigtime_t now = system_time();
		_Advance((now - fLastTick) / 1000000.0f);
		fLastTick = now;
		return;
	}
	BView::MessageReceived(message);
}
//---------------------------------------------------------------------------------------------------------------------------------//


void CatView::Draw(BRect update) {
	if (++fDrawLogCount <= 3 || fDrawLogCount == 60)
		FatCatDebug("CatView::Draw this=%p n=%d update=(%.0f,%.0f,%.0f,%.0f) window=%p",
			(void*)this, fDrawLogCount, update.left, update.top, update.right,
			update.bottom, (void*)Window());
	SetDrawingMode(B_OP_COPY);
	if (fBackdrop)
		DrawBitmap(fBackdrop.get(), update, update);
	else {
		SetHighUIColor(B_PANEL_BACKGROUND_COLOR);
		FillRect(update);
	}

	for (const Cat& cat : fCats) {
		BBitmap* sheet = fSheets[cat.variant].get();
		if (!sheet)
			continue;
		float cellW = (sheet->Bounds().Width() + 1) / 4;
		float cellH = (sheet->Bounds().Height() + 1) / 6;
		int32 activity = fReducedMotion ? 4 : cat.activity;
		BRect source(cat.frame * cellW, activity * cellH,
			(cat.frame + 1) * cellW - 1, (activity + 1) * cellH - 1);
		BRect destination = _CatFrame(cat);
		SetDrawingMode(B_OP_ALPHA);
		SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		if (cat.direction < 0) {
			PushState();
			TranslateBy(destination.left + destination.right, 0);
			ScaleBy(-1, 1);
		}
		DrawBitmap(sheet, source, destination);
		if (cat.direction < 0)
			PopState();
	}
}
//---------------------------------------------------------------------------------------------------------------------------------//
