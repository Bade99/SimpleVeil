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

constexpr TCHAR appName[] = _t("SimpleVeil"); //Program name, to be displayed on the title of windows

struct unCapClSettings {

#define foreach_unCapClSettings_member(op) \
		op(RECT, rc,200,200,700,600 ) \
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
			HWND button_on_off;
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

	int btn_onoff_w = 70;
	int btn_onoff_h = 30;
	int btn_onoff_x = w - w_pad - btn_onoff_w;
	int btn_onoff_y = (h - btn_onoff_h) / 2;

	int slider_brightness_x = w_pad;
	int slider_brightness_h = btn_onoff_h;
	int slider_brightness_y = (h - slider_brightness_h) / 2;
	int slider_brightness_w = max(btn_onoff_x - w_pad * 2, 0);

	int hotkey_w = w / 2;
	int hotkey_x = (w - hotkey_w) / 2;
	int hotkey_h = btn_onoff_h;
	int hotkey_y = h - hotkey_h - h_pad;

	//TODO(fran): resizer that takes into account multiple hwnds
	MoveWindow(state->controls.slider_brightness, slider_brightness_x, slider_brightness_y, slider_brightness_w, slider_brightness_h, FALSE);
	MoveWindow(state->controls.button_on_off, btn_onoff_x, btn_onoff_y, btn_onoff_w, btn_onoff_h, FALSE);
	MoveWindow(state->controls.edit_hotkey, hotkey_x, hotkey_y, hotkey_w, hotkey_h, FALSE);
}

void SetText_file_app(HWND wnd, const TCHAR* new_filename, const TCHAR* new_appname) {
	if (new_filename == NULL || *new_filename == NULL) {
		SetWindowTextW(wnd, new_appname);
	}
	else {
		std::wstring title_window = new_filename;
		title_window += L" - ";
		title_window += new_appname;
		SetWindowTextW(wnd, title_window.c_str());
	}
}

void UNCAPCL_add_controls(unCapClProcState* state, HINSTANCE hInstance) {

	state->controls.slider_brightness = CreateWindowExW(0, TRACKBAR_CLASS, 0, WS_CHILD | WS_VISIBLE | TBS_NOTICKS
		, 0, 0, 0, 0, state->wnd, (HMENU)SLIDER_BRIGHTNESS, NULL, NULL);

	SendMessage(state->controls.slider_brightness, TBM_SETRANGE, TRUE, (LPARAM)MAKELONG(0, brightness_slider_max));
	SendMessage(state->controls.slider_brightness, TBM_SETPAGESIZE, 0, (LPARAM)5);
	SendMessage(state->controls.slider_brightness, TBM_SETLINESIZE, 0, (LPARAM)5);
	SendMessage(state->controls.slider_brightness, TBM_SETPOS, (WPARAM)TRUE, (LPARAM)state->settings->slider_brightness_pos);

	state->controls.button_on_off = CreateWindowW(unCap_wndclass_button, NULL, WS_VISIBLE | WS_CHILD | WS_TABSTOP
		, 0, 0, 0, 0, state->wnd, (HMENU)BUTTON_ONOFF, NULL, NULL);
	//AWT(state->controls.button_removecomment, LANG_CONTROL_REMOVE);
	UNCAPBTN_set_brushes(state->controls.button_on_off, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);

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
	//std::wstring info_save_file = GetInfoPath();

	RECT rc; GetWindowRect(state->wnd, &rc);// MapWindowPoints(state->wnd, HWND_DESKTOP, (LPPOINT)&rc, 2); //TODO(fran): can I simply use GetWindowRect?

	state->settings->rc = rc;
	//NOTE: the rest of settings are already being updated, and this one should too
}

unCapClProcState* UNCAPCL_get_state(HWND uncapcl) {
	unCapClProcState* state = (unCapClProcState*)GetWindowLongPtr(uncapcl, 0);
	return state;
}

void SIMPLEVEIL_update_veil_wnd(unCapClProcState* state) {
	if (state->settings->is_veil_on) {
		ShowWindow(state->settings->veil_wnd, SW_MAXIMIZE);
		SetLayeredWindowAttributes(state->settings->veil_wnd, 0, (BYTE)(((float)state->settings->slider_brightness_pos / 100.f) * 255.f), LWA_ALPHA);
		BringWindowToTop(state->nc_parent);
	}
	else {
		ShowWindow(state->settings->veil_wnd, SW_HIDE);
	}
}

void SIMPLEVEIL_update_btn_onoff_text(unCapClProcState* state) {
	if (state->settings->is_veil_on) {
		SetWindowText(state->controls.button_on_off, RCS(LANG_BTN_TURN_OFF));
	}
	else {
		SetWindowText(state->controls.button_on_off, RCS(LANG_BTN_TURN_ON));
	}
}

