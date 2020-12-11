#pragma once
#include <Windows.h>
#include <shellapi.h>
#include "unCap_Platform.h"

#ifdef _DEBUG
#define UNCAP_ASSERTIONS
#endif

#ifdef UNCAP_ASSERTIONS
#define Assert(assertion) if(!(assertion))*(int*)NULL=0
#else 
#define Assert(assertion) 
#endif

#ifdef UNICODE
#define to_str(v) std::to_wstring(v)
#else
#define to_str(v) std::to_string(v)
#endif

//NOTE: do NOT include the null terminator
#ifdef UNICODE
static size_t cstr_len(const cstr* s) {
	return wcslen(s);
}
#define cstr_printf(...)  wprintf(__VA_ARGS__)

static errno_t cstr_lwr(cstr* s, size_t char_count /*includes null terminator*/) {
	return _wcslwr_s(s, char_count);
}
#else
static size_t cstr_len(const cstr* s) {
	return strlen(s);
}

#define cstr_printf(...)  printf(__VA_ARGS__)

static errno_t cstr_lwr(cstr* s, size_t char_count /*includes null terminator*/) {
	return _strlwr_s(s,char_count);
}
#endif

//Thanks to https://handmade.network/forums/t/1273-post_your_c_c++_macro_tricks/3
template <typename F> struct Defer { Defer(F f) : f(f) {} ~Defer() { f(); } F f; };
template <typename F> Defer<F> makeDefer(F f) { return Defer<F>(f); };
#define __defer( line ) defer_ ## line
#define _defer( line ) __defer( line )
struct defer_dummy { };
template<typename F> Defer<F> operator+(defer_dummy, F&& f) { return makeDefer<F>(std::forward<F>(f)); }
//usage: defer{block of code;}; //the last defer in a scope gets executed first (LIFO)
#define defer auto _defer( __LINE__ ) = defer_dummy( ) + [ & ]( )

static u32 safe_u64_to_u32(u64 n) {
	Assert(n <= 0xFFFFFFFF); //TODO(fran): u32_max and _min
	return (u32)n;
}

