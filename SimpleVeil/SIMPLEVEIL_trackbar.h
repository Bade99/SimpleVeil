#pragma once
#include <windows.h>
#include <CommCtrl.h>
#include "windows_msg_mapper.h"

//Also known as Slider

//INFO: This is a simple subclass with just one job in mind, making the trackbar slider go to the CORRECT side when scrolling over it
//USE WITH: any trackbar that has the same scrolling issue as windows'
//USAGE: SetWindowSubclass(hwnd of trackbar, TrackbarProc, 0, (DWORD_PTR)calloc(1, sizeof(TrackbarProcState))); 
	//NOTE: TrackbarProc will take care of releasing that memory

struct TrackbarProcState {
	bool not_first_time;
	HWND wnd;
	HWND parent;
	struct {
		struct {
			HBRUSH thumb;
			HBRUSH channel_bk; //TODO: new type TwoTone to lerp between two colors as the slider moves
			HBRUSH thumb_channel_bk;
			HBRUSH channel_border;
			HBRUSH thumb_channel_border;
		} brushes;
		struct {
			f32 border_thickness;
		} dimensions;
	} styles;
};

void TRACKBAR_set_brushes(TrackbarProcState* state, 
	HBRUSH thumb, HBRUSH channel_bk, HBRUSH thumb_channel_bk, HBRUSH channel_border, HBRUSH thumb_channel_border,
	HWND wnd = nullptr) {
	if (thumb)state->styles.brushes.thumb = thumb;
	if (channel_bk)state->styles.brushes.channel_bk = channel_bk;
	if (thumb_channel_bk)state->styles.brushes.thumb_channel_bk = thumb_channel_bk;
	if (channel_border)state->styles.brushes.channel_border = channel_border;
	if (thumb_channel_border)state->styles.brushes.thumb_channel_border = thumb_channel_border;
	AskForRepaint(wnd ? wnd : state->wnd);
}

void TRACKBAR_set_dimensions(TrackbarProcState* state, 
	f32 border_thickness, 
	HWND wnd = nullptr) {
	state->styles.dimensions.border_thickness = border_thickness;
	AskForRepaint(wnd ? wnd : state->wnd);
}

