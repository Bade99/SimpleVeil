#pragma once
#include <Windows.h>
#include "resource.h"
#include <Richedit.h>


namespace updater {
	constexpr int BUTTON_CANCEL = 23;
	constexpr int BUTTON_DOWNLOAD = 24;
	
	struct ProcState;
	typedef void(*func_on_click)(ProcState* state, void* user_extra);

	struct ProcState {
		HWND wnd;
		HWND nc_parent;

		union Controls {
			struct {
				HWND markdown_update_notes;
				HWND button_download;
				HWND button_cancel;
				HWND progressbar_download;
				HWND text_bytes_downloaded;
			};
			HWND all[5];
			void _() { static_assert(sizeof(*this) == sizeof(this->all), "Update 'all[x]' to the correct number of HWNDs"); }
		}controls;

		func_on_click on_confirm;
		func_on_click on_cancel;
		void* user_extra;

		bool downloading;
		cstr bytes_text[32];
	};

	utf16 wndclass[] = L"unCap_wndclass_updater";

	void resize_controls(ProcState* state) {
		RECT r; GetClientRect(state->wnd, &r);
		int w = RECTW(r);
		int h = RECTH(r);
		f32 w_pad = w * .05f;
		f32 h_pad = h * .05f;

		i32 small_wnd_h = 30;
		i32 small_wnd_w = 90;

		i32 fixed_w_pad = small_wnd_h / 6;
		i32 fixed_h_pad = fixed_w_pad;

		rect_f32 markdown = rect_f32::from_RECT(r);
		markdown.inflate(-2*fixed_w_pad, -2*fixed_h_pad);
		rect_f32 progressbar = markdown.cut_bottom(small_wnd_h);
		auto button_space = rect_f32{ progressbar };
		rect_f32 btn_yes = button_space.cut_right(small_wnd_w);
		button_space.cut_right(fixed_w_pad);
		rect_f32 btn_no = button_space.cut_right(small_wnd_w);
		auto text_bytes = progressbar.cut_right(small_wnd_w * 1.5f);

		auto& controls = state->controls;
		MoveWindow(controls.markdown_update_notes, markdown);
		MoveWindow(controls.progressbar_download, progressbar);
		MoveWindow(controls.text_bytes_downloaded, text_bytes);
		MoveWindow(controls.button_cancel, btn_no);
		MoveWindow(controls.button_download, btn_yes);
	}

	void set_title(ProcState* state, const TCHAR* title) {
		SetWindowTextW(state->nc_parent, title);
	}

	void set_content(ProcState* state, const utf8* content_markdown) {
		auto rtf_formatter = [](std::string content) -> std::string {
			auto replaceAll = [](std::string& text, const std::string from, const std::string to) {
				size_t start_pos = 0;
				while ((start_pos = text.find(from, start_pos)) != std::string::npos) {
					text.replace(start_pos, from.length(), to);
					start_pos += to.length(); // Handles case where 'to' is a substring of 'from'
				}
			};
			replaceAll(content, "\n- ", u8"\n    ●  ");
			replaceAll(content, "\n* ", u8"\n    ●  ");
			replaceAll(content, "\n+ ", u8"\n    ●  ");
			replaceAll(content, "\n1. ", u8"\n    1.  ");
			replaceAll(content, "\n2. ", u8"\n    2.  ");
			replaceAll(content, "\n3. ", u8"\n    3.  ");
			replaceAll(content, "\n4. ", u8"\n    4.  ");
			replaceAll(content, "\n5. ", u8"\n    5.  ");
			replaceAll(content, "\n6. ", u8"\n    6.  ");
			replaceAll(content, "\n7. ", u8"\n    7.  ");
			replaceAll(content, "\n8. ", u8"\n    8.  ");

			auto replaceAllSections = [](std::string& text, const std::string target, const std::string section_start, const std::string section_end){
				size_t start_pos = 0;
				while ((start_pos = text.find(target, start_pos)) != std::string::npos) {
					auto next_target = text.find(target, start_pos + target.length());
					auto next_newline = text.find("\n", start_pos + target.length());
					if (next_target < next_newline && text[start_pos + target.length()] != ' ' && text[next_target - 1] != ' ') {
						text.replace(next_target, target.length(), section_end);
						text.replace(start_pos, target.length(), section_start);
						start_pos += section_start.length();
					}
					else {
						start_pos += target.length();
					}
				}
			};

			replaceAllSections(content, "***", "{\\b\\i ", "}");
			replaceAllSections(content, "**", "{\\b ", "}");
			replaceAllSections(content, "*", "{\\i ", "}");
			replaceAllSections(content, "___", "{\\b\\i ", "}");
			replaceAllSections(content, "__", "{\\b ", "}");
			replaceAllSections(content, "_", "{\\i ", "}");

			replaceAll(content, "\n", "\\line ");
			return content;
		};
		if (richedit_support) {
			SETTEXTEX text_format; text_format.codepage = CP_UTF8; text_format.flags = ST_DEFAULT;

			auto formatted_text = rtf_formatter(std::string(content_markdown));
			auto rtf = std::string("{\\rtf ") + formatted_text + std::string("}");

			SendMessageW(state->controls.markdown_update_notes, EM_SETTEXTEX, (WPARAM)&text_format, (LPARAM)rtf.c_str());

			//NOTE: The Rich Edit control sucks so hard that you cannot set the default text color until text is already present on the control. So we have to wait until the text is set, and only then can we plead for it to change its color.
			CHARFORMAT cf{ sizeof(cf) };
			cf.dwMask = CFM_COLOR;
			cf.crTextColor = ColorFromBrush(unCap_colors.ControlTxt);
			SendMessageW(state->controls.markdown_update_notes, EM_SETCHARFORMAT, SCF_ALL | SPF_SETDEFAULT, (LPARAM)&cf);
		}
		else {
			SetWindowTextW(state->controls.markdown_update_notes, convert_utf8_to_utf16(content_markdown).c_str());
		}
	}

