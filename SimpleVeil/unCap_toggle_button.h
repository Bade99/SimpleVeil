#pragma once
#include <Windows.h>
#include <windowsx.h>
#include "unCap_Global.h"
#include "unCap_Helpers.h"
#include "unCap_Renderer.h"
#include "windows_undoc.h"

//NOTE: this buttons can have text or an img, but not both at the same time
//NOTE: it's important that the parent uses WS_CLIPCHILDREN to avoid horrible flickering
//NOTE: this button follows the standard button tradition of getting the msg to send to the parent from the hMenu param of CreateWindow/Ex
//NOTE: when clicked notifies the parent through WM_COMMAND with LOWORD(wParam)= msg number specified in hMenu param of CreateWindow/Ex ; HIWORD(wParam)=0 ; lParam= HWND of the button. Just like the standard button

constexpr TCHAR unCap_wndclass_toggle_button[] = TEXT("unCap_wndclass_toggle_button");

static LRESULT CALLBACK ToggleButtonProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

#define ANIMATION_TOGGLE_BUTTON 1

struct ToggleButtonProcState { //NOTE: must be initialized to zero
	HWND wnd;
	HWND parent;

	bool onMouseOver;
	bool onLMouseClick;//The left click is being pressed on the background area
	bool OnMouseTracking; //Left click was pressed in our bar and now is still being held, the user will probably be moving the mouse around, so we want to track it to move the scrollbar

	UINT msg_to_send;

	bool* on; //IMPORTANT: user MUST call UNCAPTGLBTN_set_state_reference after window creation to initialize the pointer

	struct {
		HBRUSH border;
		HBRUSH fg;
		HBRUSH bk_on, bk_off;
		HBRUSH bk_push_on, bk_push_off;
		HBRUSH bk_mouseover_on, bk_mouseover_off;
	} brushes;

	struct {
		bool** on;
		f32 percentage_speed;
		f32 percentage_position;
		i32 refresh_rate_hz;
		i64 timer;
	} animation_state;
};

void init_wndclass_unCap_toggle_button(HINSTANCE instance) {
	WNDCLASSEXW button;

	button.cbSize = sizeof(button);
	button.style = CS_HREDRAW | CS_VREDRAW;
	button.lpfnWndProc = ToggleButtonProc;
	button.cbClsExtra = 0;
	button.cbWndExtra = sizeof(ToggleButtonProcState*);
	button.hInstance = instance;
	button.hIcon = NULL;
	button.hCursor = LoadCursor(nullptr, IDC_HAND);
	button.hbrBackground = NULL;
	button.lpszMenuName = NULL;
	button.lpszClassName = unCap_wndclass_toggle_button;
	button.hIconSm = NULL;

	ATOM class_atom = RegisterClassExW(&button);
	Assert(class_atom);
}

ToggleButtonProcState* UNCAPTGLBTN_get_state(HWND hwnd) {
	auto state = (ToggleButtonProcState*)GetWindowLongPtr(hwnd, 0);//INFO: windows recomends to use GWL_USERDATA https://docs.microsoft.com/en-us/windows/win32/learnwin32/managing-application-state-
	return state;
}

void UNCAPTGLBTN_ask_for_repaint(ToggleButtonProcState* state) {
	InvalidateRect(state->wnd, NULL, FALSE);
}

//NOTE: any NULL HBRUSH remains unchanged
void UNCAPTGLBTN_set_brushes(HWND uncap_btn, BOOL repaint, 
	HBRUSH border, HBRUSH fg,
	HBRUSH bk_on, HBRUSH bk_off,
	HBRUSH bk_push_on, HBRUSH bk_push_off,
	HBRUSH bk_mouseover_on, HBRUSH bk_mouseover_off) {
	if (uncap_btn) {
		auto state = UNCAPTGLBTN_get_state(uncap_btn);
		if (border)state->brushes.border = border;
		if (fg)state->brushes.fg = fg;
		if (bk_on)state->brushes.bk_on = bk_on;
		if (bk_off)state->brushes.bk_off = bk_off;
		if (bk_push_on)state->brushes.bk_push_on = bk_push_on;
		if (bk_push_off)state->brushes.bk_push_off = bk_push_off;
		if (bk_mouseover_on)state->brushes.bk_mouseover_on = bk_mouseover_on;
		if (bk_mouseover_off)state->brushes.bk_mouseover_off = bk_mouseover_off;
		if (repaint) UNCAPTGLBTN_ask_for_repaint(state);
	}
}


