//Pointless warnings
#pragma warning(disable : 4065) //switch statement contains 'default' but no 'case' labels
#pragma warning(disable : 4100) //unreferenced formal parameter (actually can be useful)
#pragma warning(disable : 4189) //local variable is initialized but not referenced (actually can be useful)
#pragma warning(disable : 4201) //nonstandard extension used: nameless struct/union
#pragma warning(disable : 4458) //declaration of '...' hides class member (warning from windows' code)
#pragma warning(disable : 4505) //unreferenced local function has been removed
#pragma warning(disable : 4702) //unreachable code

#ifndef UNICODE
#error This application can only be built for unicode (utf16) versions of Windows
#endif

#define __STDC_WANT_LIB_EXT1__ 1 //add safe function definitions for c standard lib

#ifdef _DEBUG
#define _SHOWCONSOLE
#define _AUTOUPDATER
#define _AUTOUPDATER_TEST
#else
//#define _SHOWCONSOLE
#define _AUTOUPDATER
#endif

#include "unCap_Platform.h"

static const wchar_t* RICHEDIT_CLASS_VERSION = nullptr;
static bool richedit_support = false;

struct unCapClSettings;
const utf16* get_settings_wndclass();

#include "resource.h"
#include "targetver.h"
#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <Commctrl.h>
#include <wininet.h>
#include "fmt.h"
#include "unCap_Helpers.h"
#include "unCap_animation.h"
#include "unCap_Global.h"
#include "LANGUAGE_MANAGER.h"
#include "unCap_uncapnc.h"
#include "unCap_updater.h"
#include "unCap_button.h"
#include "unCap_toggle_button.h"
#include "unCap_Reflection.h"
#include "unCap_Serialization.h"
#include "unCap_edit_oneline.h"
#include "unCap_uncapcl.h"
#include "unCap_settings.h"
#include "SimpleVeil_veil.h"
#include <vector>
#include <algorithm>
//#include <io.h> //_setmode
//#include <fcntl.h> //_O_U16TEXT
#include "nlohmann/json.hpp"
using json = nlohmann::json;

#pragma comment(lib, "comctl32.lib" ) //common controls lib
#pragma comment(lib,"shlwapi.lib") //strcmpI strcpyN
#pragma comment(lib,"UxTheme.lib") // setwindowtheme
#pragma comment(lib,"Imm32.lib") // IME related stuff
#pragma comment(lib,"Wininet.lib") // internet
#pragma comment(lib,"Urlmon.lib") // UrlDownloadToFile

#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

//TODO(fran): windows that are created after the veil is running might go over it, even if they arent topmost, we gotta find a way to keep making the veil the top most one
//TODO(fran): better calculation for default wnd size, also store last screen size cause if it changes next time we gotta revert back to default wnd size
//TODO(fran): win8 works well, UI for nonclient needs specific win8 work to match the style
//TODO(fran): win7 works well, UI becomes basically unusable once you minimize and restore, also non client needs special win7 style drawing
//TODO(fran): add the gamma table option, apart from changing brightness via a dark window there's the option to set the color mapping for the screen, that way we can map every pixel lower, this way we might be able to get around fullscreen applications like games, that once fullscreen wont allow anything to go above them without minimizing, problem is if we crash then the user has to restart the pc to reset the gamma mapping

i32 n_tabs = 0;//Needed for serialization

UNCAP_COLORS unCap_colors{ 0 };
UNCAP_FONTS unCap_fonts{ 0 };

HWND uncapnc, updaternc;

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
    EnumFontFamiliesExW(dc, NULL
        , [](const LOGFONT* lpelfe, const TEXTMETRIC* /*lpntme*/, DWORD /*FontType*/, LPARAM lParam)->int {((std::vector<str>*)lParam)->push_back(lpelfe->lfFaceName); return TRUE; }
    , (LPARAM)&fontnames, NULL);

    const utf16* requested_fontname[] = { L"Segoe UI", L"Arial Unicode MS", L"Microsoft YaHei", L"Microsoft YaHei UI"
                                        , L"Microsoft JhengHei", L"Microsoft JhengHei UI" };

    for (int i = 0; i < ARRAYSIZE(requested_fontname); i++)
        if (std::any_of(fontnames.begin(), fontnames.end(), [f = requested_fontname[i]](str s) {return !StrCmpIW(s.c_str(), f); })) 
            return requested_fontname[i];

    return L"";
}

