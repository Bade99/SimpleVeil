#pragma once
#include <Windows.h>
#include "unCap_Reflection.h"
#include "unCap_Serialization.h"
#include "LANGUAGE_MANAGER.h"
#include "TRAY_HANDLER.h"
#include "fmt.h"
#include "resource.h"
#include "unCap_uncapnc.h"
#include "SimpleVeil_hotkey.h"
#include "SIMPLEVEIL_trackbar.h"
#include <filesystem>
#include <fstream>
#include <propvarutil.h>
#include <propkey.h>

#define BUTTON_ONOFF 14
#define SLIDER_BRIGHTNESS 15
#define TRAY 16
#define EDIT_HOTKEY 17

//TODO(fran): check that the veil remains on top, maybe update from time to time, or find some trigger when someone else goes on top

constexpr int y_pad = 10;

constexpr int brightness_slider_max = 90;

#define APP_NAME "SimpleVeil"
#define APP_VERSION "1.0.1"

constexpr TCHAR appName[] = TEXT(APP_NAME);

constexpr TCHAR appVersion[] = TEXT(APP_VERSION);

struct unCapClSettings {

#define foreach_unCapClSettings_member(op) \
		op(RECT, rc,200,200,700,482 ) \
		op(int, slider_brightness_pos,10 ) \
		op(hotkey_nfo, hotkey,0,0,0 ) \

		HWND veil_wnd;
		bool is_veil_on;

	foreach_unCapClSettings_member(_generate_member);

	_generate_default_struct_serialize(foreach_unCapClSettings_member);

	_generate_default_struct_deserialize(foreach_unCapClSettings_member);

};

_add_struct_to_serialization_namespace(unCapClSettings)

struct unCapClProcState {
	HWND wnd;
	HWND nc_parent;

	struct {//menu related
		HMENU menu;
		HMENU menu_file;
		HMENU menu_lang;
	};
	union unCapClControls {
		struct {
			HWND toggle_button_on_off;
			HWND slider_brightness;
			HWND edit_hotkey;
		};
		HWND all[3];//REMEMBER TO UPDATE
	}controls;
	unCapClSettings* settings;

};

TCHAR unCap_wndclass_uncap_cl[] = TEXT("unCap_wndclass_uncap_cl"); //Client uncap

void UNCAPCL_ResizeWindows(unCapClProcState* state) {
	RECT r; GetClientRect(state->wnd, &r);
	int w = RECTWIDTH(r);
	int h = RECTHEIGHT(r);
	int w_pad = (int)((float)w * .05f);
	int h_pad = (int)((float)h * .05f);

	int toggle_btn_onoff_w = 70;
	int toggle_btn_onoff_h = 30;
	int toggle_btn_onoff_x = w - w_pad - toggle_btn_onoff_w;
	int toggle_btn_onoff_y = (h - toggle_btn_onoff_h) / 2;

	int slider_brightness_x = w_pad;
	int slider_brightness_h = toggle_btn_onoff_h;
	int slider_brightness_y = (h - slider_brightness_h) / 2;
	int slider_brightness_w = max(toggle_btn_onoff_x - w_pad * 2, 0);

	int hotkey_w = w / 2;
	int hotkey_x = (w - hotkey_w) / 2;
	int hotkey_h = toggle_btn_onoff_h;
	int hotkey_y = h - hotkey_h - h_pad;

	MoveWindow(state->controls.slider_brightness, slider_brightness_x, slider_brightness_y, slider_brightness_w, slider_brightness_h, FALSE);
	MoveWindow(state->controls.toggle_button_on_off, toggle_btn_onoff_x, toggle_btn_onoff_y, toggle_btn_onoff_w, toggle_btn_onoff_h, FALSE);
	MoveWindow(state->controls.edit_hotkey, hotkey_x, hotkey_y, hotkey_w, hotkey_h, FALSE);
}

void SetText_app_name(HWND wnd, const TCHAR* new_appname) {
	SetWindowTextW(wnd, new_appname);
}

