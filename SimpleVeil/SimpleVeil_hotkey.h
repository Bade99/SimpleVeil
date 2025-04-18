#pragma once
#include <Windows.h>
#include <CommCtrl.h>
#include "unCap_Helpers.h"
#include "unCap_Reflection.h"
#include "windows_vk_mapper.h"

//USE WITH: unCap_wndclass_edit_oneline (this is needed to change text colors, otherwise it'd work with any edit control)
//USAGE: SetWindowSubclass(your hwnd, HotkeyProc, 0, (DWORD_PTR)calloc(1,sizeof(HotkeyProcState))); //NOTE: HotkeyProc will take care of releasing that memory
//NOTIFICATION: when the hotkey is triggered a msg will be sent to the parent wnd through WM_COMMAND, LOWORD(wparam)= msg number specified in hMenu param of CreateWindow/Ex, lparam=HWND of the control

//TODO(fran): make sure we dont accept IME
//TODO(fran): maybe it's just better to take the stuff from edit_oneline and put it with the hotkey control, mainly because of our need to be switching brushes, the other way would be that we store the normal text, hotkey accepted and hotkey rejected brushes and the edit control below has no text brush at all, only for disabled

//Extra scancode nfo https://handmade.network/wiki/2823-keyboard_inputs_-_scancodes,_raw_input,_text_input,_key_names

struct hotkey_nfo {
#define foreach_hotkey_nfo_member(op) \
	op(u16, hk_mod,0) \
	op(u8, hk_vk,0) \
	op(LPARAM, hk_trasn_nfo,0) \

	//hk_trasn_nfo: this is given to you in WM_KEYDOWN and similar msgs

	foreach_hotkey_nfo_member(_generate_member);

	_generate_default_struct_serialize(foreach_hotkey_nfo_member);

	_generate_default_struct_deserialize(foreach_hotkey_nfo_member);
};

_add_struct_to_serialization_namespace(hotkey_nfo)

struct HotkeyProcState {
	bool not_first_time;
	HWND wnd;
	HWND parent;
	UINT msg_to_send;
	struct { //brushes
		HBRUSH txt_hotkey_accepted;
		HBRUSH txt_hotkey_rejected;
		HBRUSH txt;
		HBRUSH default_text;
	} brushes;
	hotkey_nfo hk;
	hotkey_nfo* stored_hk;//On startup represents represents what was saved on serialization, after that it represents the currently valid/working hotkey
	cstr default_text[100];//TODO(fran): add msg to be able to change this
};

//NOTE: the caller takes care of deleting the brushes, we dont do it
void HOTKEY_set_brushes(HotkeyProcState* state, HBRUSH txt, HBRUSH default_text, HBRUSH txt_hotkey_accepted, HBRUSH txt_hotkey_rejected, HWND wnd = nullptr) {
	if (txt)state->brushes.txt = txt;
	if (default_text)state->brushes.default_text = default_text;
	if (txt_hotkey_accepted)state->brushes.txt_hotkey_accepted = txt_hotkey_accepted;
	if (txt_hotkey_rejected)state->brushes.txt_hotkey_rejected = txt_hotkey_rejected;
	AskForRepaint(wnd ? wnd : state->wnd);
}

void HOTKEY_set_txt_brush(HotkeyProcState* state, HBRUSH txt_br) {//Internal function
	EDITONELINE_set_brushes(state->wnd, TRUE, txt_br,0,0,0,0,0);
}

str HOTKEY_hk_to_str(u16 hk_mod, LPARAM lparam/*This is the one given to you by WM_KEYDOWN and the like*/, u8 vk/*if the string for the virtual key code cant be retrieved from lparam we attempt to get it from vk*/) {
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
	bool valid_vk = false;
	TCHAR vk_str[30];
	int keynameres = GetKeyNameText((LONG)lparam, vk_str, ARRAYSIZE(vk_str));//TODO(fran): modify bit 25 to make sure we never care about left or right keys

	//if (!res) {
	//	//TODO(fran): MapVirtualKeyEx has some extra crazy stuff that could be useful
	//	UINT scancode = MapVirtualKeyExW(vk, MAPVK_VK_TO_VSC_EX,GetKeyboardLayout(0));
	//	if (scancode) {
	//		res = GetKeyNameText(scancode<<16, vk_str, ARRAYSIZE(vk_str));
	//	}
	//}

	if (keynameres) {
		valid_vk = true;
		if(keynameres > 1) cstr_lwr(vk_str + 1, cstr_len(vk_str)); //TODO(fran): better lowercase functions, and work with locales
	}
	else {
		const cstr* stringed_vk = vkToString(vk);
		if (*stringed_vk) {
			valid_vk = true;
			size_t stringed_vk_len = cstr_len(stringed_vk)+1;
			memcpy_s(vk_str, sizeof(vk_str), stringed_vk, stringed_vk_len * sizeof(*stringed_vk));
		}
	}

	if (valid_vk) {
		if (!first_mod) hotkey_str += L"+";
		hotkey_str += vk_str;
	}
	return hotkey_str;
}

