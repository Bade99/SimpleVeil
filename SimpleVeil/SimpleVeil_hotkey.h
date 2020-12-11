#pragma once
#include <Windows.h>
#include <CommCtrl.h>
#include "unCap_Helpers.h"
#include "unCap_Reflection.h"

//USE WITH: any type of edit control, but preferably unCap_wndclass_edit_oneline
//USAGE: SetWindowSubclass(your hwnd, HotkeyProc, 0, (DWORD_PTR)calloc(1,sizeof(HotkeyProcState))); //NOTE: HotkeyProc will take care of releasing that memory

//TODO(fran): make sure we dont accept IME

struct HotkeyProcState {
	bool not_first_time;
	HWND wnd;
	struct {//hotkey
		u16 hk_mod;
		u8 hk_vk;
	};
	cstr default_text[100];
};

str HOTKEY_hk_to_str(u16 hk_mod, LPARAM lparam/*This is the one given to you by WM_KEYDOWN and the like*/) {
	str hotkey_str=_t("");
	bool first_mod = true;
	if (hk_mod & MOD_CONTROL) {
		if (!first_mod) hotkey_str += L"+";
		hotkey_str += L"Ctrl";
		first_mod = false;
	}
	if (hk_mod & MOD_ALT) {
		if (!first_mod) hotkey_str += L"+";
		hotkey_str += L"Alt";
		first_mod = false;
	}
	if (hk_mod & MOD_SHIFT) {
		if (!first_mod) hotkey_str += L"+";
		hotkey_str += L"Shift";
		first_mod = false;
	}
	TCHAR vk_str[20];
	int res = GetKeyNameText((LONG)lparam, vk_str, ARRAYSIZE(vk_str));//TODO(fran): modify bit 25 to make sure we never care about left or right keys
	if (res) {
		if(res > 1) cstr_lwr(vk_str + 1, cstr_len(vk_str)); //TODO(fran): better lowercase functions, and work with locales
		if (!first_mod) hotkey_str += L"+";
		hotkey_str += vk_str;
	}
	return hotkey_str;
}

LRESULT CALLBACK HotkeyProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData) {
	HotkeyProcState* state = (HotkeyProcState*)dwRefData;
	Assert(state);
	if (!state->not_first_time) {
		state->not_first_time = !state->not_first_time;
		state->wnd = wnd;
		cstr default_text[] = _t("Choose a hotkey");
		memcpy(state->default_text, default_text, sizeof(default_text));//TODO(fran): check this is correct
		SetWindowText(wnd, state->default_text);
		//TODO(fran): flip text and text_disabled brush
	}
	switch (msg) {
	//TODO(fran): should I stop keyup msgs?
	//TODO(fran): I cant press multiple modifiers, eg ctrl+shift+alt, or at least the display isnt able to show it
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN://When the user presses a non-system key this is the 1st msg we receive
	{
		//TODO(fran): here we process things like VK_HOME VK_NEXT VK_LEFT VK_RIGHT VK_DELETE
		//NOTE: there are some keys that dont get sent to WM_CHAR, we gotta handle them here, also there are others that get sent to both, TODO(fran): it'd be nice to have all the key things in only one place
		//NOTE: for things like _shortcuts_ you wanna handle them here cause on WM_CHAR things like Ctrl+V get translated to something else
		//		also you want to use the uppercase version of the letter, eg case _t('V'):
		char vk = (char)wparam;
		bool ctrl_is_down = HIBYTE(GetKeyState(VK_CONTROL));
		bool alt_is_down = HIBYTE(GetKeyState(VK_MENU));
		bool shift_is_down = HIBYTE(GetKeyState(VK_SHIFT));
		//TODO(fran): ignore windows key, it's reserved for the system (VK_LWIN VK_RWIN), also I dont think we should accept just mod keys (VK_CONTROL VK_SHIFT VK_MENU)
		switch (vk) {
		case VK_LWIN:
		case VK_RWIN:
		case VK_CONTROL:
		case VK_LCONTROL:
		case VK_RCONTROL:
		case VK_SHIFT:
		case VK_LSHIFT:
		case VK_RSHIFT:
		case VK_MENU:
		{
			str hk_str = HOTKEY_hk_to_str(ctrl_is_down ? MOD_CONTROL : 0 | alt_is_down ? MOD_ALT : 0 | shift_is_down ? MOD_SHIFT : 0, 0);
			SetWindowText(state->wnd, hk_str.c_str());
			return 0;
		} break;
		default://We have a valid vk
		{
			//We got a valid virtual key
			state->hk_mod = ctrl_is_down ? MOD_CONTROL : 0 | alt_is_down ? MOD_ALT : 0 | shift_is_down ? MOD_SHIFT : 0 | MOD_NOREPEAT;
			state->hk_vk = vk;
			if (state->hk_mod != MOD_NOREPEAT) {//We have valid modifiers
				UnregisterHotKey(state->wnd, 0);
				BOOL res = RegisterHotKey(state->wnd, 0, state->hk_mod, state->hk_vk);
				if (res) {
					//TODO(fran): change text brush to green and store previous text brush
				}
				else {
					//TODO(fran): change text brush to red and store previous text brush
				}
			}
			str hk_str = HOTKEY_hk_to_str(state->hk_mod, lparam);
			SetWindowText(state->wnd, hk_str.c_str());
			return 0;
		} break;
		}
		return 0;
	}break;
	case WM_CHAR://When the user presses a non-system key this is the 2nd msg we receive
	{//NOTE: a WM_KEYDOWN msg was translated by TranslateMessage() into WM_CHAR
		return 0;
	} break;
	case WM_SYSCHAR:
	{
		return 0;
	} break;
	case WM_DESTROY://TODO(fran): look for others, also what if subclassing is removed
	{
		free(state);
	} break;
	default: return DefSubclassProc(wnd, msg, wparam, lparam);
	}
	return 0;
}