void ShowFormattedLastError(DWORD dw = GetLastError())
{
    LPCTSTR lpMsgBuf;

    auto title = L"Error";
    auto error_code = L"\nError code: " + to_str(dw);

    if (FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        GetModuleHandleW(L"wininet.dll"), dw, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL) == 0)
        MessageBoxW(NULL, (str(L"FormatMessage failed") + error_code).c_str(), title, MB_OK);
    else {
        MessageBoxW(NULL, (str(lpMsgBuf) + error_code).c_str(), title, MB_OK);
        LocalFree((LPVOID)lpMsgBuf);
    }
}

void ShowFormattedInternetLastResponseInfo()
{
    TCHAR internet_buffer[1024 * 4];
    DWORD error, buflen = ARRAYSIZE(internet_buffer);
    auto res = InternetGetLastResponseInfoW(&error, internet_buffer, &buflen);

    if (res) {
        printf("ShowFormattedInternetLastResponseInfo Error Code: %d\n", error);
        MessageBoxW(NULL, internet_buffer, TEXT("Error"), MB_OK);
    }
    else
        ShowFormattedLastError();
}

struct _internet_context {
    HINTERNET internet = nullptr, internet_session = nullptr, internet_request = nullptr;
    void close_handles() {
        if (this->internet_request) { InternetCloseHandle(this->internet_request); this->internet_request = nullptr; }
        if (this->internet_session) { InternetCloseHandle(this->internet_session); this->internet_session = nullptr; }
        if (this->internet) { InternetCloseHandle(this->internet); this->internet = nullptr; }
    }
} static internet_context;

std::string_view get_to_first_number(std::string s) {
    auto numbers = "0123456789";
    auto res = std::string_view(s).substr(min(s.find_first_of(numbers), s.length()));
    return res;
}

class DownloadCallback : public IBindStatusCallback {
public:
    u32 downloaded_bytes, filesize_bytes;
    void reset_state() {
        this->downloaded_bytes = 0;
        this->filesize_bytes = 0;
    }
    HRESULT __stdcall QueryInterface(const IID&, void**) { return E_NOINTERFACE; }
    ULONG STDMETHODCALLTYPE AddRef(void) { return 1; }
    ULONG STDMETHODCALLTYPE Release(void) { return 1; }
    HRESULT STDMETHODCALLTYPE OnStartBinding(DWORD dwReserved, IBinding* pib) {
        this->reset_state();
        return E_NOTIMPL; 
    }
    virtual HRESULT STDMETHODCALLTYPE GetPriority(LONG* pnPriority) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE OnLowResource(DWORD reserved) { return S_OK; }
    virtual HRESULT STDMETHODCALLTYPE OnStopBinding(HRESULT hresult, LPCWSTR szError) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE GetBindInfo(DWORD* grfBINDF, BINDINFO* pbindinfo) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE OnDataAvailable(DWORD grfBSCF, DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed) { return E_NOTIMPL; }
    virtual HRESULT STDMETHODCALLTYPE OnObjectAvailable(REFIID riid, IUnknown* punk) { return E_NOTIMPL; }

