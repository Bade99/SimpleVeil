//Pointless warnings
#pragma warning(disable : 4065) //switch statement contains 'default' but no 'case' labels
#pragma warning(disable : 4100) //unreferenced formal parameter (actually can be useful)
#pragma warning(disable : 4189) //local variable is initialized but not referenced (actually can be useful)
#pragma warning(disable : 4201) //nonstandard extension used: nameless struct/union
#pragma warning(disable : 4458) //declaration of '...' hides class member (warning from windows' code)
#pragma warning(disable : 4505) //unreferenced local function has been removed
#pragma warning(disable : 4702) //unreachable code

#ifdef _DEBUG
//TODO(fran): change to logging
#define _SHOWCONSOLE
#else
//#define _SHOWCONSOLE
#endif

#include "resource.h"
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <Commctrl.h>
#include "unCap_Helpers.h"
#include "unCap_Global.h"
#include "LANGUAGE_MANAGER.h"
#include "unCap_uncapnc.h"
#include "unCap_button.h"
#include "unCap_Reflection.h"
#include "unCap_Serialization.h"
#include "unCap_edit_oneline.h"
#include "unCap_uncapcl.h"
#include "SimpleVeil_veil.h"
#include <vector>
#include <algorithm>
//#include <io.h> //_setmode
//#include <fcntl.h> //_O_U16TEXT

#pragma comment(lib, "comctl32.lib" ) //common controls lib
#pragma comment(lib,"shlwapi.lib") //strcmpI strcpyN
#pragma comment(lib,"UxTheme.lib") // setwindowtheme
#pragma comment(lib,"Imm32.lib") // IME related stuff

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

//TODO(fran): multimonitor support
//TODO(fran): better calculation for default wnd size, also store last screen size cause if it changes next time we gotta revert back to default wnd size

i32 n_tabs = 0;//Needed for serialization

UNCAP_COLORS unCap_colors{ 0 };
UNCAP_FONTS unCap_fonts{ 0 };