void SIMPLEVEIL_restore_wnd(unCapClProcState* state) {
	if (!IsWindowVisible(state->nc_parent))
		RestoreWndFromTray(state->nc_parent);
	else
		ShowWindow(state->nc_parent, SW_SHOW);
}

LRESULT CALLBACK UncapClProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	unCapClProcState* state = UNCAPCL_get_state(hwnd);
	switch (msg) {
	case WM_DRAWITEM:
	{
		DRAWITEMSTRUCT* item = (DRAWITEMSTRUCT*)lparam;
		switch (wparam) {//wParam specifies the identifier of the control that needs painting
		default: return DefWindowProc(hwnd, msg, wparam, lparam);
		}
		return 0;
	} break;
	case WM_CTLCOLORLISTBOX:
	{
		HDC comboboxDC = (HDC)wparam;
		SetBkColor(comboboxDC, ColorFromBrush(unCap_colors.ControlBk));
		SetTextColor(comboboxDC, ColorFromBrush(unCap_colors.ControlTxt));

		return (INT_PTR)unCap_colors.ControlBk;
	}
	case WM_CTLCOLOREDIT://TODO(fran): paint my own edit box so I can add a 1px border, OR paint it here, I do get the dc, and apply a clip region so no one overrides it
	{//NOTE: there doesnt seem to be a way to modify the border that I added with WS_BORDER, it probably is handled outside of the control
		/*HWND ctl = (HWND)lparam;
		HDC dc = (HDC)wparam;
		if (ctl == hInitialChar || ctl == hFinalChar)
		{
			SetBkColor(dc, ColorFromBrush(unCap_colors.ControlBk));
			SetTextColor(dc, ColorFromBrush(unCap_colors.ControlTxt));
			return (LRESULT)unCap_colors.ControlBk;
		}
		else return DefWindowProc(hwnd, msg, wparam, lparam);*/
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
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

		SetText_file_app(state->nc_parent, nullptr, appName);

		state->settings->is_veil_on = true;

		SIMPLEVEIL_update_veil_wnd(state);
		SIMPLEVEIL_update_btn_onoff_text(state);

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
			break;
		}
		case WM_RBUTTONDOWN:
		{
			SIMPLEVEIL_restore_wnd(state);

			break;
		}
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
			SIMPLEVEIL_restore_wnd(state);
			return 0;
		} break;
		case BUTTON_ONOFF:
		{
			state->settings->is_veil_on = !state->settings->is_veil_on;

			SIMPLEVEIL_update_veil_wnd(state);
			SIMPLEVEIL_update_btn_onoff_text(state);
			
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
	case WM_NCCALCSIZE:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NOTIFYFORMAT:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_QUERYUISTATE: //Neat, I dont know who asks for this but it's interesting, you can tell whether you need to draw keyboards accels
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_MOVE:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_SHOWWINDOW:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_WINDOWPOSCHANGING:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_WINDOWPOSCHANGED:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCPAINT:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_ERASEBKGND:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);//TODO(fran): or return 1, idk
	} break;
	case WM_PAINT:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_NCHITTEST:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_SETCURSOR:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_MOUSEMOVE:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_MOUSEACTIVATE:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_LBUTTONDOWN:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_LBUTTONUP:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_RBUTTONDOWN:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_RBUTTONUP:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_CONTEXTMENU://Interesting
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_DESTROY:
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_DELETEITEM://sent to owner of combo/list box (for menus also) when it is destroyed or items are being removed. when you do SetMenu you also get this msg if you had a menu previously attached
		//TODO(fran): we could try to use this to manage state destruction
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_GETTEXT://we should return NULL
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	case WM_PARENTNOTIFY://TODO(fran): see what we're getting
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}break;
	case WM_NOTIFY://TODO(fran): see what we're getting
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	}break;
	case WM_APPCOMMAND://This is triggered, for example, when the user presses one of the media keys (next track, prev, ...) in their keyboard
	{
		return DefWindowProc(hwnd, msg, wparam, lparam);
	} break;
	default:
#ifdef _DEBUG
		Assert(0);
#else 
		return DefWindowProc(hwnd, msg, wparam, lparam);
#endif
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
	wcex.lpszMenuName = 0;// MAKEINTRESOURCEW(IDC_SRTFIXWIN32);//TODO(fran): remove?
	wcex.lpszClassName = unCap_wndclass_uncap_cl;
	wcex.hIconSm = 0;

	ATOM class_atom = RegisterClassExW(&wcex);
	Assert(class_atom);
	return class_atom;
}