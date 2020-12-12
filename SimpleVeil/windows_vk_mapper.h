#pragma once
#include "unCap_Platform.h"
#include "unCap_Reflection.h"
#include "unCap_Helpers.h"
#include <map>

//Many vks cant be mapped to strings by GetKeyNameText, mainly extended ones, MapVirtualKeyEx is able to produce the appropiate scancode but GetKeyNameText simply refuses to work with extended keys. Gotta do it manually, which means we'll only return strings in English


// Virtual Keys, Standard Set
static std::map<u8, const cstr*> vkTranslation = {
{0x00, _t("")},
{0x01, _t("Left click")},
{0x02, _t("Right click")},
{0x03, _t("Control-break processing")},
{0x04, _t("Middle click")},
{0x05, _t("X1 click")},
{0x06, _t("X2 click")},
{0x07, _t("undefined 0x07")},
{0x08, _t("Backspace")},
{0x09, _t("Tab")},
{0x0A, _t("undefined 0x0A")},
{0x0B, _t("undefined 0x0B")},
{0x0C, _t("Clear")},
{0x0D, _t("Enter")},
{0x0E, _t("undefined 0x0E")},
{0x0F, _t("undefined 0x0F")},
{0x10, _t("Shift")},
{0x11, _t("Ctrl")},
{0x12, _t("Alt")},
{0x13, _t("Pause") },
{0x14, _t("Caps lock") },
{0x15, _t("Kana mode") },
{0x16, _t("IME on")},
{0x17, _t("Junja mode")},
{0x18, _t("Final mode")},
{0x19, _t("Hanja mode")},
{0x19, _t("Kanji mode")},
{0x1A, _t("IME off")},
{0x1B, _t("Escape")},
{0x1C, _t("IME convert")},
{0x1D, _t("IME nonconvert")},
{0x1E, _t("IME accept")},
{0x1F, _t("IME mode change")},
{0x20, _t("Space")},
{0x21, _t("Page up")},
{0x22, _t("Page down")},
{0x23, _t("End")},
{0x24, _t("Home")},
{0x25, _t("Left arrow")},
{0x26, _t("Up arrow")},
{0x27, _t("Right arrow")},
{0x28, _t("Down arrow")},
{0x29, _t("Select")},
{0x2A, _t("Print")},
{0x2B, _t("Execute")},
{0x2C, _t("Print screen")},
{0x2D, _t("Insert")},
{0x2E, _t("Delete")},
{0x2F, _t("Help")},
{0x30, _t("0")},
{0x31, _t("1")},
{0x32, _t("2")},
{0x33, _t("3")},
{0x34, _t("4")},
{0x35, _t("5")},
{0x36, _t("6")},
{0x37, _t("7")},
{0x38, _t("8")},
{0x39, _t("9")},
{0x3A, _t("undefined 0x3A")},
{0x40, _t("undefined 0x40")},
{0x41, _t("A")},
{0x42, _t("B")},
{0x43, _t("C")},
{0x44, _t("D")},
{0x45, _t("E")},
{0x46, _t("F")},
{0x47, _t("G")},
{0x48, _t("H")},
{0x49, _t("I")},
{0x4A, _t("J")},
{0x4B, _t("K")},
{0x4C, _t("L")},
{0x4D, _t("M")},
{0x4E, _t("N")},
{0x4F, _t("O")},
{0x50, _t("P")},
{0x51, _t("Q")},
{0x52, _t("R")},
{0x53, _t("S")},
{0x54, _t("T")},
{0x55, _t("U")},
{0x56, _t("V")},
{0x57, _t("W")},
{0x58, _t("X")},
{0x59, _t("Y")},
{0x5A, _t("Z")},
{0x5B, _t("Windows left")},
{0x5C, _t("Windows right")},
{0x5D, _t("Applications")},
{0x5E, _t("undefined 0x5E")},
{0x5F, _t("Sleep")},
{0x60, _t("Keypad 0")},
{0x61, _t("Keypad 1")},
{0x62, _t("Keypad 2")},
{0x63, _t("Keypad 3")},
{0x64, _t("Keypad 4")},
{0x65, _t("Keypad 5")},
{0x66, _t("Keypad 6")},
{0x67, _t("Keypad 7")},
{0x68, _t("Keypad 8")},
{0x69, _t("Keypad 9")},
{0x6A, _t("Multiply")},
{0x6B, _t("Add")},
{0x6C, _t("Separator")},
{0x6D, _t("Subtract")},
{0x6E, _t("Decimal")},
{0x6F, _t("Divide")},
{0x70, _t("F1")},
{0x71, _t("F2")},
{0x72, _t("F3")},
{0x73, _t("F4")},
{0x74, _t("F5")},
{0x75, _t("F6")},
{0x76, _t("F7")},
{0x77, _t("F8")},
{0x78, _t("F9")},
{0x79, _t("F10")},
{0x7A, _t("F11")},
{0x7B, _t("F12")},
{0x7C, _t("F13")},
{0x7D, _t("F14")},
{0x7E, _t("F15")},
{0x7F, _t("F16")},
{0x80, _t("F17")},
{0x81, _t("F18")},
{0x82, _t("F19")},
{0x83, _t("F20")},
{0x84, _t("F21")},
{0x85, _t("F22")},
{0x86, _t("F23")},
{0x87, _t("F24")},
{0x88, _t("UI nav view")},
{0x89, _t("UI nav menu")},
{0x8A, _t("UI nav up")},
{0x8B, _t("UI nav down")},
{0x8C, _t("UI nav left")},
{0x8D, _t("UI nav right")},
{0x8E, _t("UI nav accept")},
{0x8F, _t("UI nav cancel")},
{0x90, _t("Num lock")},
{0x91, _t("Scroll lock")},
{0x92, _t("OEM specific 0x92") },
{0x93, _t("OEM specific 0x93") },
{0x94, _t("OEM specific 0x94") },
{0x95, _t("OEM specific 0x95") },
{0x96, _t("OEM specific 0x96") },
{0x97, _t("undefined 0x97") },
{0x98, _t("undefined 0x98") },
{0x99, _t("undefined 0x99") },
{0x9A, _t("undefined 0x9A") },
{0x9B, _t("undefined 0x9B") },
{0x9C, _t("undefined 0x9C") },
{0x9D, _t("undefined 0x9D") },
{0x9E, _t("undefined 0x9E") },
{0x9F, _t("undefined 0x9F") },
// VK_L* and VK_R* for Alt, Ctrl and Shift vks are used only as params to GetAsyncKeyState(), GetKeyState(). No other API or msg uses this
{0xA0, _t("Left Shift") },
{0xA1, _t("Right Shift") },
{0xA2, _t("Left Control") },
{0xA3, _t("Right Control") },
{0xA4, _t("Left Alt") },
{0xA5, _t("Right Alt") },
{0xA6, _t("Browser Back")},
{0xA7, _t("Browser Forward")},
{0xA8, _t("Browser Refresh")},
{0xA9, _t("Browser Stop")},
{0xAA, _t("Browser Search")},
{0xAB, _t("Browser Favorites")},
{0xAC, _t("Browser Home")},
{0xAD, _t("Mute")},
{0xAE, _t("Volume Down")},
{0xAF, _t("Volume Up")},
{0xB0, _t("Next Track")},
{0xB1, _t("Prev Track")},
{0xB2, _t("Stop")},
{0xB3, _t("Play/Pause")},
{0xB4, _t("Start Mail")},
{0xB5, _t("Select Media")},
{0xB6, _t("Start app 1")},
{0xB7, _t("Start app 2")},
{0xB8, _t("undefined 0xB8") },
{0xB9, _t("undefined 0xB9") },
{0xBA, _t("OEM specific 0xBA")}, // ';:' for US
{0xBB, _t("'+'")}, // '+' any country
{0xBC, _t("','")}, // ',' any country
{0xBD, _t("'-'")}, // '-' any country
{0xBE, _t("'.'")}, // '.' any country
{0xBF, _t("OEM specific 0xBF")}, // '/?' for US
{0xC0, _t("OEM specific 0xC0")}, // '`~' for US
{0xC1, _t("undefined 0xC1")},
{0xC2, _t("undefined 0xC2")},
{0xC3, _t("Gamepad A")}, // reserved
{0xC4, _t("Gamepad B")}, // reserved
{0xC5, _t("Gamepad X")}, // reserved
{0xC6, _t("Gamepad Y")}, // reserved
{0xC7, _t("Gamepad Right Shoulder")}, // reserved
{0xC8, _t("Gamepad Left Shoulder")}, // reserved
{0xC9, _t("Gamepad Left Trigger")}, // reserved
{0xCA, _t("Gamepad Right Trigger")}, // reserved
{0xCB, _t("Gamepad Dpad Up")}, // reserved
{0xCC, _t("Gamepad Dpad Down")}, // reserved
{0xCD, _t("Gamepad Dpad Left")}, // reserved
{0xCE, _t("Gamepad Dpad Right")}, // reserved
{0xCF, _t("Gamepad Menu")}, // reserved
{0xD0, _t("Gamepad View")}, // reserved
{0xD1, _t("Gamepad Left Thumb Button")}, // reserved
{0xD2, _t("Gamepad Right Thumb Button")}, // reserved
{0xD3, _t("Gamepad Left Thumb Up")}, // reserved
{0xD4, _t("Gamepad Left Thumb Down")}, // reserved
{0xD5, _t("Gamepad Left Thumb Right")}, // reserved
{0xD6, _t("Gamepad Left Thumb Left")}, // reserved
{0xD7, _t("Gamepad Right Thumb Up")}, // reserved
{0xD8, _t("Gamepad Right Thumb Down")}, // reserved
{0xD9, _t("Gamepad Right Thumb Right")}, // reserved
{0xDA, _t("Gamepad Right Thumb Left")}, // reserved
{0xDB, _t("OEM specific 0xDB")}, //  '[{' for US
{0xDC, _t("OEM specific 0xDC")}, //  '\|' for US
{0xDD, _t("OEM specific 0xDD")}, //  ']}' for US
{0xDE, _t("OEM specific 0xDE")}, //  ''"' for US
{0xDF, _t("OEM specific 0xDF")},
{0xE0, _t("undefined 0xE0")},
{0xE1, _t("OEM specific 0xE1")},  //  'AX' key on Japanese AX kbd
{0xE2, _t("OEM specific 0xE2")},  //  " < > " or "\ | " on RT 102-key kbd.
{0xE3, _t("OEM specific 0xE3")},  //  Help key on ICO
{0xE4, _t("OEM specific 0xE4")},  //  00 key on ICO
{0xE5, _t("IME process")},
{0xE6, _t("OEM specific 0xE6")}, // VK_ICO_CLEAR
{0xE7, _t("Packet")}, // Some crazy unicode stuff
{0xE8, _t("undefined 0xE8")},
{0xE9, _t("OEM specific 0xE9")}, // VK_OEM_RESET      
{0xEA, _t("OEM specific 0xEA")}, // VK_OEM_JUMP       
{0xEB, _t("OEM specific 0xEB")}, // VK_OEM_PA1        
{0xEC, _t("OEM specific 0xEC")}, // VK_OEM_PA2        
{0xED, _t("OEM specific 0xED")}, // VK_OEM_PA3        
{0xEE, _t("OEM specific 0xEE")}, // VK_OEM_WSCTRL     
{0xEF, _t("OEM specific 0xEF")}, // VK_OEM_CUSEL      
{0xF0, _t("OEM specific 0xF0")}, // VK_OEM_ATTN       
{0xF1, _t("OEM specific 0xF1")}, // VK_OEM_FINISH     
{0xF2, _t("OEM specific 0xF2")}, // VK_OEM_COPY       
{0xF3, _t("OEM specific 0xF3")}, // VK_OEM_AUTO       
{0xF4, _t("OEM specific 0xF4")}, // VK_OEM_ENLW       
{0xF5, _t("OEM specific 0xF5")}, // VK_OEM_BACKTAB    
{0xF6, _t("Attn")},
{0xF7, _t("CrSel")},
{0xF8, _t("ExSel")},
{0xF9, _t("Erase EOF")},
{0xFA, _t("Play")},
{0xFB, _t("Zoom")},
{0xFC, _t("undefined 0xFC")}, //VK_NONAME
{0xFD, _t("PA1")},
{0xFE, _t("Clear")},
{0xFF, _t("undefined 0xFF")}
};

//Always returns a valid pointer, when vk==0 it will return just one null character
const cstr* vkToString(u8 vk) { //The user should NOT try to free the pointer
	const cstr* res = vkTranslation[vk];
	return res;
}