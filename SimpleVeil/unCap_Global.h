#pragma once
#include <Windows.h>
#include "unCap_Reflection.h"
#include "unCap_Serialization.h"

union UNCAP_COLORS {//TODO(fran): HBRUSH Border
	struct {
		//TODO(fran): macro magic to auto generate the appropriately sized HBRUSH array
		//TODO(fran): add _disabled for at least txt,bk,border
#define foreach_color(op) \
		op(HBRUSH,ControlBk,CreateSolidBrush(RGB(40, 41, 35))) \
		op(HBRUSH,ControlBkPush,CreateSolidBrush(RGB(0, 110, 200))) \
		op(HBRUSH,ControlBkMouseOver,CreateSolidBrush(RGB(0, 120, 215))) \
		op(HBRUSH,ControlTxt,CreateSolidBrush(RGB(248, 248, 242))) \
		op(HBRUSH,ControlTxt_Inactive,CreateSolidBrush(RGB(208, 208, 202))) \
		op(HBRUSH,ControlMsg,CreateSolidBrush(RGB(248, 230, 0))) \
		op(HBRUSH,Scrollbar,CreateSolidBrush(RGB(148, 148, 142))) \
		op(HBRUSH,ScrollbarMouseOver,CreateSolidBrush(RGB(188, 188, 182))) \
		op(HBRUSH,ScrollbarBk,CreateSolidBrush(RGB(50, 51, 45))) \
		op(HBRUSH,Img,CreateSolidBrush(RGB(228, 228, 222))) \
		op(HBRUSH,Img_Inactive,CreateSolidBrush(RGB(198, 198, 192))) \
		op(HBRUSH,CaptionBk,CreateSolidBrush(RGB(20, 21, 15))) \
		op(HBRUSH,CaptionBk_Inactive,CreateSolidBrush(RGB(60, 61, 65))) \
		op(HBRUSH,ControlBk_Disabled,CreateSolidBrush(RGB(35, 36, 30))) \
		op(HBRUSH,ControlTxt_Disabled,CreateSolidBrush(RGB(128, 128, 122))) \
		op(HBRUSH,Img_Disabled,CreateSolidBrush(RGB(98, 98, 92))) \
		op(HBRUSH,Veil,CreateSolidBrush(RGB(0, 0, 0))) \
		op(HBRUSH,HotkeyTxt_Accepted,CreateSolidBrush(RGB(0, 225, 0))) \
		op(HBRUSH,HotkeyTxt_Rejected,CreateSolidBrush(RGB(225, 0, 0))) \
		op(HBRUSH, ToggleBk_On, CreateSolidBrush(RGB(0, 110, 200))) \
		op(HBRUSH, ToggleBk_Off, CreateSolidBrush(RGB(200, 0, 60))) \
		op(HBRUSH, ToggleBkPush_On, CreateSolidBrush(RGB(0, 95, 190))) \
		op(HBRUSH, ToggleBkPush_Off, CreateSolidBrush(RGB(190, 0, 45))) \
		op(HBRUSH, ToggleBkMouseOver_On, CreateSolidBrush(RGB(0, 120, 215))) \
		op(HBRUSH, ToggleBkMouseOver_Off, CreateSolidBrush(RGB(210, 0, 75))) \
		op(HBRUSH, TrackbarThumb, CreateSolidBrush(RGB(228, 228, 222))) \
		op(HBRUSH, TrackbarChannelBk, CreateSolidBrush(RGB(40, 41, 35))) \
		op(HBRUSH, TrackbarThumbChannelBk, CreateSolidBrush(RGB(80, 82, 70))) \
		op(HBRUSH, TrackbarChannelBorder, CreateSolidBrush(RGB(120, 123, 105))) \
		op(HBRUSH, TrackbarThumbChannelBorder, CreateSolidBrush(RGB(160, 164, 140))) \

		foreach_color(_generate_member_no_default_init);

	};
	HBRUSH brushes[18];//REMEMBER to update

	_generate_default_struct_serialize(foreach_color);

	_generate_default_struct_deserialize(foreach_color);
};

void default_colors_if_not_set(UNCAP_COLORS* c) {
#define _default_initialize(type, name,value) if(!c->name) c->name = value;
	foreach_color(_default_initialize);
#undef _default_initialize
}

extern UNCAP_COLORS unCap_colors;

union UNCAP_FONTS {
	struct {
		HFONT General;
		HFONT Menu;
	};
	HFONT fonts[2];//REMEMBER to update
};

extern UNCAP_FONTS unCap_fonts;