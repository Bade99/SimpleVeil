#pragma once
#include <windows.h>
#include <map>
#include <strsafe.h> //for stringcchcopy
#include <commctrl.h>
#include "LANGUAGE_MANAGER.h"

//Needs Common Controls library to be loaded
class TRAY_HANDLER
{
public:
	/// <summary>
	/// Retrieves the current instance of the Tray Handler
	/// </summary>
	static TRAY_HANDLER& Instance()
	{
		static TRAY_HANDLER instance;

		return instance;
	}
	TRAY_HANDLER(TRAY_HANDLER const&) = delete;
	void operator=(TRAY_HANDLER const&) = delete;

	/// <summary>
	/// Creates a new tray icon and loads it into the tray
	/// <para>Your hwnd must have its hInstance parameter set</para>
	/// </summary>
	/// <param name="hwnd">The window whose procedure will receive messages from the tray icon</param>
	/// <param name="uID">Can be any unique number that you assign</param>
	/// <param name="iconID">The icon's resource ID, for use in MAKEINTRESOURCE(iconID)</param>
	/// <param name="uCallbackMessage">The message code that will be sent to the hwnd when some event happens on the tray object</param>
	/// <param name="messageID">The message to show on mouseover, for use in MAKEINTRESOURCE(messageID)</param>
	/// <returns>TRUE if created, FALSE if failed</returns>
	BOOL CreateTrayIcon(HWND hwnd, UINT uID, LONG iconID, UINT uCallbackMessage, WORD messageID);

	/// <summary>
	/// Destroys the tray object associated with that hwnd and uID
	/// </summary>
	/// <param name="hwnd">The window used in CreateTrayIcon</param>
	/// <param name="uID">The uID used in CreateTrayIcon</param>
	/// <returns>TRUE if the tray icon existed and was deleted, FALSE otherwise</returns>
	BOOL DestroyTrayIcon(HWND hwnd, UINT uID);

	/// <summary>
	/// Updates language of the text shown on mouseover
	/// <para>TODO(fran): integration with LANGUAGE_MANAGER</para>
	/// </summary>
	BOOL UpdateTrayTips();


private:
	TRAY_HANDLER();
	~TRAY_HANDLER();

	//·Each tray icon (notifyicondata) is mapped by hwnd+uid (into uint)
	std::map<std::pair<HWND, UINT>, std::pair<NOTIFYICONDATA, WORD>> TrayElements;

	BOOL UpdateTrayTip(NOTIFYICONDATA notification, WORD messageID);
};

BOOL TRAY_HANDLER::CreateTrayIcon(HWND hwnd, UINT uID, LONG iconID, UINT uCallbackMessage, WORD messageID)
{

	HICON tray_icon = NULL;
	LoadIconMetric((HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE), MAKEINTRESOURCE(iconID), LIM_SMALL, &tray_icon);

	if (!tray_icon) return FALSE;

	NOTIFYICONDATA notification;
	notification.cbSize = sizeof(NOTIFYICONDATA);//TODO(fran): this is not quite the way
	notification.hWnd = hwnd;
	notification.uID = uID;
	notification.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP | NIF_SHOWTIP;//TODO(fran): check this
	notification.uCallbackMessage = uCallbackMessage;
	notification.hIcon = tray_icon;
	notification.dwState = NIS_SHAREDICON;//TODO(fran): check this
	notification.dwStateMask = NIS_SHAREDICON;
	notification.szInfo[0] = 0;
	notification.uVersion = NOTIFYICON_VERSION_4;//INFO(fran): this changes the message format, but not when nif_showtip is enabled, I think
	notification.szInfoTitle[0] = 0;
	notification.dwInfoFlags = NIIF_NONE;
	//notification.guidItem = ;
	notification.hBalloonIcon = NULL;
	StringCchCopy(notification.szTip, ARRAYSIZE(notification.szTip), RCS(messageID));//INFO: msg MUST be less than 128 chars
	BOOL ret = Shell_NotifyIcon(NIM_ADD, &notification);
	if (ret)
		this->TrayElements[std::make_pair(hwnd, uID)] = std::make_pair(notification, messageID);//TODO(fran): should we check whether there is an element in that key already?
	return ret;
}

BOOL TRAY_HANDLER::DestroyTrayIcon(HWND hwnd, UINT uID) //TODO(fran): should go in destructor too
{
	try {
		std::pair<NOTIFYICONDATA, WORD> notif_msg = this->TrayElements.at(std::make_pair(hwnd, uID));
		BOOL ret = Shell_NotifyIcon(NIM_DELETE, &notif_msg.first);
		DestroyIcon(notif_msg.first.hIcon);
		this->TrayElements.erase(std::make_pair(hwnd, uID));
		return ret;
	}
	catch (...) { return FALSE; } //.at() throws
}

BOOL TRAY_HANDLER::UpdateTrayTips()
{
	for (auto const& hwnd_msg : this->TrayElements)
		this->UpdateTrayTip(hwnd_msg.second.first, hwnd_msg.second.second);
	return TRUE;
}

BOOL TRAY_HANDLER::UpdateTrayTip(NOTIFYICONDATA notification, WORD messageID)
{
	StringCchCopy(notification.szTip, ARRAYSIZE(notification.szTip), RCS(messageID));
	Shell_NotifyIcon(NIM_MODIFY, &notification);
	return 0;
}

TRAY_HANDLER::TRAY_HANDLER()
{
}


TRAY_HANDLER::~TRAY_HANDLER()
{
}