LRESULT CALLBACK HotkeyProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData) {
#define HOTKEY_TIMER 0xf1
	constexpr int timer_ms = 3000;
	
	HotkeyProcState* state = (HotkeyProcState*)dwRefData;
	Assert(state);
	if (!state->not_first_time) {
		state->not_first_time = !state->not_first_time;
		state->wnd = wnd;

		LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
		Assert(style & WS_CHILD);

		state->parent = GetParent(state->wnd);
		state->msg_to_send = (UINT)(UINT_PTR)GetMenu(state->wnd);
		str default_text = RS(LANG_HOTKEY_PLACEHOLDER);
		memcpy_s(state->default_text,sizeof(state->default_text), default_text.c_str(), (default_text.length()+1)*sizeof(*default_text.c_str()));
		if (state->stored_hk && state->stored_hk->hk_trasn_nfo && (state->stored_hk->hk_mod || state->stored_hk->hk_vk)) {//We saved a hotkey in a previous execution
			state->hk = *state->stored_hk;

			//TODO(fran): make this into a function, it's the same code from wm_keydown
			if (state->hk.hk_mod != MOD_NOREPEAT) {//We have valid modifiers
				BOOL res = RegisterHotKey(state->wnd, 0, state->hk.hk_mod, state->hk.hk_vk);
				if (res) {
					*state->stored_hk = state->hk;
					HOTKEY_set_txt_brush(state, state->brushes.txt_hotkey_accepted);
				}
				else {
					*state->stored_hk = hotkey_nfo{ 0 };
					HOTKEY_set_txt_brush(state, state->brushes.txt_hotkey_rejected);
				}
			}
			else {
				*state->stored_hk = hotkey_nfo{ 0 };
				HOTKEY_set_txt_brush(state, state->brushes.txt_hotkey_rejected);
			}
			SetTimer(state->wnd, HOTKEY_TIMER, timer_ms, NULL);
			str hk_str = HOTKEY_hk_to_str(state->hk.hk_mod, state->hk.hk_trasn_nfo,state->hk.hk_vk);
			SetWindowText(state->wnd, hk_str.c_str()); //BUG: edit control doesnt seem to be ready yet, cursor selection will be wrong
		}
		else {
			//PostMessage(wnd, WM_SETTEXT, 0, (LPARAM)state->default_text); //TODO(fran): cant use Post to fix the bug, msg gets lost, why?
			SetWindowText(wnd, state->default_text); //BUG: edit control doesnt seem to be ready yet, cursor selection will be wrong
			HOTKEY_set_txt_brush(state, state->brushes.default_text);
		}
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

		UnregisterHotKey(state->wnd, 0);//No matter what, if we got here we are gonna modify the hotkey, so we remove the previous one

		//INFO: wow, Alt Gr sends Ctrl + Alt, how crazy is that //TODO(fran): should I bother with that? http://vision.ime.usp.br/~cejnog/librealsense/examples/third_party/glfw/src/win32_window.c

		//NOTE: we ignore windows key, it's reserved for the system (VK_LWIN VK_RWIN), also mod keys by themselves(VK_CONTROL VK_SHIFT VK_MENU) and F12 which is reserved for use by debuggers
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
		case VK_F12:
		{
			str hk_str = HOTKEY_hk_to_str((ctrl_is_down ? MOD_CONTROL : 0) | (alt_is_down ? MOD_ALT : 0) | (shift_is_down ? MOD_SHIFT : 0), vk==VK_F12?lparam : 0, 0);
			SetWindowText(state->wnd, hk_str.c_str());
			*state->stored_hk = hotkey_nfo{ 0 };
			HOTKEY_set_txt_brush(state, state->brushes.txt_hotkey_rejected);
			SetTimer(state->wnd, HOTKEY_TIMER, timer_ms, NULL);
			return 0;
		} break;
		default://We have a valid vk
		{
			//We got a valid virtual key
			state->hk.hk_mod = (ctrl_is_down ? MOD_CONTROL : 0) | (alt_is_down ? MOD_ALT : 0) | (shift_is_down ? MOD_SHIFT : 0) | MOD_NOREPEAT;
			state->hk.hk_vk = vk;
			state->hk.hk_trasn_nfo = lparam;
			if (state->hk.hk_mod != MOD_NOREPEAT) {//We have valid modifiers
				BOOL res = RegisterHotKey(state->wnd, 0, state->hk.hk_mod, state->hk.hk_vk);
				if (res) {
					*state->stored_hk = state->hk;
					HOTKEY_set_txt_brush(state, state->brushes.txt_hotkey_accepted);
				}
				else {
					*state->stored_hk = hotkey_nfo{ 0 };
					HOTKEY_set_txt_brush(state, state->brushes.txt_hotkey_rejected);
				}
			}
			else {
				*state->stored_hk = hotkey_nfo{ 0 };
				HOTKEY_set_txt_brush(state, state->brushes.txt_hotkey_rejected);
			}
			SetTimer(state->wnd, HOTKEY_TIMER, timer_ms, NULL);
			str hk_str = HOTKEY_hk_to_str(state->hk.hk_mod, state->hk.hk_trasn_nfo, state->hk.hk_vk);
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
	case WM_HOTKEY:
	{
		PostMessage(state->parent, WM_COMMAND, (WPARAM)MAKELONG(state->msg_to_send, 0), (LPARAM)state->wnd);//notify parent
	} break;
	case WM_TIMER:
	{
		WPARAM timerID = wparam;
		switch (timerID) {
		case HOTKEY_TIMER:
		{
			KillTimer(state->wnd, timerID);
			HOTKEY_set_txt_brush(state, state->brushes.txt);
			return 0;
		} break;
		default: return DefSubclassProc(wnd, msg, wparam, lparam);
		}
	} break;
	case WM_DESTROY://TODO(fran): look for others, also what if subclassing is removed
	{
		UnregisterHotKey(state->wnd, 0);
		free(state);
	} break;
	default: return DefSubclassProc(wnd, msg, wparam, lparam);
	}
	return 0;
}