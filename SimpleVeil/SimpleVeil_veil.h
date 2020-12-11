#pragma once
#include <windows.h>
#include "unCap_Helpers.h"

constexpr TCHAR SimpleVeil_wndclass_veil[] = TEXT("SimpleVeil_wndclass_veil");

LRESULT CALLBACK VeilProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}

	return 0;
}

void init_wndclass_SimpleVeil_veil(HINSTANCE hInst, HBRUSH color) {
    // Register Veil class
    WNDCLASSEXW Wc;
    Wc.cbSize = sizeof(WNDCLASSEXW);
    Wc.style = CS_HREDRAW | CS_VREDRAW;
    Wc.lpfnWndProc = VeilProc;
    Wc.cbClsExtra = 0;
    Wc.cbWndExtra = 0;
    Wc.hInstance = hInst;
    Wc.hIcon = NULL;
    Wc.hCursor = NULL;
    Wc.hbrBackground = color;
    Wc.lpszMenuName = nullptr;
    Wc.lpszClassName = SimpleVeil_wndclass_veil;
    Wc.hIconSm = NULL;
    const ATOM veil_class = RegisterClassExW(&Wc);
    Assert(veil_class);
}