void UNCAPCL_add_controls(unCapClProcState* state, HINSTANCE hInstance) {
	state->controls.slider_brightness = CreateWindowExW(0, TRACKBAR_CLASS, 0, WS_CHILD | WS_VISIBLE | TBS_NOTICKS | TBS_HORZ
		, 0, 0, 0, 0, state->wnd, (HMENU)SLIDER_BRIGHTNESS, NULL, NULL);

	SendMessage(state->controls.slider_brightness, TBM_SETRANGE, TRUE, (LPARAM)MAKELONG(0, brightness_slider_max));
	SendMessage(state->controls.slider_brightness, TBM_SETPAGESIZE, 0, (LPARAM)5);
	SendMessage(state->controls.slider_brightness, TBM_SETLINESIZE, 0, (LPARAM)5);
	SendMessage(state->controls.slider_brightness, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)state->settings->slider_brightness_pos);

	SetWindowSubclass(state->controls.slider_brightness, TrackbarProc, 0, 0);

	state->controls.toggle_button_on_off = CreateWindowExW(WS_EX_COMPOSITED | WS_EX_TRANSPARENT, unCap_wndclass_toggle_button, NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP
		, 0, 0, 0, 0, state->wnd, (HMENU)BUTTON_ONOFF, NULL, NULL);
	UNCAPTGLBTN_set_brushes(state->controls.toggle_button_on_off, TRUE, unCap_colors.Img, unCap_colors.Img, unCap_colors.ToggleBk_On, unCap_colors.ToggleBk_Off, unCap_colors.ToggleBkPush_On, unCap_colors.ToggleBkPush_Off, unCap_colors.ToggleBkMouseOver_On, unCap_colors.ToggleBkMouseOver_Off);
	UNCAPTGLBTN_set_state_reference(state->controls.toggle_button_on_off, &state->settings->is_veil_on);

	state->controls.edit_hotkey = CreateWindowW(unCap_wndclass_edit_oneline, L"", WS_VISIBLE | WS_CHILD | ES_CENTER
		, 0,0,0,0, state->wnd, (HMENU)EDIT_HOTKEY, NULL, NULL);
	EDITONELINE_set_brushes(state->controls.edit_hotkey, TRUE, unCap_colors.ControlTxt, unCap_colors.ControlBk, unCap_colors.Img, unCap_colors.ControlTxt_Disabled, unCap_colors.ControlBk_Disabled, unCap_colors.Img_Disabled);
	HotkeyProcState* hotkeyprocstate = (HotkeyProcState*)calloc(1, sizeof(HotkeyProcState));
	SetWindowSubclass(state->controls.edit_hotkey, HotkeyProc, 0, (DWORD_PTR)hotkeyprocstate);
	HOTKEY_set_brushes(hotkeyprocstate, unCap_colors.ControlTxt, unCap_colors.ControlTxt_Disabled, unCap_colors.HotkeyTxt_Accepted, unCap_colors.HotkeyTxt_Rejected);
	hotkeyprocstate->stored_hk = &state->settings->hotkey;
	
	for (auto ctl : state->controls.all)
		SendMessage(ctl, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);

	UNCAPCL_ResizeWindows(state);
}

void UNCAPCL_save_settings(unCapClProcState* state) {
	RECT rc; GetWindowRect(state->wnd, &rc);
	state->settings->rc = rc;
}

unCapClProcState* UNCAPCL_get_state(HWND uncapcl) {
	unCapClProcState* state = (unCapClProcState*)GetWindowLongPtr(uncapcl, 0);
	return state;
}

RECT GetVirtualScreenRect() {
	RECT rcCombined{ 0,0,0,0 };
	EnumDisplayMonitors(NULL, NULL, 
		[](HMONITOR, HDC, LPRECT lprcMonitor, LPARAM pData) ->BOOL {
			RECT* rcCombined = (RECT*)pData;
			UnionRect(rcCombined, rcCombined, lprcMonitor);
			return TRUE;
		}
	, (LPARAM)&rcCombined);
	return rcCombined;
	//NOTE: other way could be with GetSystemMetrics
}

void SIMPLEVEIL_resize_veil_cover_all_monitors(unCapClProcState* state) { //Simulates the window being maximized in all monitors
	//NOTE: you could also change it in WM_GETMINMAXINFO https://stackoverflow.com/questions/64882357/windows-api-maximize-a-window-across-all-monitors
#if 0
	RECT virtual_screen = GetVirtualScreenRect();

	SetWindowPos(state->settings->veil_wnd, NULL, virtual_screen.left, virtual_screen.top, RECTWIDTH(virtual_screen), RECTHEIGHT(virtual_screen), SWP_NOACTIVATE | SWP_NOZORDER);
#else
	int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
	int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
	int w = GetSystemMetrics(SM_CXVIRTUALSCREEN);
	int h = GetSystemMetrics(SM_CYVIRTUALSCREEN);
	SetWindowPos(state->settings->veil_wnd, NULL, x,y,w,h, SWP_NOACTIVATE | SWP_NOZORDER);
#endif
}

