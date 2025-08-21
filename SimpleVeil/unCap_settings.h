#pragma once
#include <Windows.h>
#include "resource.h"
#include <Richedit.h>
#include <ctime>
#include "unCap_combo_subclass.h"

namespace settings {
	constexpr int BUTTON_CANCEL = 23;
	constexpr int BUTTON_SAVE = 24;
	constexpr int COMBO_UPDATE_FREQUENCY= 25;

	struct ProcState {
		HWND wnd;
		HWND nc_parent;

		union Controls {
			struct {
				HWND text_update_check_frequency;
				HWND combo_update_check_frequency;
				HWND text_last_update_check;
				HWND button_save;
				HWND button_cancel;
			};
			HWND all[5];
			void _() { static_assert(sizeof(*this) == sizeof(this->all), "Update 'all[x]' to the correct number of HWNDs"); }
		}controls;
		unCapClSettings* settings;
		unCapClSettings edit_settings;
	};

	utf16 wndclass[] = L"unCap_wndclass_settings";

	void resize_controls(ProcState* state) {
		RECT r; GetClientRect(state->wnd, &r);
		int w = RECTW(r);
		int h = RECTH(r);
		f32 w_pad = w * .05f;
		f32 h_pad = h * .05f;

		i32 small_wnd_h = 30;
		i32 small_wnd_w = 90;
		i32 medium_wnd_w = small_wnd_w * 1.25f;

		i32 fixed_w_pad = small_wnd_h / 6;
		i32 fixed_h_pad = fixed_w_pad;

		rect_f32 page = rect_f32::from_RECT(r);
		page.inflate(-2*fixed_w_pad, -2*fixed_h_pad);
		
		auto text_update_check_frequency = page.cut_top(small_wnd_h);
		auto combo_update_check_frequency = text_update_check_frequency.cut_right(medium_wnd_w);
		page.cut_top(fixed_h_pad);
		auto text_last_update_check = page.cut_top(small_wnd_h);
		auto buttonbar = page.cut_bottom(small_wnd_h);
		auto btn_yes = buttonbar.cut_right(small_wnd_w);
		buttonbar.cut_right(fixed_w_pad);
		auto btn_no = buttonbar.cut_right(small_wnd_w);

		auto& controls = state->controls;
		MoveWindow(controls.text_update_check_frequency, text_update_check_frequency);
		MoveWindow(controls.combo_update_check_frequency, combo_update_check_frequency);
		MoveWindow(controls.text_last_update_check, text_last_update_check);
		MoveWindow(controls.button_cancel, btn_no);
		MoveWindow(controls.button_save, btn_yes);
	}

	void set_title(ProcState* state, const TCHAR* title) {
		SetWindowTextW(state->nc_parent, title);
	}

	void add_controls(ProcState* state) {
		auto& controls = state->controls;

		controls.text_update_check_frequency = CreateWindowExW(
			WS_EX_COMPOSITED | WS_EX_TRANSPARENT,
			L"Static", L"Check for updates on startup: ",
			WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE | SS_ENDELLIPSIS
			, 0, 0, 0, 0, state->wnd, nullptr, nullptr, nullptr);

		controls.combo_update_check_frequency = CreateWindowExW(
			WS_EX_COMPOSITED | WS_EX_TRANSPARENT, 
			L"ComboBox", NULL,
			WS_VISIBLE | WS_CHILD | CBS_DROPDOWNLIST | WS_TABSTOP | CBS_ROUNDRECT
			, 0, 0, 0, 0, state->wnd, (HMENU)COMBO_UPDATE_FREQUENCY, nullptr, nullptr);
#define _add_combo_item_enum(member,value) ComboBox_AddString(controls.combo_update_check_frequency, _t(#member));
		_foreach_update_check_frequency(_add_combo_item_enum);
		ComboBox_SetCurSel(controls.combo_update_check_frequency, state->settings->update_check_frequency);
		SetWindowSubclass(controls.combo_update_check_frequency, ComboProc, 0, 0);

		time_t t = state->settings->last_update_check;
		utf16 time[30] = L"Never"; 
		if (t) {
			struct tm local_time_info; localtime_s(&local_time_info, &t);
			std::wcsftime(time, ARRAYSIZE(time), L"%F %T", &local_time_info);
			for (auto& c : time) if (c == L'-') c = L'/';
		}
		controls.text_last_update_check = CreateWindowExW(
			WS_EX_COMPOSITED | WS_EX_TRANSPARENT,
			L"Static", (str(L"Last checked: ") + time).c_str(),
			WS_VISIBLE | WS_CHILD | SS_CENTERIMAGE | SS_ENDELLIPSIS
			, 0, 0, 0, 0, state->wnd, nullptr, nullptr, nullptr);

		controls.button_cancel = CreateWindowExW(
			WS_EX_COMPOSITED | WS_EX_TRANSPARENT,
			unCap_wndclass_button, L"Cancel",
			WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_ROUNDRECT
			, 0, 0, 0, 0, state->wnd, (HMENU)BUTTON_CANCEL, nullptr, nullptr);
		UNCAPBTN_set_brushes(controls.button_cancel, TRUE, unCap_colors.Img, unCap_colors.CaptionBk_Inactive, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);
		UNCAPBTN_set_cursor(controls.button_cancel, IDC_HAND);

		controls.button_save = CreateWindowExW(
			WS_EX_COMPOSITED | WS_EX_TRANSPARENT, 
			unCap_wndclass_button, L"Save",
			WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_ROUNDRECT | WS_DISABLED //by default the save button is not clickable
			, 0, 0, 0, 0, state->wnd, (HMENU)BUTTON_SAVE, nullptr, nullptr);
		UNCAPBTN_set_brushes(controls.button_save, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver, unCap_colors.Img_Disabled, unCap_colors.ControlBk_Disabled, unCap_colors.ControlTxt_Disabled);
		UNCAPBTN_set_cursor(controls.button_save, IDC_HAND);

		for (auto ctl : state->controls.all)
			SendMessage(ctl, WM_SETFONT, (WPARAM)unCap_fonts.General, TRUE);

		resize_controls(state);
	}

