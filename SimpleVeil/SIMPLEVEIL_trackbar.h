#pragma once
#include <windows.h>
#include <CommCtrl.h>
#include "windows_msg_mapper.h"

//Also known as Slider

//INFO: This is a simple subclass with just one job in mind, making the trackbar slider go to the CORRECT side when scrolling over it
//USE WITH: any trackbar that has the same scrolling issue as windows'
//USAGE: SetWindowSubclass(hwnd of trackbar, TrackbarProc, 0, 0);

LRESULT CALLBACK TrackbarProc(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam, UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData) {
	printf(msgToString(msg)); printf("\n");
	switch (msg) {
	case WM_MOUSEWHEEL:
	{
		WORD keys = LOWORD(wparam);
		short wheel_delta = (short)HIWORD(wparam);
		wheel_delta = -wheel_delta;

		return DefSubclassProc(wnd, msg, MAKELONG(keys,wheel_delta), lparam);
	} break;
	default: return DefSubclassProc(wnd, msg, wparam, lparam);
	}
	return 0;
}