    virtual HRESULT __stdcall OnProgress(ULONG ulProgress, ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
    {
        this->downloaded_bytes = ulProgress;
        this->filesize_bytes = ulProgressMax;

        auto updater_state = updater::get_state(UNCAPNC_get_state(updaternc)->client);

        updater::set_download_progress(updater_state, this->downloaded_bytes, this->filesize_bytes);
        return S_OK;
    }

} static download_callback;

struct github_asset {
    std::string name;
    std::string content_type;
    u32 size;
    std::string browser_download_url;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(github_asset, name, content_type, size, browser_download_url)

struct github_release {
    std::string tag_name;
    bool draft;
    bool prerelease;
    std::vector<github_asset> assets;
    std::string body;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(github_release, tag_name, draft, prerelease, assets, body)

std::string cleanup_filename(std::string filename) {
    //TODO: we could use PathCleanupSpec() or some variant that allows for longer than MAX_PATH filepaths
    std::string res;
    res.reserve(filename.size());
    for (size_t i = 0; i < filename.size(); i++)
        if (filename[i] != '\\' && 
            filename[i] != '/' &&
            filename[i] != ':' &&
            filename[i] != '*' &&
            filename[i] != '?' &&
            filename[i] != '"' &&
            filename[i] != '<' &&
            filename[i] != '>' &&
            filename[i] != '|'&&
            filename[i] != ';'&&
            filename[i] != ',') res += filename[i];
    return res;
}

void InternetStatusCallback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength)
{
    switch (dwInternetStatus)
    {
    case INTERNET_STATUS_REQUEST_COMPLETE:
    {
        auto context = (_internet_context*)dwContext; defer{ context->close_handles(); };
        auto res = (INTERNET_ASYNC_RESULT*)lpvStatusInformation;
        if (res->dwResult) {
            DWORD content_length_bytes;
            DWORD buf_sz = sizeof(content_length_bytes);
            DWORD header_index = 0;

            auto querynfo_res = HttpQueryInfoW(hInternet, HTTP_QUERY_CONTENT_LENGTH | HTTP_QUERY_FLAG_NUMBER, &content_length_bytes, &buf_sz, &header_index);
            if (!querynfo_res) //TODO: if we get the content length we should be able to do only one call to InternetReadFile and exit without having to do the additional copy
                content_length_bytes = 1024;

            char* temp_buffer = new char[content_length_bytes]; defer{ delete[] temp_buffer; };

            std::string buffer;
            auto success = false;
            auto fails = 3;
            while (true) {
                DWORD bytes_read = 0;
                auto read_res = InternetReadFile(hInternet, temp_buffer, content_length_bytes, &bytes_read);

                if (!bytes_read) { success = true; break; }

                if (read_res) buffer.append(temp_buffer, bytes_read);
                else {
                    ShowFormattedLastError();
                    if (!fails--) break;
                }
            }
            if (success) {
                try {
                    auto release = json::parse(buffer).get<github_release>();

                    auto old_version = std::string(APP_VERSION);
                    auto new_version = get_to_first_number(release.tag_name);
                    if (release.assets.size() >= 1 && release.assets[0].content_type == "application/x-msdownload" && !release.draft && !release.prerelease && new_version > old_version) { //TODO: better comparison function, the object is formed like so XXX.XXX.XXX, a simple > is not enough for cases like 1.14.0 > 1.2.0
                        auto& asset = release.assets[0];
                        auto title = convert_utf8_to_utf16(_f("Update from {} to {}", old_version, new_version));
                        auto description = _f("There's a new version of {} available!\nWould you like to update to {}?\n\n{}\n\nDownload size: {}", APP_NAME, new_version, release.body, formatBytesA(asset.size));
                        ShowWindow(updaternc, SW_SHOW);
                        auto updater_state = updater::get_state(UNCAPNC_get_state(updaternc)->client);
                        updater::set_title(updater_state, title.c_str());
                        updater::set_content(updater_state, description.c_str());

                        struct download_info { std::string browser_download_url, name; };
                        auto asset_data = new download_info{asset.browser_download_url, asset.name};
                        updater::set_user_extra(updater_state, asset_data);

                        updater::set_on_cancel(updater_state, [](updater::ProcState* state, void* user_extra) {
                            delete (download_info*)user_extra;
                            DestroyWindow(state->nc_parent);
                        });
                        updater::set_on_confirm(updater_state, [](updater::ProcState* state, void* user_extra) {
                            updater::set_downloading(state);
                            static const auto download_and_install = [](void* user_extra) -> DWORD {
                                auto asset_data = (download_info*)user_extra; defer{ delete asset_data; };
                                auto new_version_download_url = asset_data->browser_download_url;
                                //auto new_version_download_url = std::string("http://ipv4.download.thinkbroadband.com/5MB.zip");
                                auto new_name = cleanup_filename(asset_data->name);

                                char exe_full_path[1024]; auto ret = GetModuleFileNameA(nullptr, exe_full_path, ARRAYSIZE(exe_full_path));
                                if (!ret || (ret == ARRAYSIZE(exe_full_path) && GetLastError() == ERROR_INSUFFICIENT_BUFFER)) return 1; //TODO: error filename is too long
                                //TODO remove filename from filepath and add a random filename, then remove the old simpleveil and rename the new file to new_name

                                auto path = std::string_view(exe_full_path); auto last_slash = path.find_last_of("/\\"); last_slash = last_slash != std::string_view::npos ? last_slash + 1 : last_slash; path = path.substr(0, last_slash);
                                auto download_full_path = std::string(path) + "VEIL.tmp";
                                //GetTempFileNameA(full_path.c_str(), "VEIL", 0, )

                                auto res = URLDownloadToFileA(nullptr, new_version_download_url.c_str(), download_full_path.c_str(), 0, &download_callback);
                                if (res == S_OK && download_callback.downloaded_bytes && download_callback.filesize_bytes && download_callback.downloaded_bytes == download_callback.filesize_bytes) {
                                    //return 0;
                                    STARTUPINFOA info = { sizeof(info) };
                                    info.lpTitle = (LPSTR)"SimpleVeil Updater";
                                    PROCESS_INFORMATION processInfo;
                                    auto new_file_full_path = std::string(path) + new_name;

                                    //TODO: if new_name is different from the current name then we should update the shortcut address that was used to launch the app
                                    //  To find the shortcut you can use GetStartupInfo() and the .lnk file path is at lpTitle https://stackoverflow.com/questions/6825869/finding-the-shortcut-a-windows-program-was-invoked-from

                                    auto command = _f("cmd /c ping localhost -n 3 > nul & del \"{}\" & move /y \"{}\" \"{}\" & start \"\" /b \"{}\" & exit", exe_full_path, download_full_path, new_file_full_path, new_file_full_path);
                                    if (uncapnc && CreateProcessA(nullptr,
                                        (LPSTR)command.c_str(),
                                        nullptr,
                                        nullptr,
                                        false,
                                        CREATE_NEW_CONSOLE, //TODO: we can use DETACHED_PROCESS if we wanted the console to stay hidden
                                        nullptr,
                                        nullptr,
                                        &info,
                                        &processInfo)) {
                                        CloseHandle(processInfo.hProcess);
                                        CloseHandle(processInfo.hThread);
                                        PostMessage(uncapnc, WM_COMMAND, (WPARAM)MAKELONG(UNCAPNC_CLOSE, 0), 0); //TODO: this does not guarantee that we exit the program, a more robust, if harsh, solution may actually be to force close the process
                                    }
                                    else {
                                        MessageBoxA(NULL,
                                            "Update installation failed, please try again",
                                            "Update Installation Failed",
                                            MB_OK | MB_ICONERROR);
                                    }
                                }
                                return 0;
                            };

                            CreateThread(nullptr, 0, download_and_install, user_extra, 0, nullptr);
                        });
                    }
                }
                catch (...){
                    //TODO: I've seen this generate two messageboxes suggesting that it is retrying, find out where, and only show one messagebox
                    auto download_url = "https://github.com/Bade99/SimpleVeil/releases";
                    MessageBoxA(NULL, //TODO: messagebox that supports hyperlinks
                        _f("We couldn't check for updates.\nPlease look for updated versions directly from:\n\n{}", download_url).c_str(),
                        "Check for Updates Failed", 
                        MB_OK | MB_ICONWARNING);
                }
            }
        }
        else {
            ShowFormattedLastError(res->dwError);
        }
    } break;
    default:
    {
        printf("%s\n", internetStatusToString(dwInternetStatus));
        Sleep(100); //TODO: this is a colossal HACK. I have no clue why, but there's some dwInternetStatus that must have some kind of race condition causing it to fail. I must continue investigating
    } break;
    }
}

_internet_context* check_for_updates() {
    //Thanks: https://gist.github.com/AhnMo/5cf37bbd9a6fa2567f99ac0528eaa185
    internet_context.close_handles();

    auto on_failure = [&]() {
        ShowFormattedLastError();
        internet_context.close_handles();
    };

    HINTERNET internet = InternetOpenW((str(appName) + L"/" + appVersion).c_str(), INTERNET_OPEN_TYPE_PRECONFIG, nullptr, nullptr, INTERNET_FLAG_ASYNC);
    internet_context.internet = internet;
    if (!internet) {
        on_failure();
        return &internet_context;
    }

    HINTERNET internet_session = InternetConnectW(internet, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, nullptr, nullptr, INTERNET_SERVICE_HTTP, INTERNET_FLAG_DONT_CACHE | INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_NO_UI | INTERNET_FLAG_SECURE, (DWORD_PTR)&internet_context);
    internet_context.internet_session = internet_session;
    if (!internet_session) {
        on_failure();
        return &internet_context;
    }

    PCTSTR header_accept_types[] = { L"application/vnd.github+json", NULL };
    auto uri = L"/repos/Bade99/SimpleVeil/releases/latest";
    HINTERNET internet_request = HttpOpenRequestW(internet_session, L"GET", uri, nullptr, nullptr, header_accept_types, INTERNET_FLAG_NO_AUTH | INTERNET_FLAG_NO_CACHE_WRITE | INTERNET_FLAG_NO_COOKIES | INTERNET_FLAG_NO_UI | INTERNET_FLAG_SECURE, (DWORD_PTR)&internet_context);
    internet_context.internet_request = internet_request;
    if (!internet_request) {
        on_failure();
        return &internet_context;
    }

    auto set_callback_res = InternetSetStatusCallbackW(internet_request, InternetStatusCallback);
    if (set_callback_res == INTERNET_INVALID_STATUS_CALLBACK) {
        on_failure();
        return &internet_context;
    }

    constexpr TCHAR additional_headers[] = L"X-GitHub-Api-Version: 2022-11-28";
    auto request_res = HttpSendRequestW(internet_request, additional_headers, ARRAYSIZE(additional_headers) - 1, nullptr, 0);
    //if (request_res != TRUE) ShowFormattedLastError(); Assert(request_res == TRUE); //NOTE: useless for async operations, request_res is always false
    return &internet_context;
}

bool load_rich_edit() {
    if (LoadLibraryW(L"Riched20.dll")) {
        RICHEDIT_CLASS_VERSION = RICHEDIT_CLASS;
        richedit_support = true;
    }
    else if (LoadLibraryW(L"Msftedit.dll")) {
        RICHEDIT_CLASS_VERSION = MSFTEDIT_CLASS;
        richedit_support = true;
    }
    else {
        RICHEDIT_CLASS_VERSION = WC_EDIT;
        richedit_support = false;
    }
    return richedit_support;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/, _In_ int nCmdShow)
{
#ifdef _SHOWCONSOLE
    AllocConsole();
    FILE* ___s; defer{ fclose(___s); };
    freopen_s(&___s, "CONIN$", "r", stdin);
    freopen_s(&___s, "CONOUT$", "w", stdout);
    freopen_s(&___s, "CONOUT$", "w", stderr);
#endif

    //Check that no instance is already running on this user session
    //TODO(fran): from my tests this system is user session independent, check that's true for every case, 
    // eg. admin - admin, admin - normal user, normal user - normal user
    HANDLE single_instance_mutex = CreateMutexW(NULL, TRUE, L"SimpleVeil Single Instance Mutex Franco Badenas Abal");
    if (single_instance_mutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS) { // Found another instance of SimpleVeil already running
        //If an instance already exists try to show its manager to the user
        //INFO: other ways of solving the hwnd finding: http://www.flounder.com/nomultiples.htm
        HWND existingApp = FindWindowW(unCap_wndclass_uncap_nc, appName);
        if (existingApp) { // Attempt to show previous instance
            //NOTE: im on a different thread and everything so I cant ask for the state, it doesnt retrieve something valid, therefore I cant get to my client, TODO IMPORTANT: this is a limitation that must be addressed from my framework, I need a way to get to communicate with my client from the parent, maybe a get client state msg is enough
            //PostMessage(existingApp's client wnd, TRAY, 0, WM_RBUTTONDOWN);
            if (!IsWindowVisible(existingApp)) { //window is minimized
                ShowWindow(existingApp,SW_SHOW);//TODO(fran): for now im content with just showing the wnd, no animation of it coming from the tray //TODO(fran): the veil could be occluded, we should check that the veil is on top too
            }
            else { //window is not minimized //TODO(fran): _but_ could be occluded (in which case we want to SW_SHOW), there doesnt seem to be an easy way to know whether your window is actually visible to the user
                ShowWindow(existingApp, SW_HIDE);
            }
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

#ifdef _AUTOUPDATER
    _internet_context* internet_context = nullptr; defer{ if (internet_context) internet_context->close_handles(); };
    auto current_time = time(nullptr);
    auto days = [](u32 cnt) {return cnt * 24 * 60 * 60; };
    i64 update_period;
    switch (uncap_cl.update_check_frequency) {
    case UpdateCheckFrequency::Always: update_period = 0; break;
    case UpdateCheckFrequency::Daily: update_period = days(1); break;
    case UpdateCheckFrequency::Weekly: update_period = days(7); break;
    case UpdateCheckFrequency::Monthly: update_period = days(30); break;
    case UpdateCheckFrequency::Never: update_period = _I64_MAX; break;
    }
    if (current_time - uncap_cl.last_update_check > update_period) {
        uncap_cl.last_update_check = current_time;
        internet_context = check_for_updates();
    }
#endif

    init_wndclass_unCap_uncapcl(hInstance);

    init_wndclass_unCap_uncapnc(hInstance);

    init_wndclass_unCap_button(hInstance);

    init_wndclass_unCap_toggle_button(hInstance);

    init_wndclass_SimpleVeil_veil(hInstance,unCap_colors.Veil);

    init_wndclass_unCap_edit_oneline(hInstance);

    updater::init_wndclass(hInstance);

    settings::init_wndclass(hInstance);

    load_rich_edit();

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
    nclpparam.is_main_wnd = true;

    //NOTE(fran): if you ask for OVERLAPPEDWINDOW someone else draws your nc area
    uncapnc = CreateWindowEx(WS_EX_CONTROLPARENT | WS_EX_TOPMOST | WS_EX_APPWINDOW, 
        unCap_wndclass_uncap_nc, NULL,
        WS_THICKFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        uncapnc_rc.left, uncapnc_rc.top, RECTWIDTH(uncapnc_rc), RECTHEIGHT(uncapnc_rc),
        nullptr, nullptr, hInstance, &nclpparam);

    if (!uncapnc) return FALSE;

    ShowWindow(uncapnc, nCmdShow);
    UpdateWindow(uncapnc);

    unCapNcLpParam updaterParams;
    updaterParams.client_class_name = updater::wndclass;
    updaterParams.client_lp_param = nullptr;
    updaterParams.can_minimize = false;

    updaternc = CreateWindowEx(WS_EX_CONTROLPARENT | WS_EX_TOPMOST | WS_EX_APPWINDOW,
        unCap_wndclass_uncap_nc, nullptr,
        WS_THICKFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
        CW_USEDEFAULT, CW_USEDEFAULT, 50 * 16, 50 * 9,
        nullptr, nullptr, hInstance, &updaterParams);

    if (!updaternc) return FALSE;

    UpdateWindow(updaternc);

    HACCEL hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDC_SIMPLEVEIL));

    MSG msg;

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    str serialized;
    _BeginSerialize();
    serialized += _serialize_struct(lang_mgr);
    serialized += _serialize_struct(uncap_cl);
    serialized += _serialize_struct(unCap_colors);

    save_to_file_serialized(serialized);

    return (int)msg.wParam;
}