void UNCAPTGLBTN_stop_capturing_mouse(ToggleButtonProcState* state) {
	ReleaseCapture();
	state->OnMouseTracking = false;
}

void UNCAPTGLBTN_create_animation_state(ToggleButtonProcState* state) {
	state->animation_state.on = &state->on;
	state->animation_state.percentage_speed = 1; //1 means do 100% in 1 second, .5 means do 50% in 1 second, and so on
	state->animation_state.percentage_position = **state->animation_state.on ? 1 : 0;
}

void UNCAPTGLBTN_set_state_reference(HWND wnd, bool* on) {
	auto state = UNCAPTGLBTN_get_state(wnd);
	state->on = on;
	UNCAPTGLBTN_create_animation_state(state);
}

void UNCAPTGLBTN_queue_next_animation_frame(ToggleButtonProcState* state, u32 animation_id);

void UNCAPTGLBTN_play_animation(HWND hwnd, UINT, UINT_PTR anim_id, DWORD) {
	auto state = UNCAPTGLBTN_get_state(hwnd);
	bool next_frame = false;

	f32 dt = EndCounter(state->animation_state.timer, CounterUnit::seconds);

	bool on = **state->animation_state.on;

	f32 t = state->animation_state.percentage_position + state->animation_state.percentage_speed * dt * (on * 2 - 1);

	f32 new_percentage_position = lerp(0, ParametricBlend(clamp01(t)), 1);

	state->animation_state.percentage_position = new_percentage_position;

	UNCAPTGLBTN_ask_for_repaint(state);

	if (on - state->animation_state.percentage_position)
		UNCAPTGLBTN_queue_next_animation_frame(state, anim_id);
	else 
		KillTimer(state->wnd, anim_id);
}

void UNCAPTGLBTN_queue_next_animation_frame(ToggleButtonProcState* state, u32 animation_id) {
	SetTimer(state->wnd, animation_id, 1000 / state->animation_state.refresh_rate_hz, UNCAPTGLBTN_play_animation); //TODO: this should be corrected based on dt, though SetTimer actually is terrible for ms range timers
}

void UNCAPTGLBTN_start_animation(ToggleButtonProcState* state) {
	state->animation_state.timer = StartCounter();
	state->animation_state.refresh_rate_hz = win32_get_refresh_rate_hz(state->wnd);
	UNCAPTGLBTN_queue_next_animation_frame(state, ANIMATION_TOGGLE_BUTTON);
}

