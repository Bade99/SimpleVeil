#pragma once
#include "unCap_Vector.h"
#include "unCap_math.h"

enum class AnimateWindowToTrayTarget { window_to_tray, tray_to_window };

class AnimateWindowToTrayState {
public:
	static constexpr u32 animationId = 85694;
	static constexpr auto wndStateKey = TEXT("AnimateWindowToTrayState");

	void animate() {
		this->onAnimationStart();
		this->advanceAnimation();
	}

	AnimateWindowToTrayState(HWND wnd, v2 from, v2 to, AnimateWindowToTrayTarget target) {
		this->wnd = wnd;
		this->from = from;
		this->to = to;
		this->completed = false;
		this->refreshRateHz = win32_get_refresh_rate_hz(wnd);
		this->timer = StartCounter();
		this->windowScale = GetScalingForMonitorFromWindow(wnd);
		this->target = target;
	}
private:
	HWND wnd;
	v2 from, to;
	f32 duration = .2f; // seconds
	i64 timer;
	i32 refreshRateHz;
	bool completed;
	f32 windowScale;
	AnimateWindowToTrayTarget target;


	void simulate(HWND hwnd, UINT, UINT_PTR anim_id, DWORD) {
		if (this->completed) {
			KillTimer(hwnd, anim_id);
			this->onAnimationEnd();
			delete this;
		}
		else {
			static auto lerp = [](f32 n1, f32 t, f32 n2) -> f32 { return (1.f - t) * n1 + t * n2; };
			static auto parametric_blend = [](f32 t) -> f32 { f32 sqt = t * t; return sqt / (2.0f * (sqt - t) + 1.0f); };
			static auto clamp = [](f32 min, f32 n, f32 max) -> f32 { f32 res = n; if (res < min) res = min; else if (res > max) res = max; return res; };
			static auto clamp01 = [&](f32 n) -> f32 { return clamp(0.f, n, 1.f); };

			auto elapsed_t = EndCounter(this->timer, CounterUnit::seconds);
			auto distance_threshold = 500.f * this->windowScale;
			auto duration = clamp(this->duration, this->duration * floorf(length(to - from) / distance_threshold), this->duration * 3); //TODO: if the distance is so long that we get to the max duration then we should consider travelling only part way to our destination (when hiding we would stop early, and when showing we would start from a closer position to the final one)
			auto t = parametric_blend(clamp01(elapsed_t / duration));

			u8 alphaStart, alphaEnd;
			if (this->target == AnimateWindowToTrayTarget::window_to_tray) {
				alphaStart = 255;
				alphaEnd = 0;
			}
			else {
				alphaStart = 0;
				alphaEnd = 255;
			}

			SetLayeredWindowAttributes(this->wnd, NULL, lerp(alphaStart, t, alphaEnd), LWA_ALPHA);
			SetWindowPos(this->wnd, NULL,
				lerp(this->from.x, t, this->to.x), lerp(this->from.y, t, this->to.y),
				0, 0, SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSIZE);
			this->completed = t == 1;

			this->advanceAnimation();
		}
	}

	void configureHwnd() {
		SetProp(this->wnd, this->wndStateKey, this);
	}

	void cleanupHwnd() {
		RemoveProp(this->wnd, this->wndStateKey);
	}

	void onAnimationStart() {
		this->configureHwnd();
		if (this->target == AnimateWindowToTrayTarget::window_to_tray)
			SetWindowLongPtr(this->wnd, GWL_EXSTYLE, GetWindowLongPtr(this->wnd, GWL_EXSTYLE) | WS_EX_LAYERED);
		else {
			SetWindowLongPtr(this->wnd, GWL_EXSTYLE, GetWindowLongPtr(this->wnd, GWL_EXSTYLE) | WS_EX_LAYERED);
			SetLayeredWindowAttributes(this->wnd, 0, 0, LWA_ALPHA);
			ShowWindow(this->wnd, SW_SHOW);
		}
	}

	void onAnimationEnd() {
		this->cleanupHwnd();
		if (this->target == AnimateWindowToTrayTarget::window_to_tray) {
			SetLayeredWindowAttributes(this->wnd, 0, 0, LWA_ALPHA);
			SetWindowPos(this->wnd, NULL, this->from.x, this->from.y, 0, 0, SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSIZE);
			ShowWindow(this->wnd, SW_HIDE);
			SetLayeredWindowAttributes(this->wnd, 0, 255, LWA_ALPHA);
			SetWindowLongPtr(this->wnd, GWL_EXSTYLE, GetWindowLongPtr(this->wnd, GWL_EXSTYLE) ^ WS_EX_LAYERED);
		}
		else {
			SetLayeredWindowAttributes(this->wnd, 0, 255, LWA_ALPHA);
			SetWindowLongPtr(this->wnd, GWL_EXSTYLE, GetWindowLongPtr(this->wnd, GWL_EXSTYLE) ^ WS_EX_LAYERED);
			SetWindowPos(this->wnd, NULL, this->to.x, this->to.y, 0, 0, SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSIZE);//just to make sure, dont tell it to redraw cause it takes a long time to do it and looks bad
			SetActiveWindow(this->wnd);
			SetForegroundWindow(this->wnd);
		}
	}

	void advanceAnimation() {
		SetTimer(this->wnd, this->animationId, USER_TIMER_MINIMUM, [](HWND x, UINT y, UINT_PTR z, DWORD w) {((AnimateWindowToTrayState*)GetProp(x, AnimateWindowToTrayState::wndStateKey))->simulate(x, y, z, w); });
	}
};

static void AnimateWindowToTray(HWND wnd, v2 from, v2 to, AnimateWindowToTrayTarget target) {
	(new AnimateWindowToTrayState(wnd, from, to, target))->animate();
}

/// <summary>
/// Minimizes a window and creates an animation to make it look like it goes to the tray
/// </summary>
/// <param name="hWnd"></param>
static void MinimizeWndToTray(HWND hWnd)
{//Thanks to: https://www.codeproject.com/Articles/735/Minimizing-windows-to-the-System-Tray
	if (GetDoAnimateMinimize())
	{
		RECT rcFrom; GetWindowRect(hWnd, &rcFrom);
		RECT rcTo; GetTrayWndRect(&rcTo);

		AnimateWindowToTray(hWnd, v2{ (f32)rcFrom.left,(f32)rcFrom.top }, v2{ (f32)rcTo.left,(f32)rcTo.top }, AnimateWindowToTrayTarget::window_to_tray);
	}
	else ShowWindow(hWnd, SW_HIDE);// Just hide the window
}

/// <summary>
/// Restores a window and makes it look like it comes out of the tray 
/// <para>and makes it back to where it was before minimizing</para>
/// </summary>
/// <param name="hWnd"></param>
static void RestoreWndFromTray(HWND hWnd)
{//Thanks to: https://www.codeproject.com/Articles/735/Minimizing-windows-to-the-System-Tray
	if (GetDoAnimateMinimize())
	{
		// Get the rect of the tray and the window. Note that the window rect
		// is still valid even though the window is hidden
		RECT rcFrom; GetTrayWndRect(&rcFrom);
		RECT rcTo; GetWindowRect(hWnd, &rcTo);

		AnimateWindowToTray(hWnd, v2{ (f32)rcFrom.left,(f32)rcFrom.top }, v2{ (f32)rcTo.left,(f32)rcTo.top }, AnimateWindowToTrayTarget::tray_to_window);
	}
	else {
		// Show the window, and make sure we're the foreground window
		ShowWindow(hWnd, SW_SHOW);
		SetActiveWindow(hWnd);
		SetForegroundWindow(hWnd);
	}
}