void SIMPLEVEIL_update_veil_wnd(unCapClProcState* state) {
	if (state->settings->is_veil_on) {
		ShowWindow(state->settings->veil_wnd, SW_SHOW);
		SetLayeredWindowAttributes(state->settings->veil_wnd, 0, (BYTE)(((float)state->settings->slider_brightness_pos / 100.f) * 255.f), LWA_ALPHA);
		BringWindowToTop(state->nc_parent);
	}
	else {
		ShowWindow(state->settings->veil_wnd, SW_HIDE);
	}
}

void SIMPLEVEIL_restore_wnd(unCapClProcState* state) {
	if (!IsWindowVisible(state->nc_parent))
		RestoreWndFromTray(state->nc_parent);
	else
		ShowWindow(state->nc_parent, SW_SHOW);
}

void SIMPLEVEIL_toggle_wnd_visibility(unCapClProcState* state) {
	if (!IsWindowVisible(state->nc_parent)) //window is minimized
		RestoreWndFromTray(state->nc_parent);//TODO(fran): the veil could be occluded, we should check that the veil is on top too
	else //window is not minimized //TODO(fran): _but_ could be occluded (in which case we want to SW_SHOW), there doesnt seem to be an easy way to know whether your window is actually visible to the user
		MinimizeWndToTray(state->nc_parent);
}

