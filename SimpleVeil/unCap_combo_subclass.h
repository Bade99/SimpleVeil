#pragma once
#include <CommCtrl.h> //DefSubclassProc

//-------------Additional Styles-------------:
#define CBS_ROUNDRECT 0x8000L //Border is made of a rounded rectangle

struct ComboProcState {
	HWND wnd;
	HBITMAP dropdown; //NOTE: we dont handle releasing the HBITMAP sent to us
	bool on_mouseover;
};

//#define CB_SETDROPDOWNIMG (WM_USER+1) //Only accepts monochrome bitmaps (.bmp), wparam=HBITMAP ; lparam=0
//#define CB_GETDROPDOWNIMG (WM_USER+2) //wparam=0 ; lparam=0 ; returns HBITMAP

//BUGs:
// 

//WinAPI Combobox BUGs: (this are bugs present in the standard combobox control, and thus may or may not be fixable)
//- When the user opens the combobox there's a small window of time where the text shown on top will get changed if the mouse got over some item of the list. To replicate: press combobox and quickly move the mouse straight down

//TODOs:
//- Configurable colors

#define bswap16(x) ((u16)((x>>8) | (x<<8)))
constexpr u16 _dropdown[16] = //TODO(fran): dropdown_inverted
{
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1100'1111'1111'0011),
	bswap16(0b1110'0111'1110'0111),
	bswap16(0b1111'0011'1100'1111),
	bswap16(0b1111'1001'1001'1111),
	bswap16(0b1111'1100'0011'1111),
	bswap16(0b1111'1110'0111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
	bswap16(0b1111'1111'1111'1111),
};
struct def_img {
	int bpp, w, h;
	void* mem;
} constexpr dropdown{ 1,16,16, (void*)_dropdown };

LRESULT CALLBACK ComboProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR /*uIdSubclass*/, DWORD_PTR /*dwRefData*/) {

	//INFO: we require GetWindowLongPtr at "position" GWLP_USERDATA to be left for us to use
	//NOTE: Im pretty sure that "position" 0 is already in use
	ComboProcState* state = (ComboProcState*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	if (!state) {
		ComboProcState* st = (ComboProcState*)calloc(1, sizeof(ComboProcState));
		st->wnd = hwnd;
		st->dropdown = CreateBitmap(dropdown.w, dropdown.h, 1, dropdown.bpp, dropdown.mem);
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)st);
		state = st;
	}

	//IMPORTANT: the user must free() the returned buffer if it is != null
	struct _GetComboText_res { cstr* buf = nullptr; bool is_cuebanner = false; };
	static auto GetComboText = [](HWND hwnd) {
		_GetComboText_res res;
		int index = ComboBox_GetCurSel(hwnd);
		if (index >= 0)
		{
			int buflen = ComboBox_GetLBTextLen(hwnd, index);
			res.buf = (decltype(res.buf))malloc((buflen + 1) * sizeof(*res.buf));
			ComboBox_GetLBText(hwnd, index, res.buf);
		}
		else if (index == CB_ERR) {//No item has been selected yet
			int max_buf_sz = 200;
			res.buf = (decltype(res.buf))malloc((max_buf_sz) * sizeof(*res.buf));
			res.is_cuebanner = ComboBox_GetCueBannerText(hwnd, res.buf, max_buf_sz);//INFO: the only way to know if there's a cue banner is by the return value, it also doesnt seem like you can retrieve the required size of the buffer
			if (!res.is_cuebanner) { free(res.buf); res.buf = nullptr; }
		}
		return res;
	};

	struct _GetBitmapPlacement_res { int bmp_align_width, bmp_align_height, bmp_width, bmp_height; };
	static auto GetBitmapPlacement = [](ComboProcState* state, RECT rc, int padding) {
		_GetBitmapPlacement_res res{ 0 };
		if (state->dropdown) {
			HBITMAP bmp = state->dropdown;
			BITMAP bitmap; GetObject(bmp, sizeof(bitmap), &bitmap);
			if (bitmap.bmBitsPixel == 1) {

				int max_sz = roundNdown(bitmap.bmWidth, (int)((float)RECTHEIGHT(rc) * .6f)); //HACK: instead use png + gdi+ + color matrices
				if (!max_sz)max_sz = bitmap.bmWidth; //More HACKs

				res.bmp_height = max_sz;
				res.bmp_width = res.bmp_height;
				res.bmp_align_width = RECTWIDTH(rc) - res.bmp_width - padding;
				res.bmp_align_height = (RECTHEIGHT(rc) - res.bmp_height) / 2;
			}
		}
		return res;
	};

	switch (msg)
	{
	case WM_NCDESTROY: //TODO(fran): better way to cleanup our state, we could have problems with state being referenced after WM_NCDESTROY and RemoveSubclass taking us out without being able to free our state, we need a way to find out when we are being removed
	{
		//	RemoveWindowSubclass(hwnd, this->ComboProc, uIdSubclass);
		if (state) {
			if (state->dropdown) { 
				DeleteObject(state->dropdown); 
				state->dropdown = nullptr; 
			}
			free(state);
		}
		return DefSubclassProc(hwnd, msg, wparam, lparam);
	}
	/*case CB_GETDROPDOWNIMG:
	{
		return (LRESULT)state->dropdown;
	} break;
	case CB_SETDROPDOWNIMG:
	{
		state->dropdown = (HBITMAP)wparam;
		return 0;
	} break;*/
	case CB_SETCURSEL:
	{
		LONG_PTR  dwStyle = GetWindowLongPtr(hwnd, GWL_STYLE);
		// turn off WS_VISIBLE
		SetWindowLongPtr(hwnd, GWL_STYLE, dwStyle & ~WS_VISIBLE);

		// perform the default action, minus painting
		LRESULT ret = DefSubclassProc(hwnd, msg, wparam, lparam); //defproc for setcursel DOES DRAWING, we gotta do the good ol' trick of invisibility, also it seems that it stores a paint request instead, cause after I make it visible again it asks for wm_paint, as it should have in the first place

		// turn WS_VISIBLE back on
		SetWindowLongPtr(hwnd, GWL_STYLE, dwStyle);

		return ret;
	}
	case WM_MOUSEHOVER:
	{
		//force button to repaint and specify hover or not
		if (state->on_mouseover) break;
		state->on_mouseover = true;
		InvalidateRect(hwnd, 0, TRUE);
		return DefSubclassProc(hwnd, msg, wparam, lparam);
	}
	case WM_MOUSELEAVE:
	{
		state->on_mouseover = false;
		InvalidateRect(hwnd, 0, TRUE);
		return DefSubclassProc(hwnd, msg, wparam, lparam);
	}
	case WM_MOUSEMOVE:
	{
		TRACKMOUSEEVENT tme;
		tme.cbSize = sizeof(TRACKMOUSEEVENT);
		tme.dwFlags = TME_HOVER | TME_LEAVE;
		tme.dwHoverTime = 1;
		tme.hwndTrack = hwnd;

		TrackMouseEvent(&tme);
		return DefSubclassProc(hwnd, msg, wparam, lparam);
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC dc = BeginPaint(hwnd, &ps); defer{ EndPaint(hwnd, &ps); };

		RECT rc; GetClientRect(hwnd, &rc);
		int w = RECTW(rc), h = RECTH(rc);
		LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE); Assert(style & CBS_DROPDOWNLIST);
		int border_thickness_pen = 0;//0 means 1px when creating pens
		int border_thickness = 1;

		BOOL button_state = ComboBox_GetDroppedState(hwnd);
		HBRUSH bk_br, border_br = unCap_colors.Img;
		if (button_state) bk_br = unCap_colors.ControlBkPush;
		else if (state->on_mouseover) bk_br = unCap_colors.ControlBkMouseOver;
		else bk_br = unCap_colors.ControlBk;

		int padding = 5;

		//Border & Bk
		{
			if (style & CBS_ROUNDRECT) {
				i32 extent = min(w, h);
				f32 radius = extent / 2.f;
				padding = radius;
				//Bk
				urender::draw_round_rectangle(dc, rc, radius, bk_br);
				//Border
				if (border_thickness) urender::draw_round_rectangle_outline(dc, rc, radius, border_br, border_thickness);
			}
			else {
				HPEN pen = CreatePen(PS_SOLID, border_thickness_pen, ColorFromBrush(border_br));
				HPEN oldpen = SelectPen(dc, pen); defer{ SelectObject(dc, oldpen); DeletePen(pen); };
				HBRUSH oldbr = SelectBrush(dc, bk_br); defer{ SelectBrush(dc,oldbr); };
				Rectangle(dc, rc.left, rc.top, rc.right, rc.bottom); //uses pen for border and brush for bk
			}
		}

		//Text
		{
			HBRUSH oldbrush = SelectBrush(dc, (HBRUSH)GetStockObject(HOLLOW_BRUSH)); defer{ SelectObject(dc, oldbrush); };
			SelectFont(dc, GetWindowFont(hwnd));
			SetBkColor(dc, ColorFromBrush(bk_br));
			SetTextColor(dc, ColorFromBrush(unCap_colors.ControlTxt));

			auto [buf, is_cuebanner] = GetComboText(hwnd);

			if (buf) {
				RECT txt_rc = rc;
				txt_rc.left += padding;
				COLORREF prev_txt_clr;
				if (is_cuebanner) {
					prev_txt_clr = SetTextColor(dc, ColorFromBrush(unCap_colors.ControlTxt_Disabled));
				}
				DrawText(dc, buf, -1, &txt_rc, DT_EDITCONTROL | DT_LEFT | DT_VCENTER | DT_SINGLELINE);
				if (is_cuebanner) {
					SetTextColor(dc, prev_txt_clr);
				}
				free(buf);
			}
		}

		//Dropdown icon
		if (state->dropdown) {//TODO(fran): flicker free
			HBITMAP bmp = state->dropdown;
			BITMAP bitmap; GetObject(bmp, sizeof(bitmap), &bitmap);
			if (bitmap.bmBitsPixel == 1) {

				auto [bmp_align_width, bmp_align_height, bmp_width, bmp_height] = GetBitmapPlacement(state, rc, padding);

				urender::draw_mask(dc, bmp_align_width, bmp_align_height, bmp_width, bmp_height, bmp, 0, 0, bitmap.bmWidth, bitmap.bmHeight, unCap_colors.Img);
			}
		}

		return 0;
	}
	case WM_ERASEBKGND:
	{
		return 1;
	}
	case WM_NCPAINT:
	{
		return 0;
	}
	}

	return DefSubclassProc(hwnd, msg, wparam, lparam);
}