static void free_file_memory(void* memory) {
	if (memory) VirtualFree(memory, 0, MEM_RELEASE);
}
struct read_entire_file_res { void* mem; u32 sz;/*bytes*/ };
//NOTE: free the memory with free_file_memory()
static read_entire_file_res read_entire_file(const cstr* filename) {
	read_entire_file_res res{ 0 };
	HANDLE hFile = CreateFile(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
	if (hFile != INVALID_HANDLE_VALUE) {
		defer{ CloseHandle(hFile); };
		if (LARGE_INTEGER sz; GetFileSizeEx(hFile, &sz)) {
			u32 sz32 = safe_u64_to_u32(sz.QuadPart);
			void* mem = VirtualAlloc(0, sz32, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);//TOOD(fran): READONLY?
			if (mem) {
				if (DWORD bytes_read; ReadFile(hFile, mem, sz32, &bytes_read, 0) && sz32 == bytes_read) {
					//SUCCESS
					res.mem = mem;
					res.sz = sz32;
				}
				else {
					free_file_memory(mem);
				}
			}
		}
	}
	return res;
}
static bool write_entire_file(const cstr* filename, void* memory, u32 mem_sz) {
	bool res = false;
	HANDLE hFile = CreateFile(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
	if (hFile != INVALID_HANDLE_VALUE) {
		defer{ CloseHandle(hFile); };
		if (DWORD bytes_written; WriteFile(hFile, memory, mem_sz, &bytes_written, 0)) {
			//SUCCESS
			res = mem_sz == bytes_written;
		}
		else {
			//TODO(fran): log
		}
	}
	return res;
}
static bool append_to_file(const cstr* filename, void* memory, u32 mem_sz) {
	bool res = false;
	HANDLE hFile = CreateFile(filename, FILE_APPEND_DATA, FILE_SHARE_READ, 0, OPEN_ALWAYS, 0, 0);
	if (hFile != INVALID_HANDLE_VALUE) {
		defer{ CloseHandle(hFile); };
		if (DWORD bytes_written; WriteFile(hFile, memory, mem_sz, &bytes_written, 0)) {
			//SUCCESS
			res = mem_sz == bytes_written;
		}
		else {
			//TODO(fran): log
		}
	}
	return res;
}

//Timing info for testing
static f64 GetPCFrequency() {
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	return f64(li.QuadPart) / 1000.0; //milliseconds
}
static i64 StartCounter() {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}
static f64 EndCounter(i64 CounterStart, f64 PCFreq = GetPCFrequency())
{
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return double(li.QuadPart - CounterStart) / PCFreq;
}

static bool str_found(size_t p) {
	return p != str::npos;
}

//NOTE: offset should be 1 after the last character of "begin"
//NOTE: returns str::npos if not found
static size_t find_closing_str(str text, size_t offset, str begin, str close) {
	//Finds closing str, eg: open= { ; close= } ; you're here-> {......{..}....} <-finds this one
	size_t closePos = offset - 1;
	i32 counter = 1;
	while (counter > 0 && text.size() > ++closePos) {
		if (!text.compare(closePos, begin.size(), begin)) {
			counter++;
		}
		else if (!text.compare(closePos, close.size(), close)) {
			counter--;
		}
	}
	return !counter ? closePos : str::npos;
}

//Returns first non matching char, could be 1 past the size of the string
static size_t find_till_no_match(str s, size_t offset, str match) {
	size_t p = offset - 1;
	while (s.size() > ++p) {
		size_t r = (str(1, s[p])).find_first_of(match.c_str());//NOTE: find_first_of is really badly designed
		if (!str_found(r)) {
			break;
		}
	}
	return p;
}

//RECT related

#define RECTWIDTH(r) (r.right >= r.left ? r.right - r.left : r.left - r.right )

#define RECTHEIGHT(r) (r.bottom >= r.top ? r.bottom - r.top : r.top - r.bottom )

static RECT rectWH(LONG left, LONG top, LONG width, LONG height) {
	RECT r;
	r.left = left;
	r.top = top;
	r.right = r.left + width;
	r.bottom = r.top + height;
	return r;
}

static RECT rectNpxL(RECT r, int n) {
	RECT res{ r.left,r.top,r.left + n,r.bottom };
	return res;
}
static RECT rectNpxT(RECT r, int n) {
	RECT res{ r.left,r.top,r.right,r.top + n };
	return res;
}
static RECT rectNpxR(RECT r, int n) {
	RECT res{ r.right - n,r.top,r.right,r.bottom };
	return res;
}
static RECT rectNpxB(RECT r, int n) {
	RECT res{ r.left ,r.bottom - n,r.right,r.bottom };
	return res;
}

static RECT rect1pxL(RECT r) {
	RECT res = rectNpxL(r, 1);
	return res;
}
static RECT rect1pxT(RECT r) {
	RECT res = rectNpxT(r, 1);
	return res;
}
static RECT rect1pxR(RECT r) {
	RECT res = rectNpxR(r, 1);
	return res;
}
static RECT rect1pxB(RECT r) {
	RECT res = rectNpxB(r, 1);
	return res;
}

#define BORDERLEFT 0x01
#define BORDERTOP 0x02
#define BORDERRIGHT 0x04
#define BORDERBOTTOM 0x08
//NOTE: borders dont get centered, if you choose 5 it'll go 5 into the rc. TODO(fran): centered borders
static void FillRectBorder(HDC dc, RECT r, int thickness, HBRUSH br, int borders) {
	RECT borderrc;
	if (borders & BORDERLEFT) { borderrc = rectNpxL(r, thickness); FillRect(dc, &borderrc, br); }
	if (borders & BORDERTOP) { borderrc = rectNpxT(r, thickness); FillRect(dc, &borderrc, br); }
	if (borders & BORDERRIGHT) { borderrc = rectNpxR(r, thickness); FillRect(dc, &borderrc, br); }
	if (borders & BORDERBOTTOM) { borderrc = rectNpxB(r, thickness); FillRect(dc, &borderrc, br); }
}

static bool test_pt_rc(POINT p, RECT r) {
	bool res = false;
	if (p.y >= r.top &&//top
		p.y < r.bottom &&//bottom
		p.x >= r.left &&//left
		p.x < r.right)//right
	{
		res = true;
	}
	return res;
}

static bool rcs_overlap(RECT r1, RECT r2) {
	bool res = (r1.left < r2.right&& r1.right > r2.left && r1.top < r2.bottom&& r1.bottom > r2.top);
	return res;
}

static bool sameRc(RECT r1, RECT r2) {
	bool res = r1.bottom == r2.bottom && r1.left == r2.left && r1.right == r2.right && r1.top == r2.top;
	return res;
}

//HBRUSH related

static COLORREF ColorFromBrush(HBRUSH br) {
	LOGBRUSH lb;
	GetObject(br, sizeof(lb), &lb);
	return lb.lbColor;
}

//ICON related

struct MYICON_INFO //This is so stupid I find it hard to believe
{
	int     nWidth;
	int     nHeight;
	int     nBitsPerPixel;
};

static MYICON_INFO MyGetIconInfo(HICON hIcon)
{
	MYICON_INFO myinfo; ZeroMemory(&myinfo, sizeof(myinfo));

	ICONINFO info; ZeroMemory(&info, sizeof(info));

	BOOL bRes = FALSE; bRes = GetIconInfo(hIcon, &info);
	if (!bRes) return myinfo;

	BITMAP bmp; ZeroMemory(&bmp, sizeof(bmp));

	if (info.hbmColor)
	{
		const int nWrittenBytes = GetObject(info.hbmColor, sizeof(bmp), &bmp);
		if (nWrittenBytes > 0)
		{
			myinfo.nWidth = bmp.bmWidth;
			myinfo.nHeight = bmp.bmHeight;
			myinfo.nBitsPerPixel = bmp.bmBitsPixel;
		}
	}
	else if (info.hbmMask)
	{
		// Icon has no color plane, image data stored in mask
		const int nWrittenBytes = GetObject(info.hbmMask, sizeof(bmp), &bmp);
		if (nWrittenBytes > 0)
		{
			myinfo.nWidth = bmp.bmWidth;
			myinfo.nHeight = bmp.bmHeight / 2;
			myinfo.nBitsPerPixel = 1;
		}
	}

	if (info.hbmColor) DeleteObject(info.hbmColor);
	if (info.hbmMask) DeleteObject(info.hbmMask);

	return myinfo;
}

//HMENU related

static BOOL SetMenuItemData(HMENU hmenu, UINT item, BOOL fByPositon, ULONG_PTR data) {
	MENUITEMINFO i;
	i.cbSize = sizeof(i);
	i.fMask = MIIM_DATA;
	i.dwItemData = data;
	return SetMenuItemInfo(hmenu, item, fByPositon, &i);
}

static BOOL SetMenuItemString(HMENU hmenu, UINT item, BOOL fByPositon, const TCHAR* str) {
	MENUITEMINFOW menu_setter;
	menu_setter.cbSize = sizeof(menu_setter);
	menu_setter.fMask = MIIM_STRING;
	menu_setter.dwTypeData = const_cast<TCHAR*>(str);
	BOOL res = SetMenuItemInfoW(hmenu, item, fByPositon, &menu_setter);
	return res;
}

/// <summary>
/// Check to see if animation has been disabled in Windows
/// </summary>
/// <returns></returns>
inline BOOL GetDoAnimateMinimize(VOID)
{//Thanks to: https://www.codeproject.com/Articles/735/Minimizing-windows-to-the-System-Tray
	ANIMATIONINFO ai;

	ai.cbSize = sizeof(ai);
	SystemParametersInfo(SPI_GETANIMATION, sizeof(ai), &ai, 0);

	return ai.iMinAnimate ? TRUE : FALSE;
}

/// <summary>
/// Returns the rect of where we think the system tray is.
/// <para>If it can't find it, it returns a rect in the lower</para>
/// <para>right hand corner of the screen</para>
/// </summary>
/// <param name="lpTrayRect"></param>
inline void GetTrayWndRect(LPRECT lpTrayRect)
{//Thanks to: https://www.codeproject.com/Articles/735/Minimizing-windows-to-the-System-Tray
	// First, we'll use a quick hack method. We know that the taskbar is a window
	// of class Shell_TrayWnd, and the status tray is a child of this of class
	// TrayNotifyWnd. This provides us a window rect to minimize to. Note, however,
	// that this is not guaranteed to work on future versions of the shell. If we
	// use this method, make sure we have a backup!
	HWND hShellTrayWnd = FindWindowEx(NULL, NULL, TEXT("Shell_TrayWnd"), NULL);
	if (hShellTrayWnd)
	{
		HWND hTrayNotifyWnd = FindWindowEx(hShellTrayWnd, NULL, TEXT("TrayNotifyWnd"), NULL);
		if (hTrayNotifyWnd)
		{
			GetWindowRect(hTrayNotifyWnd, lpTrayRect);
			return;
		}
	}

	// OK, we failed to get the rect from the quick hack. Either explorer isn't
	// running or it's a new version of the shell with the window class names
	// changed (how dare Microsoft change these undocumented class names!) So, we
	// try to find out what side of the screen the taskbar is connected to. We
	// know that the system tray is either on the right or the bottom of the
	// taskbar, so we can make a good guess at where to minimize to
	APPBARDATA appBarData;
	appBarData.cbSize = sizeof(appBarData);
	if (SHAppBarMessage(ABM_GETTASKBARPOS, &appBarData))
	{
		// We know the edge the taskbar is connected to, so guess the rect of the
		// system tray. Use various fudge factor to make it look good
		switch (appBarData.uEdge)
		{
		case ABE_LEFT:
		case ABE_RIGHT:
			// We want to minimize to the bottom of the taskbar
			lpTrayRect->top = appBarData.rc.bottom - 100;
			lpTrayRect->bottom = appBarData.rc.bottom - 16;
			lpTrayRect->left = appBarData.rc.left;
			lpTrayRect->right = appBarData.rc.right;
			break;

		case ABE_TOP:
		case ABE_BOTTOM:
			// We want to minimize to the right of the taskbar
			lpTrayRect->top = appBarData.rc.top;
			lpTrayRect->bottom = appBarData.rc.bottom;
			lpTrayRect->left = appBarData.rc.right - 100;
			lpTrayRect->right = appBarData.rc.right - 16;
			break;
		}

		return;
	}

	// Blimey, we really aren't in luck. It's possible that a third party shell
	// is running instead of explorer. This shell might provide support for the
	// system tray, by providing a Shell_TrayWnd window (which receives the
	// messages for the icons) So, look for a Shell_TrayWnd window and work out
	// the rect from that. Remember that explorer's taskbar is the Shell_TrayWnd,
	// and stretches either the width or the height of the screen. We can't rely
	// on the 3rd party shell's Shell_TrayWnd doing the same, in fact, we can't
	// rely on it being any size. The best we can do is just blindly use the
	// window rect, perhaps limiting the width and height to, say 150 square.
	// Note that if the 3rd party shell supports the same configuraion as
	// explorer (the icons hosted in NotifyTrayWnd, which is a child window of
	// Shell_TrayWnd), we would already have caught it above
	hShellTrayWnd = FindWindowEx(NULL, NULL, TEXT("Shell_TrayWnd"), NULL);

#define DEFAULT_RECT_WIDTH 150
#define DEFAULT_RECT_HEIGHT 30

	if (hShellTrayWnd)
	{
		GetWindowRect(hShellTrayWnd, lpTrayRect);
		if (lpTrayRect->right - lpTrayRect->left > DEFAULT_RECT_WIDTH)
			lpTrayRect->left = lpTrayRect->right - DEFAULT_RECT_WIDTH;
		if (lpTrayRect->bottom - lpTrayRect->top > DEFAULT_RECT_HEIGHT)
			lpTrayRect->top = lpTrayRect->bottom - DEFAULT_RECT_HEIGHT;

		return;
	}

	// OK. Haven't found a thing. Provide a default rect based on the current work
	// area
	SystemParametersInfo(SPI_GETWORKAREA, 0, lpTrayRect, 0);
	lpTrayRect->left = lpTrayRect->right - DEFAULT_RECT_WIDTH;
	lpTrayRect->top = lpTrayRect->bottom - DEFAULT_RECT_HEIGHT;

#undef DEFAULT_RECT_WIDTH
#undef DEFAULT_RECT_HEIGHT
}

inline i32 win32_get_refresh_rate_hz(HWND wnd) {
	//TODO(fran): this may be simpler with GetDeviceCaps
	int res = 60;
	HMONITOR mon = MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST); //TODO(fran): should it directly receive the hmonitor?
	if (mon) {
		MONITORINFOEX nfo;
		nfo.cbSize = sizeof(nfo);
		if (GetMonitorInfo(mon, &nfo)) {
			DEVMODE devmode;
			devmode.dmSize = sizeof(devmode);
			devmode.dmDriverExtra = 0;
			if (EnumDisplaySettings(nfo.szDevice, ENUM_CURRENT_SETTINGS, &devmode)) {
				res = devmode.dmDisplayFrequency;
			}
		}
	}
	return res;
}

/// <summary>
/// Animates a window going to the tray and hides it
/// <para>Does not check whether the window is already minimized</para>
/// <para>The window position is left right where it started, so you can later use GetWindowRect to determine the "to" in TrayToWindow</para>
/// </summary>
/// <param name="from">Place from where the window will start moving, ej its top-left corner</param>
/// <param name="to">Window destination, aka the tray's top left corner, or wherever you want</param>
/// <param name="milliseconds">Duration of the animation</param>
/// <returns></returns>
inline void WindowToTray(HWND hWnd, POINT from, POINT to, int milliseconds) {
	float frametime = (1.f / win32_get_refresh_rate_hz(hWnd)) * 1000.f;
	float frames = milliseconds / frametime;
	RECT rc = { from.x,from.y,to.x,to.y };
	SIZE offset;
	offset.cx = (LONG)(RECTWIDTH(rc) / frames);//TODO(fran): should start faster and end slower, NOT be constant
	offset.cy = (LONG)(RECTHEIGHT(rc) / frames);
	POINT sign;
	sign.x = (from.x >= to.x) ? -1 : 1;
	sign.y = (from.y >= to.y) ? -1 : 1;
	float alphaChange = 100 / frames;


	//AnimateWindow(hWnd, milliseconds, AW_BLEND | AW_HIDE); //Not even async support!

	// Set WS_EX_LAYERED on this window 
	SetWindowLongPtr(hWnd, GWL_EXSTYLE, GetWindowLongPtr(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);

	for (int i = 1; i <= frames; i++) {
		SetLayeredWindowAttributes(hWnd, NULL, (BYTE)((255 * (100 - alphaChange * i)) / 100), LWA_ALPHA);
		SetWindowPos(hWnd, NULL, from.x + sign.x * offset.cx * i, from.y + sign.y * offset.cy * i, 0, 0, SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSIZE);
		Sleep((int)floor(frametime));//good enough
	}
	SetLayeredWindowAttributes(hWnd, 0, 0, LWA_ALPHA);
	SetWindowPos(hWnd, NULL, from.x, from.y, 0, 0, SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSIZE);
	ShowWindow(hWnd, SW_HIDE);
	SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA);
	SetWindowLongPtr(hWnd, GWL_EXSTYLE, GetWindowLongPtr(hWnd, GWL_EXSTYLE) ^ WS_EX_LAYERED);

}

//TODO(fran): combine this two, or at least parts of them
/// <summary>
/// Animates a window coming out of the tray and shows it
/// </summary>
/// <param name="hWnd"></param>
/// <param name="from">Place from where the window will start moving, ej its top-left corner</param>
/// <param name="to">Window destination, aka the top left corner of where the window was before minimizing, or wherever you want</param>
/// <param name="milliseconds">Duration of the animation</param>
inline void TrayToWindow(HWND hWnd, POINT from, POINT to, int milliseconds) {
	float frametime = (1.f / win32_get_refresh_rate_hz(hWnd)) * 1000.f;
	float frames = milliseconds / frametime;
	RECT rc = { from.x,from.y,to.x,to.y };
	SIZE offset;
	offset.cx = (LONG)(RECTWIDTH(rc) / frames); //TODO(fran): should start faster and end slower, NOT be constant
	offset.cy = (LONG)(RECTHEIGHT(rc) / frames);
	POINT sign;
	sign.x = (from.x >= to.x) ? -1 : 1;
	sign.y = (from.y >= to.y) ? -1 : 1;
	float alphaChange = 100 / frames;

	//We put the window at 0 alpha then we make the window visible 
	SetWindowLongPtr(hWnd, GWL_EXSTYLE, GetWindowLongPtr(hWnd, GWL_EXSTYLE) | WS_EX_LAYERED);
	SetLayeredWindowAttributes(hWnd, 0, 0, LWA_ALPHA);
	ShowWindow(hWnd, SW_SHOW);
	SendMessage(hWnd, WM_PAINT, 0, 0); EnumChildWindows(hWnd, [](HWND wnd, LPARAM lparam) {SendMessage(wnd, WM_PAINT, 0, 0); return TRUE; }, 0); //A little cheating, we force painting so everything looks normal when bringing the unloaded windows from the tray //TODO(fran): find a better way
	for (int i = 1; i <= frames; i++) {
		SetLayeredWindowAttributes(hWnd, NULL, (BYTE)((255 * (alphaChange * i)) / 100), LWA_ALPHA);
		SetWindowPos(hWnd, NULL, from.x + sign.x * offset.cx * i, from.y + sign.y * offset.cy * i, 0, 0, SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSIZE);
		Sleep((int)floor(frametime));//good enough
	}
	SetLayeredWindowAttributes(hWnd, 0, 255, LWA_ALPHA); //just to make sure
	SetWindowLongPtr(hWnd, GWL_EXSTYLE, GetWindowLongPtr(hWnd, GWL_EXSTYLE) ^ WS_EX_LAYERED);
	SetWindowPos(hWnd, NULL, to.x, to.y, 0, 0, SWP_NOZORDER | SWP_NOREDRAW | SWP_NOSIZE);//just to make sure, dont tell it to redraw cause it takes a long time to do it and looks bad

	SetActiveWindow(hWnd);
	SetForegroundWindow(hWnd);
}

/// <summary>
/// Minimizes a window and creates an animation to make it look like it goes to the tray
/// </summary>
/// <param name="hWnd"></param>
inline void MinimizeWndToTray(HWND hWnd)
{//Thanks to: https://www.codeproject.com/Articles/735/Minimizing-windows-to-the-System-Tray
	if (GetDoAnimateMinimize())
	{
		RECT rcFrom, rcTo;

		// Get the rect of the window. It is safe to use the rect of the whole
		// window - DrawAnimatedRects will only draw the caption
		GetWindowRect(hWnd, &rcFrom);
		GetTrayWndRect(&rcTo);

		WindowToTray(hWnd, { rcFrom.left,rcFrom.top }, { rcTo.left,rcTo.top }, 200);
	}
	else ShowWindow(hWnd, SW_HIDE);// Just hide the window
}

/// <summary>
/// Restores a window and makes it look like it comes out of the tray 
/// <para>and makes it back to where it was before minimizing</para>
/// </summary>
/// <param name="hWnd"></param>
inline void RestoreWndFromTray(HWND hWnd)
{//Thanks to: https://www.codeproject.com/Articles/735/Minimizing-windows-to-the-System-Tray
	if (GetDoAnimateMinimize())
	{
		// Get the rect of the tray and the window. Note that the window rect
		// is still valid even though the window is hidden
		RECT rcFrom, rcTo;
		GetTrayWndRect(&rcFrom);
		GetWindowRect(hWnd, &rcTo);

		// Get the system to draw our animation for us
		TrayToWindow(hWnd, { rcFrom.left,rcFrom.top }, { rcTo.left,rcTo.top }, 200);
	}
	else {
		// Show the window, and make sure we're the foreground window
		ShowWindow(hWnd, SW_SHOW);
		SetActiveWindow(hWnd);
		SetForegroundWindow(hWnd);
	}
}