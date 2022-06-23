#ifndef __HEXCOLORFINDER_H_
#define __HEXCOLORFINDER_H_

#if _MSC_VER > 1000
#pragma once
#endif

#include "resource.h"
#include <vector>
#include <string>
#include <iostream>
#include <strstream>
#include <fstream>

// global structures
struct Preset
{
	std::string name;
	std::vector<COLORREF> colors;
};

// RT_TOOLBAR resource
struct TOOLBARDATA {
	WORD wVersion;           // should be 1
	WORD wWidth;             // bitmap width
	WORD wHeight;            // height 
	WORD wItemCount;         // num items
	WORD items[1];
};

struct DragData
{
	enum
	{
		DragNone,
		DragInRect, // waiting to move mouse out of rectangle to begin dragging
		DragDragging,
	} enState;

	RECT rcDrag;
	int nDragCursor;
	int nColorIndex;

	DragData()
	{
		enState = DragNone;
		SetRect(&rcDrag,0,0,0,0);
		nDragCursor = -1;
		nColorIndex = -1;
	}
};

struct NMCHANGECOLOR
{
	NMHDR hdr;
	COLORREF clrnew;
};

// global constants
#define HCS_SELECTED	0x00000001L
#define HCS_RCLICK		0x00000002L
#define HCN_USERCHANGE	-1400L

#define COLORPOPUPID_FIRST		ID_MANIPULATE_INVERT
#define	COLORPOPUPID_LAST		ID_QUICKCOLORS_COMMON_DARKBLUE

// global macros

// global variables
BOOL g_bInFunction; // prevent recursion
BOOL g_bLinkScrollbars;
BOOL g_bNewColorScreenPick; // create a new color instead of modifying the current one
							// when using screen color picker

std::vector<COLORREF> g_vColorInfo;
std::vector<Preset> g_vPresets;
BOOL g_bPresetsModified;
int g_nSelPreset;

DragData g_dragData;

char g_strTitle[255];
HINSTANCE g_hInstance;
HWND g_dlgMain;
HIMAGELIST g_imlTB;

// global functions
int FitByte(int n){ return max(0,min(255,n)); }
BOOL RegisterClasses();
BOOL InitInstance();
BOOL ExitInstance();
BOOL LoadRegistryData();
BOOL SaveRegistryData();

void LoadPresets();
BOOL SavePresets(BOOL bAskIfModified);
BOOL SwitchToPreset(int nIndex);

std::string ColorToHex(COLORREF clr, BOOL bPoundSign = TRUE);
COLORREF ColorFromHex(std::string strHex);

BOOL IsSelected(UINT nControlID);
void SetSelected(UINT nControlID, BOOL bSelected);

COLORREF GetColor(UINT nControlID);
void SetColor(UINT nControlID, COLORREF clr);

void SetEditColor(COLORREF clrEdit, BOOL bRefreshGradient, UINT nSourceID);
void SetEditColorPart(int nPart, BYTE nValue, UINT nSourceID);
void SelectLeftOrRightColor(BOOL bRight, UINT nSourceID);
void UpdateControls(UINT nSourceID);
void UpdatePresets(BOOL bSelector = TRUE, BOOL bColors = TRUE, BOOL bToolbar = TRUE);

void HandleScroll(HWND wnd, WPARAM params);

void DrawTransparentBitmap(HDC destdc, int x, int y, HBITMAP hbmp,
	COLORREF clrTransparent);
void DrawColorBox(HDC destdc, LPRECT prc, BOOL bGradient, BOOL bSelected,
	COLORREF clr1, COLORREF clr2 = 0);

INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT uiMessage, WPARAM wParam, LPARAM lParam);
INT_PTR CALLBACK PresetDlgProc(HWND hDlg, UINT uiMessage, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ColorProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK GradientProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ScreenPickerProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ColorListProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam);

INT_PTR CALLBACK AboutProc(HWND hDlg, UINT uiMessage, WPARAM wParam, LPARAM lParam);

#endif