	void set_downloading(ProcState* state, bool downloading = true) {
		state->downloading = downloading;
		bool when_downloading, when_not_downloading;
		if (state->downloading) {
			when_downloading = SW_SHOW;
			when_not_downloading = SW_HIDE;
		}
		else {
			when_downloading = SW_HIDE;
			when_not_downloading = SW_SHOW;
		}
		ShowWindow(state->controls.button_cancel, when_not_downloading);
		ShowWindow(state->controls.button_download, when_not_downloading);
		ShowWindow(state->controls.progressbar_download, when_downloading);
		ShowWindow(state->controls.text_bytes_downloaded, when_downloading);
		AskForRepaint(state->wnd);
	}

	void set_download_progress(ProcState* state, u32 current_bytes, u32 total_bytes) {
		i32 progress = ((f32)current_bytes / max((f32)total_bytes, 1)) * 100;
		PostMessageW(state->controls.progressbar_download, PBM_SETPOS, clamp(0, progress, 100), 0);

		auto text_bytes = total_bytes ? _f(L"{} / {}", formatBytes(current_bytes), formatBytes(total_bytes)) : formatBytes(current_bytes);
		SetWindowTextW(state->controls.text_bytes_downloaded, text_bytes.c_str());
	}

	void set_user_extra(ProcState* state, void* user_extra) {
		state->user_extra = user_extra;
	}

	void set_on_confirm(ProcState* state, func_on_click on_confirm) {
		state->on_confirm = on_confirm;
	}

	void set_on_cancel(ProcState* state, func_on_click on_cancel) {
		state->on_cancel = on_cancel;
	}

	void add_controls(ProcState* state) {
		auto& controls = state->controls;

		controls.markdown_update_notes = CreateWindowExW(
			WS_EX_COMPOSITED | WS_EX_TRANSPARENT, RICHEDIT_CLASS_VERSION, nullptr, WS_VISIBLE | WS_CHILD | ES_MULTILINE | ES_READONLY
			, 0, 0, 0, 0, state->wnd, nullptr, nullptr, nullptr);

		controls.button_cancel = CreateWindowExW(
			WS_EX_COMPOSITED | WS_EX_TRANSPARENT, unCap_wndclass_button, L"Cancel", WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_ROUNDRECT
			, 0, 0, 0, 0, state->wnd, (HMENU)BUTTON_CANCEL, NULL, NULL);
		UNCAPBTN_set_brushes(controls.button_cancel, TRUE, unCap_colors.Img, unCap_colors.CaptionBk_Inactive, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);
		UNCAPBTN_set_cursor(controls.button_cancel, IDC_HAND);


		controls.button_download = CreateWindowExW(
			WS_EX_COMPOSITED | WS_EX_TRANSPARENT, unCap_wndclass_button, L"Download", WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_ROUNDRECT
			, 0, 0, 0, 0, state->wnd, (HMENU)BUTTON_DOWNLOAD, NULL, NULL);
		UNCAPBTN_set_brushes(controls.button_download, TRUE, unCap_colors.Img, unCap_colors.ControlBk, unCap_colors.ControlTxt, unCap_colors.ControlBkPush, unCap_colors.ControlBkMouseOver);
		UNCAPBTN_set_cursor(controls.button_download, IDC_HAND);

		controls.progressbar_download = CreateWindowExW(
			WS_EX_COMPOSITED | WS_EX_TRANSPARENT, PROGRESS_CLASS, 0, WS_CHILD | PBS_SMOOTH
			, 0, 0, 0, 0, state->wnd, nullptr, nullptr, nullptr);

		controls.text_bytes_downloaded = CreateWindowExW(
			WS_EX_COMPOSITED | WS_EX_TRANSPARENT, L"Static", nullptr, WS_CHILD | SS_CENTER | SS_CENTERIMAGE | SS_ENDELLIPSIS
			, 0, 0, 0, 0, state->wnd, nullptr, nullptr, nullptr);

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

	LRESULT CALLBACK proc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
		//printf(msgToString(msg)); printf("\n");
		auto state = get_state(hwnd);
		switch (msg) {
		case WM_CREATE:
		{
			CREATESTRUCT* createnfo = (CREATESTRUCT*)lparam;

			add_controls(state);

			set_title(state, L"Auto-Updater");

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

			return TRUE;
		} break;
		case WM_COMMAND:
		{
			switch (LOWORD(wparam)) // Msgs from my controls
			{
			case BUTTON_DOWNLOAD:
			{
				if (state->on_confirm) state->on_confirm(state, state->user_extra);
			} break;
			case BUTTON_CANCEL:
			{
				if (state->on_cancel) state->on_cancel(state, state->user_extra);
			} break;
			}
			return DefWindowProc(hwnd, msg, wparam, lparam);
		} break;
		case WM_CTLCOLORSTATIC:
		{
			HWND static_wnd = (HWND)lparam;
			HDC dc = (HDC)wparam;
			SetBkColor(dc, ColorFromBrush(unCap_colors.ControlBk));
			SetTextColor(dc, ColorFromBrush(unCap_colors.ControlTxt));
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