LRESULT CALLBACK UncapClProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	//printf(msgToString(msg)); printf("\n");
	unCapClProcState* state = UNCAPCL_get_state(hwnd);
	switch (msg) {
	case WM_CTLCOLORLISTBOX:
	{
		HDC comboboxDC = (HDC)wparam;
		SetBkColor(comboboxDC, ColorFromBrush(unCap_colors.ControlBk));
		SetTextColor(comboboxDC, ColorFromBrush(unCap_colors.ControlTxt));

		return (INT_PTR)unCap_colors.ControlBk;
	}
	case WM_CTLCOLORSTATIC:
	{
		//TODO(fran): check we are changing the right controls
		HWND static_wnd = (HWND)lparam;
		HDC dc = (HDC)wparam;
		switch (GetDlgCtrlID(static_wnd)) {
			default://NOTE: this is for state->controls.static_removecomment
			{
				//TODO(fran): there's something wrong with Initial & Final character controls when they are disabled, documentation says windows always uses same color text when window is disabled but it looks to me that it is using both this and its own color
				SetBkColor(dc, ColorFromBrush(unCap_colors.ControlBk));
				SetTextColor(dc, ColorFromBrush(unCap_colors.ControlTxt));
				return (LRESULT)unCap_colors.ControlBk;
			}
		}

		return DefWindowProc(hwnd, msg, wparam, lparam);
	}
	case WM_CREATE:
	{
		CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;

		UNCAPCL_add_controls(state, (HINSTANCE)GetWindowLongPtr(state->wnd, GWLP_HINSTANCE));

		SetText_app_name(state->nc_parent, appName);

		state->settings->is_veil_on = true;

		SIMPLEVEIL_resize_veil_cover_all_monitors(state);
		SIMPLEVEIL_update_veil_wnd(state);

		BOOL tray_res = TRAY_HANDLER::Instance().CreateTrayIcon(state->wnd, 1, UNCAP_ICO_LOGO, TRAY, LANG_TRAY_TIP);
		Assert(tray_res);

		return 0;
	} break;
	case TRAY: //Tray icon handling
	{
		switch (lparam) {
		case WM_LBUTTONDOWN:
		{
			SendMessage(state->wnd, WM_COMMAND, MAKELONG(BUTTON_ONOFF, 0), 0);
		} break;
		case WM_RBUTTONDOWN:
		{
			//SIMPLEVEIL_restore_wnd(state);
			SIMPLEVEIL_toggle_wnd_visibility(state);
		} break;
		}
		break;
	}
	case WM_SIZE:
	{
		LRESULT res = DefWindowProc(hwnd, msg, wparam, lparam);
		UNCAPCL_ResizeWindows(state);
		return res;
	} break;
	case WM_NCDESTROY:
	{
		UNCAPCL_save_settings(state);
		TRAY_HANDLER::Instance().DestroyTrayIcon(state->wnd, 1);
		if (state) {
			free(state);
			state = nullptr;
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCCREATE:
	{
		CREATESTRUCT* create_nfo = (CREATESTRUCT*)lparam;
		unCapClProcState* st = (unCapClProcState*)calloc(1, sizeof(unCapClProcState));
		Assert(st);
		SetWindowLongPtr(hwnd, 0, (LONG_PTR)st);
		st->wnd = hwnd;
		st->nc_parent = create_nfo->hwndParent;
		st->settings = (unCapClSettings*)create_nfo->lpCreateParams;
		//UNCAPCL_load_settings(st);

		return TRUE;
	} break;
	case WM_HSCROLL: //Manage Slider
	case WM_VSCROLL:
	//INFO: horizontal sliders send WM_HSCROLL, vertical ones send WM_VSCROLL
	{
		switch (LOWORD(wparam)) {
		case TB_PAGEDOWN:
		case TB_PAGEUP:
		case TB_ENDTRACK:
		case TB_LINEDOWN:
		case TB_LINEUP:
		case TB_BOTTOM:
		case TB_THUMBPOSITION:
		case TB_THUMBTRACK:
		case TB_TOP:
		{
			HWND slider = (HWND)lparam;
			int pos = (int)SendMessage(slider, TBM_GETPOS, 0, 0);
			if (GetDlgCtrlID(slider) == SLIDER_BRIGHTNESS) {
				state->settings->slider_brightness_pos = pos;
				SIMPLEVEIL_update_veil_wnd(state);
			}
		} break;
		default: return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		break;
	}
	case WM_COMMAND:
	{
		// Msgs from my controls
		switch (LOWORD(wparam))
		{
		case EDIT_HOTKEY:
		{
			//SIMPLEVEIL_restore_wnd(state);
			SIMPLEVEIL_toggle_wnd_visibility(state);
			return 0;
		} break;
		case BUTTON_ONOFF:
		{
			state->settings->is_veil_on = !state->settings->is_veil_on;

			SIMPLEVEIL_update_veil_wnd(state);
		} break;
		case UNCAPNC_MINIMIZE://NOTE: msg sent from the parent, expects return !=0 in case we handle it
		{
			if (IsWindowVisible(state->nc_parent))
				MinimizeWndToTray(state->nc_parent);
			else
				ShowWindow(state->nc_parent, SW_MINIMIZE);
			return 1;
		} break;
#define _generate_enum_cases(member,value_expr) case LANGUAGE::member:
		_foreach_language(_generate_enum_cases)
		{
			LANGUAGE_MANAGER::Instance().ChangeLanguage((LANGUAGE)LOWORD(wparam));

			return 0;
		}
		}
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_DISPLAYCHANGE://Sent by our nc parent (otherwise we would not be able to get this msg)
	{//see https://docs.microsoft.com/en-us/windows/win32/gdi/hmonitor-and-the-device-context
		//SIMPLEVEIL_update_veil_wnd(state);
		SIMPLEVEIL_resize_veil_cover_all_monitors(state);
		//PostMessage(state->wnd, WM_COMMAND, MAKELONG(BUTTON_ONOFF, 0), 0);//HACK: for some reason we cant simply SIMPLEVEIL_update_veil_wnd, I guess windows' code hasnt updated yet, so we need to offset the update in time, that why I do two PostMessage, so we go back to our original state but with the correct new size for the veil
		//PostMessage(state->wnd, WM_COMMAND, MAKELONG(BUTTON_ONOFF, 0), 0);
		return 0;
	} break;
	case WM_CTLCOLOREDIT:
	case WM_ERASEBKGND: //TODO(fran): or return 1, idk
	case WM_DRAWITEM:
	case WM_NCCALCSIZE:
	case WM_NOTIFYFORMAT:
	case WM_QUERYUISTATE: //Neat, I dont know who asks for this but it's interesting, you can tell whether you need to draw keyboards accels
	case WM_MOVE:
	case WM_SHOWWINDOW:
	case WM_WINDOWPOSCHANGING:
	case WM_WINDOWPOSCHANGED:
	case WM_NCPAINT:
	case WM_PAINT:
	case WM_NCHITTEST:
	case WM_SETCURSOR:
	case WM_MOUSEMOVE:
	case WM_MOUSEACTIVATE:
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_CONTEXTMENU:
	case WM_DESTROY:
	case WM_DELETEITEM:
	case WM_GETTEXT:
	case WM_PARENTNOTIFY:
	case WM_NOTIFY:
	case WM_APPCOMMAND:
	case WM_MOUSEWHEEL:
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

ATOM init_wndclass_unCap_uncapcl(HINSTANCE inst) {
	WNDCLASSEXW wcex{ sizeof(wcex) };
	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = UncapClProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = sizeof(unCapClProcState*);
	wcex.hInstance = inst;
	wcex.hIcon = 0;
	wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wcex.hbrBackground = unCap_colors.ControlBk;
	wcex.lpszMenuName = 0;
	wcex.lpszClassName = unCap_wndclass_uncap_cl;
	wcex.hIconSm = 0;

	ATOM class_atom = RegisterClassExW(&wcex);
	Assert(class_atom);
	return class_atom;
}