//The dc is passed to EnumFontFamiliesEx, you can just pass the desktop dc for example //TODO(fran): can we guarantee which dc we use doesnt matter? in that case dont ask the user for a dc and do it myself
BOOL hasFontFace(HDC dc, const TCHAR* facename) {
    int res = EnumFontFamiliesEx(dc/*You have to put some dc,not NULL*/, NULL
        , [](const LOGFONT* lpelfe, const TEXTMETRIC* /*lpntme*/, DWORD /*FontType*/, LPARAM lparam)->int {
            if (!StrCmpI((TCHAR*)lparam, lpelfe->lfFaceName)) {//Non case-sensitive comparison
                return 0;
            }
            return 1;
        }
    , (LPARAM)facename, NULL);
    return !res;
}
str GetFontFaceName() {
    //Font guidelines: https://docs.microsoft.com/en-us/windows/win32/uxguide/vis-fonts
    //Stock fonts: https://docs.microsoft.com/en-us/windows/win32/gdi/using-a-stock-font-to-draw-text

    //TODO(fran): can we take the best codepage from each font and create our own? (look at font linking & font fallback)

    //We looked at 2195 fonts, this is what's left
    //Options:
    //Segoe UI (good txt, jp ok) (10 codepages) (supported on most versions of windows)
    //-Arial Unicode MS (good text; good jp) (33 codepages) (doesnt come with windows)
    //-Microsoft YaHei or UI (look the same,good txt,good jp) (6 codepages) (supported on most versions of windows)
    //-Microsoft JhengHei or UI (look the same,good txt,ok jp) (3 codepages) (supported on most versions of windows)

    i64 cnt = StartCounter(); defer{ printf("ELAPSED: %f ms\n",EndCounter(cnt)); };

    HDC dc = GetDC(GetDesktopWindow()); defer{ ReleaseDC(GetDesktopWindow(),dc); }; //You can use any hdc, but not NULL
    std::vector<str> fontnames;
    EnumFontFamiliesEx(dc, NULL
        , [](const LOGFONT* lpelfe, const TEXTMETRIC* /*lpntme*/, DWORD /*FontType*/, LPARAM lParam)->int {((std::vector<str>*)lParam)->push_back(lpelfe->lfFaceName); return TRUE; }
    , (LPARAM)&fontnames, NULL);

    const TCHAR* requested_fontname[] = { TEXT("Segoe UI"), TEXT("Arial Unicode MS"), TEXT("Microsoft YaHei"), TEXT("Microsoft YaHei UI")
                                        , TEXT("Microsoft JhengHei"), TEXT("Microsoft JhengHei UI") };

    for (int i = 0; i < ARRAYSIZE(requested_fontname); i++)
        if (std::any_of(fontnames.begin(), fontnames.end(), [f = requested_fontname[i]](str s) {return !s.compare(f); })) return requested_fontname[i];

    return TEXT("");
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow)
{
    UNREFERENCED_PARAMETER(lpCmdLine);//TODO(fran): get dropped files (CommandLineToArgvW)

#ifdef _SHOWCONSOLE
    AllocConsole();
    FILE* ___s; defer{ fclose(___s); };
    freopen_s(&___s, "CONIN$", "r", stdin);
    freopen_s(&___s, "CONOUT$", "w", stdout);
    freopen_s(&___s, "CONOUT$", "w", stderr);
    //INFO: _setmode(_fileno(stdout), _O_U16TEXT); not really sure what it does, but I'd suppose it tries to make stdout output utf16, though the console is still unable to do it :c (also using printf instead of wprintf will trigger an assertion)
#endif

    //Check that no instance is already running on this user session
    //TODO(fran): from my tests this system is user session independent, check that's true for every case, 
    // eg. admin - admin, admin - normal user, normal user - normal user
    HANDLE single_instance_mutex = CreateMutex(NULL, TRUE, L"SimpleVeil Single Instance Mutex Franco Badenas Abal");
    if (single_instance_mutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
        printf("Found another instance of SimpleVeil already running\n");
        //If an instance already exists try to show its manager to the user
        //INFO: other ways of solving the hwnd finding: http://www.flounder.com/nomultiples.htm
        HWND existingApp = FindWindow(unCap_wndclass_uncap_nc, appName);
        if (existingApp) {
            //NOTE: im on a different thread and everything so I cant ask for the state, it doesnt retrieve something valid, therefore I cant get to my client, TODO IMPORTANT: this is a limitation that must be addressed from my framework, I need a way to get to communicate with my client from the parent, maybe a get client state msg is enough
            printf("Attempt to show previous instance\n");
            //PostMessage(existingApp's client wnd, TRAY, 0, WM_RBUTTONDOWN);
            ShowWindow(existingApp,SW_SHOW);//TODO(fran): for now im content with just showing the wnd, no animation of it coming from the tray
        }
        return 0; // Exit the app
    }
    defer{ ReleaseMutex(single_instance_mutex);	CloseHandle(single_instance_mutex); };


    urender::init(); defer{ urender::uninit(); };

    //Initialization of common controls
    INITCOMMONCONTROLSEX icc{ sizeof(icc) };
    icc.dwICC = ICC_STANDARD_CLASSES;

    BOOL comm_res = InitCommonControlsEx(&icc);
    Assert(comm_res);



    LOGFONT lf{ 0 };
    lf.lfQuality = CLEARTYPE_QUALITY;
    lf.lfHeight = -15;//TODO(fran): parametric
    //INFO: by default if I dont set faceName it uses "Modern", looks good but it lacks some charsets
    StrCpyN(lf.lfFaceName, GetFontFaceName().c_str(), ARRAYSIZE(lf.lfFaceName));

    unCap_fonts.General = CreateFontIndirect(&lf);
    Assert(unCap_fonts.General);

    lf.lfHeight = (LONG)((float)GetSystemMetrics(SM_CYMENU) * .85f);

    unCap_fonts.Menu = CreateFontIndirectW(&lf);
    Assert(unCap_fonts.Menu);

    const str to_deserialize = load_file_serialized();

    LANGUAGE_MANAGER& lang_mgr = LANGUAGE_MANAGER::Instance(); lang_mgr.SetHInstance(hInstance);
    unCapClSettings uncap_cl;

    _BeginDeserialize();
    _deserialize_struct(lang_mgr, to_deserialize);
    _deserialize_struct(uncap_cl, to_deserialize);
    _deserialize_struct(unCap_colors, to_deserialize);
    default_colors_if_not_set(&unCap_colors);
    defer{ for (HBRUSH& b : unCap_colors.brushes) if (b) { DeleteBrush(b); b = NULL; } };

    init_wndclass_unCap_uncapcl(hInstance);

    init_wndclass_unCap_uncapnc(hInstance);

    init_wndclass_unCap_button(hInstance);

    init_wndclass_SimpleVeil_veil(hInstance,unCap_colors.Veil);

    init_wndclass_unCap_edit_oneline(hInstance);

    // Create Veil window
    HWND veil_wnd = CreateWindowEx(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        SimpleVeil_wndclass_veil, L"The Veil itself",
        WS_POPUP,
        0, 0, 100, 100,
        nullptr, nullptr, hInstance, nullptr);
    Assert(veil_wnd);
    //ShowWindow(veil_wnd, SW_MAXIMIZE);

    uncap_cl.veil_wnd = veil_wnd;

    RECT uncapnc_rc = UNCAPNC_calc_nonclient_rc_from_client(uncap_cl.rc, FALSE);

    unCapNcLpParam nclpparam;
    nclpparam.client_class_name = unCap_wndclass_uncap_cl;
    nclpparam.client_lp_param = &uncap_cl;

    //NOTE(fran): if you ask for OVERLAPPEDWINDOW someone else draws your nc area
    HWND uncapnc = CreateWindowEx(WS_EX_CONTROLPARENT | WS_EX_TOPMOST | WS_EX_APPWINDOW, unCap_wndclass_uncap_nc, NULL, WS_THICKFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        uncapnc_rc.left, uncapnc_rc.top, RECTWIDTH(uncapnc_rc), RECTHEIGHT(uncapnc_rc), nullptr, nullptr, hInstance, &nclpparam);

    if (!uncapnc) return FALSE;

    ShowWindow(uncapnc, nCmdShow);
    UpdateWindow(uncapnc);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SIMPLEVEIL));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        //if (!IsDialogMessage(hwnd, &msg))
        //{
        //	TranslateMessage(&msg);
        //	DispatchMessage(&msg);
        //}

    }

    str serialized;
    _BeginSerialize();
    serialized += _serialize_struct(lang_mgr);
    serialized += _serialize_struct(uncap_cl);
    serialized += _serialize_struct(unCap_colors);

    save_to_file_serialized(serialized);

    return (int)msg.wParam;
}