	ProcState* get_state(HWND updater) {
		auto state = (ProcState*)GetWindowLongPtr(updater, 0);
		return state;
	}

	void set_state(HWND updater, ProcState* state) {
		SetWindowLongPtr(updater, 0, (LONG_PTR)state);
	}

	void update_settings(ProcState* state) {
		state->settings->update_check_frequency = state->edit_settings.update_check_frequency;
	}

	void restart_controls_settings(ProcState* state) {
		auto& controls = state->controls;
		ComboBox_SetCurSel(controls.combo_update_check_frequency, state->settings->update_check_frequency);
	}

	//Call this when a setting has been edited. It's primary task is to detect if the 'Save' button should be enabled or disabled
	void on_setting_changed(ProcState* state) {
		bool changed = state->settings->update_check_frequency != state->edit_settings.update_check_frequency;
		EnableWindow(state->controls.button_save, changed);
	}

	LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		//printf(msgToString(msg)); printf("\n");
		auto state = get_state(hwnd);
		switch (msg) {
		case WM_CREATE:
		{
			CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;

			add_controls(state);

			set_title(state, L"Settings");

			return 0;
		} break;
		case WM_SIZE:
		{
			LRESULT res = DefWindowProc(hwnd, msg, wparam, lparam);
			resize_controls(state);
			return res;
		} break;
		case WM_NCDESTROY:
		{
			if (state) {
				free(state);
				state = nullptr;
			}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_NCCREATE:
		{
			CREATESTRUCT* create_nfo = (CREATESTRUCT*)lparam;
			auto st = (ProcState*)calloc(1, sizeof(ProcState));
			Assert(st);
			set_state(hwnd, st);
			st->wnd = hwnd;
			st->nc_parent = create_nfo->hwndParent;
			st->settings = (unCapClSettings*)create_nfo->lpCreateParams;
			st->edit_settings = *st->settings; //Potential BUG: if someone else were to change the settings then this would have outdated information. We should be the only ones that can update the specific settings that we handle.

			return TRUE;
		} break;
		case WM_COMMAND:
		{
			switch (LOWORD(wparam)) // Msgs from my controls
			{
			case COMBO_UPDATE_FREQUENCY:
			{
				HWND combo = (HWND)lparam;
				int selected_idx = ComboBox_GetCurSel(combo);
				if (selected_idx != CB_ERR) state->edit_settings.update_check_frequency = (UpdateCheckFrequency)selected_idx;
				on_setting_changed(state);
			} break;
			case BUTTON_SAVE:
				update_settings(state);
				//Do NOT break. After we update settings we fall off to the 'close' case to close the window and do default closing logic
			case BUTTON_CANCEL:
			case UNCAPNC_CLOSE:
			{
				ShowWindow(state->nc_parent, SW_HIDE);
				state->edit_settings = *state->settings;
				on_setting_changed(state);
				restart_controls_settings(state);
				return TRUE;
			} break;
			}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_CTLCOLORSTATIC:
		{
			HWND static_wnd = (HWND)lparam;
			HDC dc = (HDC)wparam;
			SetBkColor(dc, ColorFromBrush(unCap_colors.ControlBk));
			SetTextColor(dc, ColorFromBrush(static_wnd == state->controls.text_last_update_check ? unCap_colors.ControlTxt_Disabled : unCap_colors.ControlTxt));
			return (LRESULT)unCap_colors.ControlBk;
		} break;
		default:
		{
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		}
		return 0;
	}

	ATOM init_wndclass(HINSTANCE inst) {
		WNDCLASSEXW wcex{ sizeof(wcex) };
		wcex.style = CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc = proc;
		wcex.cbClsExtra = 0;
		wcex.cbWndExtra = sizeof(ProcState*);
		wcex.hInstance = inst;
		wcex.hIcon = 0;
		wcex.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wcex.hbrBackground = unCap_colors.ControlBk;
		wcex.lpszMenuName = 0;
		wcex.lpszClassName = wndclass;
		wcex.hIconSm = 0;

		ATOM class_atom = RegisterClassExW(&wcex);
		Assert(class_atom);
		return class_atom;
	}
}


const utf16* get_settings_wndclass() {
	return settings::wndclass;
}