LRESULT CALLBACK TrackbarProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData) {
	//TODO: since I have no way to tell the underlying control to make the thumb bigger Im gonna need to create my own control, either that or try to handle WM_LBUTTONDOWN

	TrackbarProcState* state = (TrackbarProcState*)dwRefData;
	Assert(state);
	if (!state->not_first_time) {
		state->not_first_time = !state->not_first_time;
		state->wnd = wnd;

		LONG_PTR  style = GetWindowLongPtr(state->wnd, GWL_STYLE);
		Assert(style & WS_CHILD);

		state->parent = GetParent(state->wnd);
	}

	switch (msg) {
	case WM_MOUSEWHEEL:
	{
		WORD keys = LOWORD(wparam);
		short wheel_delta = (short)HIWORD(wparam);
		wheel_delta = -wheel_delta;
		AskForRepaint(state->wnd);
		return DefSubclassProc(state->wnd, msg, MAKELONG(keys,wheel_delta), lparam);
	} break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_KEYDOWN:
	case RB_DRAGMOVE:
	case RB_ENDDRAG:
	{
		AskForRepaint(state->wnd); //INFO: this is the magical last piece of the alpha blended rendering puzzle, this probably clears the sections of the bitmap that can be rendered, thus restarting it to its initial state where nothing about this control gets rendered
		return DefSubclassProc(state->wnd, msg, wparam, lparam);
	} break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		RECT rc; GetClientRect(state->wnd, &rc);
		auto w = RECTW(rc), h = RECTH(rc);
		HDC dc = BeginPaint(state->wnd, &ps); defer{ EndPaint(state->wnd, &ps); };
		auto& brushes = state->styles.brushes;
		auto& dimensions = state->styles.dimensions;
		auto border_thickness = dimensions.border_thickness;
		HBRUSH channel_border_br = brushes.channel_border, channel_bk_br = brushes.channel_bk, 
			thumb_channel_border_br = brushes.thumb_channel_border, thumb_channel_bk_br = brushes.thumb_channel_bk, thumb_br = brushes.thumb;

		//TODO: the control adds extra space at the bottom to place the markers (that I dont use), can I tell it not to do that? or am I gonna have to create my own control from scratch?
		//Visualization of the size of the control
		//auto br2 = CreateSolidBrush(RGB(110, 0, 200)); defer{ DeleteObject(br2); };
		//FillRect(dc, &rc, br2);

		RECT _channel_rc; SendMessage(state->wnd, TBM_GETCHANNELRECT, 0, (LPARAM)&_channel_rc);
		
		RECT _thumb_rc; SendMessage(state->wnd, TBM_GETTHUMBRECT, 0, (LPARAM)&_thumb_rc);
		auto thumb_rc = rect_f32::from_RECT(_thumb_rc);
		auto radius = max(thumb_rc.w, thumb_rc.h) * .5f;
		auto center_x = thumb_rc.center_x(), center_y = thumb_rc.center_y();
		auto new_thumb_rc = rect_f32::from_center_radius(center_x, center_y, radius);

		rect_f32 channel_rc;
		auto channel_w = RECTW(_channel_rc), channel_h = RECTH(_channel_rc);
		auto orientation_horizontal = channel_w >= channel_h;
		auto channel_thickness = radius * 1.5f;
		if (orientation_horizontal)
			channel_rc = rect_f32::from_center_wh(rc.left + w * .5f, center_y, channel_w, channel_thickness);
		else
			channel_rc = rect_f32::from_center_wh(center_x, rc.top + h * .5f, channel_thickness, channel_h);

		auto final_channel_rc = channel_rc.to_RECT();
		auto channel_roundrect_radius = channel_rc.fully_roundrect_radius();
		if (channel_bk_br)
			urender::draw_round_rectangle(dc, final_channel_rc, channel_roundrect_radius, channel_bk_br);
		if (border_thickness && channel_border_br)
			urender::draw_round_rectangle_outline(dc, final_channel_rc, channel_roundrect_radius, channel_border_br, border_thickness);

		auto thumb_channel_rc = channel_rc;
		if (orientation_horizontal)
			thumb_channel_rc.w = new_thumb_rc.right() - thumb_channel_rc.left;
		else
			thumb_channel_rc.h = new_thumb_rc.bottom() - thumb_channel_rc.top;
		auto final_thumb_channel_rc = thumb_channel_rc.to_RECT();
		if (thumb_channel_bk_br)
			urender::draw_round_rectangle(dc, final_thumb_channel_rc, channel_roundrect_radius, thumb_channel_bk_br);
		if (border_thickness && thumb_channel_border_br)
			urender::draw_round_rectangle_outline(dc, final_thumb_channel_rc, channel_roundrect_radius, thumb_channel_border_br, border_thickness);

		urender::draw_ellipse(dc, new_thumb_rc, thumb_br);
		//TODO: there's a slight offset between the y position of the channel and the thumb. I pressume it may be due to some difference between how we handle rect_f32 and how Windows handles RECT, specifically on how the edges are calculated for rendering, leading to an off by one error

		return 0;
	} break;
	case WM_NCPAINT:
	case WM_ERASEBKGND:
	case WM_NCCALCSIZE: 
	{
		return 0;
	} break;
	case WM_SETTEXT:
	{
		return HandleSetText(wnd, msg, wparam, lparam);
	} break;
	default: 
	{
		//static i32 cnt; printf("%d: %s\n", cnt++, msgToString(msg));
		return DefSubclassProc(wnd, msg, wparam, lparam);
	} break;
	}
	return 0;
}