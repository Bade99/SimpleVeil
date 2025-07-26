#pragma once
#include <Windows.h>
#include <shellapi.h>
#include <ShellScalingApi.h>

#include "unCap_Platform.h"
#include "fmt.h"

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
enum class CounterUnit {
	seconds = 1,
	milliseconds = 1000
};
//Timing info for testing
static f64 GetPCFrequency(CounterUnit unit) {
	LARGE_INTEGER li;
	QueryPerformanceFrequency(&li);
	return f64(li.QuadPart) / (f32)unit;
}
static i64 StartCounter() {
	LARGE_INTEGER li;
	QueryPerformanceCounter(&li);
	return li.QuadPart;
}
static f64 EndCounter(i64 CounterStart, CounterUnit unit = CounterUnit::milliseconds)
{
	f64 PCFreq = GetPCFrequency(unit);
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

#define RECTW RECTWIDTH

#define RECTH RECTHEIGHT

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
static void FillRectBorder(HDC dc, RECT r, int thickness, HBRUSH br, u8 borders) {
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

static f32 fully_roundrect_radius(RECT rc) {
	auto res = min(RECTWIDTH(rc), RECTHEIGHT(rc)) * .5f;
	return res;
}

//rect_f32

struct rect_f32 {
	f32 left, top, w, h;

	static rect_f32 from_RECT(RECT r) { return rect_f32{ (f32)r.left, (f32)r.top, (f32)RECTWIDTH(r), (f32)RECTHEIGHT(r) }; }
	static rect_f32 from_center_wh(f32 center_x, f32 center_y, f32 w, f32 h) { return rect_f32{ center_x - w * .5f, center_y - h * .5f, w, h }; }
	static rect_f32 from_center_radius(f32 center_x, f32 center_y, f32 radius) { auto diameter = radius * 2;  return rect_f32::from_center_wh(center_x, center_y, diameter, diameter); }
	RECT to_RECT() { return RECT{ (i32)this->left, (i32)this->top, (i32)this->right(), (i32)this->bottom() }; }

	f32 right() const { return left + w; }
	f32 bottom() const { return top + h; }
	f32 center_x() const { return left + w / 2.f; }
	f32 center_y() const { return top + h / 2.f; }
	rect_f32 cut_left(f32 w) {
		rect_f32 cut_section{ this->left, this->top, w, this->h };
		this->left += w;
		this->w -= w;
		return cut_section;
	}
	rect_f32 cut_right(f32 w) {
		this->w -= w;
		rect_f32 cut_section{ this->left + this->w, this->top, w, this->h };
		return cut_section;
	}
	rect_f32 cut_top(f32 h) {
		rect_f32 cut_section{ this->left, this->top, this->w, h };
		this->top += h;
		this->h -= h;
		return cut_section;
	}
	rect_f32 cut_bottom(f32 h) {
		this->h -= h;
		rect_f32 cut_section{ this->left, this->top + this->h, this->w, h };
		return cut_section;
	}
	void inflate(f32 dx, f32 dy) {
		f32 half_dx = dx / 2.f;
		f32 half_dy = dy / 2.f;
		this->left -= half_dx;
		this->top -= half_dy;
		this->w += dx;
		this->h += dy;
	}
	f32 fully_roundrect_radius() { return min(this->w, this->h) * .5f; }
};

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

//HWND

static void MoveWindow(HWND wnd, rect_f32 r, bool repaint = false) {
	MoveWindow(wnd, r.left, r.top, r.w, r.h, repaint);
}

/// <summary>
/// Check to see if animation has been disabled in Windows
/// </summary>
/// <returns></returns>
static BOOL GetDoAnimateMinimize(VOID)
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
static void GetTrayWndRect(LPRECT lpTrayRect)
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

static i32 win32_get_refresh_rate_hz(HWND wnd) {
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

static void AskForRepaint(HWND wnd) {
	InvalidateRect(wnd, NULL, TRUE);
	//RedrawWindow(wnd, NULL, NULL, RDW_INVALIDATE);
}

//WM_SETTEXT has an insane default implementation, where, unexplicably, it DOES PAINTING. This allows you to handle set text normally, without having it paint over your stuff.
static LRESULT HandleSetText(HWND wnd, UINT msg, WPARAM wparam, LPARAM lparam) {
	LONG_PTR  dwStyle = GetWindowLongPtr(wnd, GWL_STYLE);
	SetWindowLongPtr(wnd, GWL_STYLE, dwStyle & ~WS_VISIBLE);
	LRESULT ret = DefWindowProc(wnd, msg, wparam, lparam);
	SetWindowLongPtr(wnd, GWL_STYLE, dwStyle);
	RedrawWindow(wnd, NULL, NULL, RDW_INVALIDATE);
	return ret;
}

//HRGN
static HRGN GetOldRegion(HDC dc) {
	HRGN oldRegion = CreateRectRgn(0, 0, 0, 0); 
	if (GetClipRgn(dc, oldRegion) != 1) { 
		DeleteObject(oldRegion);
		oldRegion = NULL; 
	} 
	return oldRegion;
}

static void RestoreOldRegion(HDC dc, HRGN oldRegion) {
	SelectClipRgn(dc, oldRegion);
	if (oldRegion != NULL) DeleteObject(oldRegion);
}

static HRGN CreateRoundRectRgnIndirect(RECT rc, f32 radius) {
	auto r = rect_f32::from_RECT(rc);
	auto diameter = radius * 2;
	return CreateRoundRectRgn(r.left, r.top, r.right(), r.bottom(), diameter, diameter);
}

//DPI
static f32 GetScalingForMonitor(HMONITOR monitor) {
	//stolen from: https://github.com/ocornut/imgui/blob/master/backends/imgui_impl_win32.cpp
	f32 res;
	UINT xdpi = 96, ydpi = 96;

	static HMODULE WinSHCore = LoadLibraryW(L"shcore.dll");
	if (WinSHCore)
	{
		typedef HRESULT STDAPICALLTYPE get_dpi_for_monitor(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*); //GetDpiForMonitor
		static get_dpi_for_monitor* GetDpiForMonitor = (get_dpi_for_monitor*)GetProcAddress(WinSHCore, "GetDpiForMonitor");
		if (GetDpiForMonitor)
		{
			GetDpiForMonitor(monitor, MDT_EFFECTIVE_DPI, &xdpi, &ydpi);
			res = xdpi / 96.0f;
		}
		else goto last_resort;
	}
	else
	{
		//TODO(fran): see if there are better alternatives (specifically for WXP & W7)
	last_resort:
		HDC dc = GetDC(HWND_DESKTOP); defer{ ReleaseDC(HWND_DESKTOP, dc); };
		xdpi = GetDeviceCaps(dc, LOGPIXELSX);
		res = xdpi / 96.0f;
	}
	return res;
}

static f32 GetScalingForMonitorFromWindow(HWND wnd) {
	HMONITOR monitor = MonitorFromWindow(wnd, MONITOR_DEFAULTTONEAREST);
	f32 res = GetScalingForMonitor(monitor);
	return res;
}

//Unicode
#ifdef UNICODE
static str convert_utf8_to_utf16(const utf8* s, int sz /*bytes, not including null terminator*/) {
	str res = L"";
	int sz_char = MultiByteToWideChar(CP_UTF8, 0, (LPCCH)s, sz, 0, 0);//NOTE: pre windows vista this may need a change
	if (sz_char) {
		try {
			res.resize(sz_char); //this is wasteful since it writes zeroes til it reaches sz_char
			MultiByteToWideChar(CP_UTF8, 0, (LPCCH)s, sz, res.data(), sz_char);
		} catch (...) {}
	}
	return res;
}
static std::string convert_utf16_to_utf8(const utf16* s, int sz /*chars, not including null terminator*/) {
	std::string res = "";
	int sz_char = WideCharToMultiByte(CP_UTF8, 0, s, sz, 0, 0, 0, 0);//NOTE: pre windows vista this may need a change
	if (sz_char) {
		try {
			res.resize(sz_char); //this is wasteful since it writes zeroes til it reaches sz_char
			WideCharToMultiByte(CP_UTF8, 0, s, sz, res.data(), sz_char, 0, 0);
		}
		catch (...) {}
	}
	return res;
}
static str convert_utf8_to_utf16(std::string s) {
	return convert_utf8_to_utf16(s.c_str(), s.length());
}
static std::string convert_utf16_to_utf8(str s) {
	return convert_utf16_to_utf8(s.c_str(), s.length());
}
#else
static str convert_utf8_to_utf16(const utf8* s, int sz /*bytes*/) {
	str res(s, sz);
	return res;
}
static str convert_utf8_to_utf16(std::string s) { return s; }
#endif

//Bytes
static str formatBytes(u64 bytes) {
	f64 number = bytes;
	const cstr* units[] = { TEXT("B"), TEXT("KB"), TEXT("MB"), TEXT("GB"), TEXT("TB"), TEXT("PB"), TEXT("EB") };
	i32 unit = 0;
	f64 val = number;
	for (; (val = val / 1024) >= 1; unit++) number = val;
	return _f(floor(number) == number ? TEXT("{:.0f} {}") : TEXT("{:.2f} {}"), number, units[unit]);
}

//Bytes
static std::string formatBytesA(u64 bytes) {
	f64 number = bytes;
	const char* units[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB" };
	i32 unit = 0;
	f64 val = number;
	for (; (val = val / 1024) >= 1; unit++) number = val;
	return _f(floor(number) == number ? "{:.0f} {}" : "{:.2f} {}", number, units[unit]);
}