static LRESULT CALLBACK ToggleButtonProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

	auto state = UNCAPTGLBTN_get_state(hwnd);
	switch (msg) {
	case WM_CANCELMODE:
	{
		if (state->OnMouseTracking) UNCAPTGLBTN_stop_capturing_mouse(state);
		state->onLMouseClick = false;
		state->onMouseOver = false;
		UNCAPTGLBTN_ask_for_repaint(state);
		return 0;
	} break;
	
	case WM_CAPTURECHANGED:
	{
		UNCAPTGLBTN_ask_for_repaint(state);
		return 0;
	} break;
	case WM_LBUTTONUP:
	{
		if (state->OnMouseTracking) {
			if (state->onMouseOver) {
				SendMessage(state->parent, WM_COMMAND, (WPARAM)MAKELONG(state->msg_to_send, 0), (LPARAM)state->wnd);
				UNCAPTGLBTN_start_animation(state);
			}
			UNCAPTGLBTN_stop_capturing_mouse(state);
		}
		state->onLMouseClick = false;

		return 0;
	} break;
	case WM_LBUTTONDOWN:
	{
		if (state->onMouseOver) {
			SetCapture(state->wnd);
			state->OnMouseTracking = true;
			state->onLMouseClick = true;
		}
		UNCAPTGLBTN_ask_for_repaint(state);
		return 0;
	} break;
	case WM_MOUSELEAVE:
	{
		state->onMouseOver = false;
		UNCAPTGLBTN_ask_for_repaint(state);
		return 0;
	} break;
	case WM_MOUSEMOVE:
	{
		POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//client coords, relative to upper-left corner
		bool prev_onMouseOver = state->onMouseOver;
		RECT rc; GetClientRect(state->wnd, &rc);
		state->onMouseOver = test_pt_rc(mouse, rc);
		bool state_change = prev_onMouseOver != state->onMouseOver;
		if (state_change) {
			UNCAPTGLBTN_ask_for_repaint(state);
			TRACKMOUSEEVENT track;
			track.cbSize = sizeof(track);
			track.hwndTrack = state->wnd;
			track.dwFlags = TME_LEAVE;
			TrackMouseEvent(&track);
		}

		return 0;
	} break;
	case WM_NCHITTEST:
	{
		POINT mouse = { GET_X_LPARAM(lparam), GET_Y_LPARAM(lparam) };//Screen coords, relative to upper-left corner
		RECT rw; GetWindowRect(state->wnd, &rw);
		LRESULT hittest = HTNOWHERE;
		if (test_pt_rc(mouse, rw)) hittest = HTCLIENT;
		return hittest;
	} break;
	case WM_SIZE: {
		//NOTE: neat, here you resize your render target, if I had one or cared to resize windows' https://docs.microsoft.com/en-us/windows/win32/winmsg/wm-size
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCCREATE: 
	{
		auto st = (ToggleButtonProcState*)calloc(1, sizeof(ToggleButtonProcState));
		Assert(st);
		SetWindowLongPtr(hwnd, 0, (LONG_PTR)st);
		CREATESTRUCT* creation_nfo = (CREATESTRUCT*)lparam;
		st->parent = creation_nfo->hwndParent;
		st->wnd = hwnd;
		st->msg_to_send = (UINT)(UINT_PTR)creation_nfo->hMenu;
		return 1;
	} break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(state->wnd, &ps); defer{ EndPaint(state->wnd, &ps); };

		RECT r; GetClientRect(state->wnd, &r);
		int h = RECTHEIGHT(r);
		int w = RECTWIDTH(r);

		auto& brushes = state->brushes;
		HBRUSH bk_on_br, bk_off_br;
		if (state->onMouseOver && state->onLMouseClick) {
			bk_on_br = brushes.bk_push_on;
			bk_off_br = brushes.bk_push_off;
		}
		else if (state->onMouseOver || state->OnMouseTracking) {
			bk_on_br = brushes.bk_mouseover_on;
			bk_off_br = brushes.bk_mouseover_off;
		}
		else {
			bk_on_br = brushes.bk_on;
			bk_off_br = brushes.bk_off;
		}

		bool on = **state->animation_state.on;

		if (!(state->animation_state.percentage_position == 1 && on))
			urender::draw_round_rectangle(dc, r, h / 2.f, bk_off_br);

		f32 button_displacement = state->animation_state.percentage_position * (w - h);
		i32 button_left = r.left + button_displacement;
		RECT button{ button_left, r.top, button_left + h, h };
		rect_f32 deflated_button = rect_f32::from_RECT(button);
		f32 deflation = -2;
		deflated_button.inflate(deflation, deflation);

		if (on || state->animation_state.percentage_position) {
			RECT cover_on = button;
			cover_on.left = r.left;
			urender::draw_round_rectangle(dc, cover_on, h / 2.f, bk_on_br);
		}

		urender::draw_ellipse(dc, deflated_button, brushes.fg);

		return 0;
	} break;
	case WM_DESTROY:
	{
		free(state);
	}break;
	case WM_MOUSEACTIVATE:
	{
		return MA_ACTIVATE; //Activate our window and post the mouse msg
	} break;
	case WM_ERASEBKGND:
	case WM_NCPAINT:
	case WM_NCCALCSIZE:
	{
		return 0;
	} break;
	case WM_SETCURSOR:
	case WM_WINDOWPOSCHANGED:
	case WM_WINDOWPOSCHANGING:
	case WM_SHOWWINDOW:
	case WM_MOVE:
	case WM_SETTEXT:
	case WM_STYLECHANGING:
	case WM_STYLECHANGED:
	case WM_SETFONT:
	case WM_GETFONT:
	case WM_GETICON:
	case WM_CREATE:
	case WM_NCDESTROY:
	case WM_GETTEXT:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	default:
	{
#ifdef _DEBUG
		Assert(0);
#else 
		return DefWindowProc(hwnd, msg, wparam, lparam);
#endif
	} break;
	}
	return 0;
}