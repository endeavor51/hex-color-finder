#include "stdafx.h"
#include "resource.h"

#include "HexColorFinder.h"

// TODO: fix problem with Ctrl+Drag not updating things

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR lpCmdLine, int nCmdShow)
{
	{
		strcpy(g_strTitle,"Hex Color Finder");

		// get version info
		char strFile[MAX_PATH];
		GetModuleFileName(NULL,strFile,MAX_PATH);

		DWORD dwInfoSize = 0;
		if (dwInfoSize = GetFileVersionInfoSize(strFile,&dwInfoSize))
		{
			void* pInfo = malloc(dwInfoSize);

			if (GetFileVersionInfo(strFile,0,dwInfoSize,pInfo))
			{
				UINT nFFSize;
				VS_FIXEDFILEINFO* pVSFFI;
				if (VerQueryValue(pInfo,"\\",(LPVOID*)&pVSFFI,&nFFSize))
				{
					char strTemp[10];
					struct
					{
						UINT minor : 16;
						UINT major : 16;
					} version;

					memcpy(&version,&pVSFFI->dwProductVersionMS,
						sizeof(pVSFFI->dwProductVersionMS));

					sprintf(strTemp," %u.%u",version.major,version.minor);
					strcat(g_strTitle,strTemp);
				}
			}

			free(pInfo);
		}
	}

	// init global data
	g_hInstance = hInstance;
	g_imlTB = NULL;

	g_vColorInfo.push_back(0x00000000);
	g_vColorInfo.push_back(0x00FFFFFF);
	g_vColorInfo.push_back(0x00000000);

	g_nSelPreset = -1;
	g_bPresetsModified = FALSE;

	// LoadRegistryData() called in init dialog
	LoadPresets();

	// init common controls
	INITCOMMONCONTROLSEX icce;
	icce.dwSize = sizeof(icce);
	icce.dwICC = ICC_BAR_CLASSES;
	InitCommonControlsEx(&icce);

	// register classes
	RegisterClasses();

	// perform app initialization
	if (!InitInstance())
		return 0;

	HACCEL hAccelTable = LoadAccelerators(hInstance,MAKEINTRESOURCE(IDR_MAIN));

	// main message loop
	MSG msg;
	BOOL bRet;
	while ((bRet = GetMessage(&msg,NULL,0,0)) != 0)
	{
		if (bRet == -1)
		{
			// process GetMessage errors
		}
		else if (!IsWindow(g_dlgMain) || (!TranslateAccelerator(g_dlgMain,hAccelTable,&msg) &&
			!IsDialogMessage(g_dlgMain,&msg)))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}

		if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE)
			PostMessage(msg.hwnd,WM_CANCELMODE,0,0);
	}

	// SaveRegistryData() called on WM_CLOSE

	// perform app cleanup
	if (!ExitInstance())
		return 0;

	return msg.wParam;
}

BOOL RegisterClasses()
{
	// Use the following template for class registration

	WNDCLASSEX wcex; memset(&wcex,0,sizeof(wcex));

	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.style = CS_DBLCLKS;
	wcex.lpfnWndProc = (WNDPROC)ColorProc;
	wcex.hInstance = g_hInstance;
	wcex.hCursor = LoadCursor(NULL,IDC_ARROW);
	wcex.lpszClassName = "HCF_Color";
	if (!RegisterClassEx(&wcex))
		return FALSE;

	wcex.lpfnWndProc = (WNDPROC)ScreenPickerProc;
	wcex.lpszClassName = "HCF_ScreenPicker";
	if (!RegisterClassEx(&wcex))
		return FALSE;

	wcex.hCursor = LoadCursor(NULL,IDC_CROSS);
	wcex.lpfnWndProc = (WNDPROC)GradientProc;
	wcex.lpszClassName = "HCF_Gradient";
	if (!RegisterClassEx(&wcex))
		return FALSE;

	return TRUE;
}

BOOL InitInstance()
{
	g_dlgMain = CreateDialog(g_hInstance,MAKEINTRESOURCE(IDD_MAIN),NULL,MainDlgProc);

	if (!g_dlgMain)
		return FALSE;

	ShowWindow(g_dlgMain,SW_SHOW);
	UpdateWindow(g_dlgMain);

	return TRUE;
}

BOOL ExitInstance()
{
	return TRUE;
}

BOOL LoadRegistryData()
{
	// start with default values
	g_bLinkScrollbars = FALSE;
	g_bNewColorScreenPick = FALSE;

	HKEY hRootKey = NULL;
	if (RegOpenKeyEx(HKEY_CURRENT_USER,"Software\\NZ Software\\Hex Color Finder",0,
		KEY_READ,&hRootKey) == ERROR_SUCCESS)
	{
		HKEY hSubKey = NULL;
		DWORD dwSize = 0;

		// get main window placement and always on top state
		if (RegOpenKeyEx(hRootKey,"View\\Main Window",0,
			KEY_READ,&hSubKey) == ERROR_SUCCESS)
		{
			WINDOWPLACEMENT wp; dwSize = sizeof(wp);
			if (RegQueryValueEx(hSubKey,"WindowPlacement",NULL,NULL,(BYTE*)&wp,
				&dwSize) == ERROR_SUCCESS)
				SetWindowPlacement(g_dlgMain,&wp);

			DWORD dwTemp = 0; dwSize = sizeof(dwTemp);
			if (RegQueryValueEx(hSubKey,"AlwaysOnTop",NULL,NULL,(BYTE*)&dwTemp,
				&dwSize) == ERROR_SUCCESS && dwTemp == 1)
			{
				// fake click the button
				SendMessage(g_dlgMain,WM_COMMAND,
					MAKEWPARAM(IDC_CHK_TOPMOST,BN_CLICKED),
					(LPARAM)GetDlgItem(g_dlgMain,IDC_CHK_TOPMOST));
			}

			RegCloseKey(hSubKey);
		}

		// get options
		if (RegOpenKeyEx(hRootKey,"View\\Options",0,
			KEY_READ,&hSubKey) == ERROR_SUCCESS)
		{
			DWORD dwTemp = 0; dwSize = sizeof(dwTemp);
			if (RegQueryValueEx(hSubKey,"LinkScrollbars",NULL,NULL,(BYTE*)&dwTemp,
				&dwSize) == ERROR_SUCCESS)
			{
				g_bLinkScrollbars = (BOOL)dwTemp;
				CheckMenuItem(GetSubMenu(GetMenu(g_dlgMain),3),ID_OPTIONS_LINK_SCROLLBARS,
					MF_BYCOMMAND | (dwTemp ? MF_CHECKED : MF_UNCHECKED));
			}

			if (RegQueryValueEx(hSubKey,"NewColorScreenPick",NULL,NULL,(BYTE*)&dwTemp,
				&dwSize) == ERROR_SUCCESS)
			{
				g_bNewColorScreenPick = (BOOL)dwTemp;
				CheckMenuItem(GetSubMenu(GetMenu(g_dlgMain),3),ID_OPTIONS_NEW_COLOR_SCREEN_PICK,
					MF_BYCOMMAND | (dwTemp ? MF_CHECKED : MF_UNCHECKED));
			}

			RegCloseKey(hSubKey);
		}

		RegCloseKey(hRootKey);
	}

	return TRUE;
}

BOOL SaveRegistryData()
{
	HKEY hRootKey = NULL;
	if (RegCreateKeyEx(HKEY_CURRENT_USER,"Software\\NZ Software\\Hex Color Finder",0,
		NULL,0,KEY_WRITE,NULL,&hRootKey,NULL) == ERROR_SUCCESS)
	{
		HKEY hSubKey = NULL;

		// set main window placement and always on top state
		if (RegCreateKeyEx(hRootKey,"View\\Main Window",0,
			NULL,0,KEY_WRITE,NULL,&hSubKey,NULL) == ERROR_SUCCESS)
		{
			WINDOWPLACEMENT wp; GetWindowPlacement(g_dlgMain,&wp);
			RegSetValueEx(hSubKey,"WindowPlacement",NULL,REG_BINARY,(BYTE*)&wp,
				sizeof(wp));

			DWORD dwTemp = (DWORD)GetProp(GetDlgItem(g_dlgMain,IDC_CHK_TOPMOST),"SET");
			RegSetValueEx(hSubKey,"AlwaysOnTop",NULL,REG_DWORD,(BYTE*)&dwTemp,
				sizeof(dwTemp));

			RegCloseKey(hSubKey);
		}

		// get options
		if (RegCreateKeyEx(hRootKey,"View\\Options",0,
			NULL,0,KEY_WRITE,NULL,&hSubKey,NULL) == ERROR_SUCCESS)
		{
			DWORD dwTemp = (DWORD)g_bLinkScrollbars;
			RegSetValueEx(hSubKey,"LinkScrollbars",NULL,REG_DWORD,(BYTE*)&dwTemp,
				sizeof(dwTemp));

			dwTemp = (DWORD)g_bNewColorScreenPick;
			RegSetValueEx(hSubKey,"NewColorScreenPick",NULL,REG_DWORD,(BYTE*)&dwTemp,
				sizeof(dwTemp));

			RegCloseKey(hSubKey);
		}

		RegCloseKey(hRootKey);
	}

	return TRUE;
}

BOOL LoadToolbar(HWND wndToolbar, UINT nToolbarID)
{
	// determine location of the bitmap in the resources
	HRSRC hRsrc = FindResource(g_hInstance,MAKEINTRESOURCE(nToolbarID),
		MAKEINTRESOURCE(241)); // 241 = RT_TOOLBAR
	if (hRsrc == NULL)
		return FALSE;

	HGLOBAL hGlobal = LoadResource(g_hInstance,hRsrc);
	if (hGlobal == NULL)
		return FALSE;

	TOOLBARDATA* pData = (TOOLBARDATA*)LockResource(hGlobal);
	if (pData == NULL || pData->wVersion != 1)
		return FALSE;

	// delete all existing buttons
	int nCount = (int)SendMessage(wndToolbar,TB_BUTTONCOUNT,0,0);
	while (nCount--)
		SendMessage(wndToolbar,TB_DELETEBUTTON,0,0);

	SendMessage(wndToolbar,TB_BUTTONSTRUCTSIZE,sizeof(TBBUTTON),0);

	TBBUTTON button; memset(&button,0,sizeof(button));
	button.iString = -1;

	BOOL bResult = TRUE;

	// add new buttons to the control
	int nImage = 0;
	for (int i = 0; i < pData->wItemCount; i++)
	{
		button.fsState = TBSTATE_ENABLED;
		if ((button.idCommand = pData->items[i]) == 0)
		{
			// separator
			button.fsStyle = TBSTYLE_SEP;

			// width of separator includes 8 pixel overlap
			if (GetWindowLongPtr(wndToolbar,GWL_STYLE) & TBSTYLE_FLAT)// || _afxComCtlVersion == VERSION_IE4)
				button.iBitmap = 6;
			else
				button.iBitmap = 8;
		}
		else
		{
			// a command button with image
			button.fsStyle = TBSTYLE_BUTTON;
			button.iBitmap = nImage++;
		}

		if (!SendMessage(wndToolbar,TB_ADDBUTTONS,1,(LPARAM)&button))
		{
			bResult = FALSE;
			break;
		}
	}

	if (bResult)
	{
		// set new sizes of the buttons
		SendMessage(wndToolbar,TB_SETBITMAPSIZE,0,
			MAKELONG(pData->wWidth,pData->wHeight));
		SendMessage(wndToolbar,TB_SETBUTTONSIZE,0,
			MAKELONG(pData->wWidth + 7,pData->wHeight + 7));

		// create system color map - skip btn face because it is
		// used as a mask in the image list
		COLORMAP cmap[] = {
			RGB(  0,  0,  0),		GetSysColor(COLOR_3DDKSHADOW),
			RGB(128,128,128),		GetSysColor(COLOR_3DSHADOW),
			RGB(233,233,233),		GetSysColor(COLOR_3DLIGHT),
			RGB(255,255,255),		GetSysColor(COLOR_3DHIGHLIGHT)
		};

		// load bitmap now that sizes are known by the toolbar control
		HBITMAP bmp = CreateMappedBitmap(g_hInstance,nToolbarID,0,
			cmap,sizeof(cmap) / sizeof(COLORMAP));
			//LoadBitmap(g_hInstance,MAKEINTRESOURCE(nToolbarID));

		g_imlTB = ImageList_Create(pData->wWidth,pData->wHeight,
			ILC_COLOR | ILC_MASK,4,4);

		if (!g_imlTB)
			bResult = FALSE;

		ImageList_AddMasked(g_imlTB,bmp,RGB(192,192,192));
		DeleteObject(bmp);

		SendMessage(wndToolbar,TB_SETIMAGELIST,0,(LPARAM)g_imlTB);
	}

	UnlockResource(hGlobal);
	FreeResource(hGlobal);

	return bResult;
}

void LoadPresets()
{
	g_vPresets.erase(g_vPresets.begin(),g_vPresets.end());

	char strFile[MAX_PATH]; GetModuleFileName(NULL,strFile,MAX_PATH);
	char* pSlash = strrchr(strFile,'\\');
	strcpy(pSlash ? pSlash + 1 : strFile,"presets.txt");
	
	std::ifstream fs(strFile);
	if (fs.fail())
		return;

	char strTemp[1000];

	do
	{
		strTemp[0] = 0;
		Preset ps;
		std::string temps;

		if (fs.fail() || fs.get() != ':')
			break;

		fs.getline(strTemp,1000);
		if (fs.fail())
			break;

		ps.name = strTemp;

		// eat spaces

		while (isspace(fs.peek()))
			fs.get();

		while (fs.peek() != ':')
		{
			fs >> temps;
			if (fs.fail())
				break;

			ps.colors.push_back(ColorFromHex(temps));

			while (isspace(fs.peek()))
				fs.get();
		}

		g_vPresets.push_back(ps);
	}
	while (!fs.fail());

	g_bPresetsModified = FALSE;
	if (g_vPresets.size())
		SwitchToPreset(0);
}

BOOL SavePresets(BOOL bAskIfModified)
{
	if (!g_bPresetsModified)
		return TRUE;

	if (bAskIfModified)
	{
		switch (MessageBox(g_dlgMain,"The presets have been modified. Would you like to save changes?",
			g_strTitle,MB_YESNOCANCEL | MB_ICONEXCLAMATION))
		{
			case IDNO:
				// reload presets
				LoadPresets();
				//g_bPresetsModified = FALSE;
				return TRUE;
			case IDCANCEL:
				return FALSE;				
		}

		// IDYES will bypass this and save the presets
	}

	char strFile[MAX_PATH]; GetModuleFileName(NULL,strFile,MAX_PATH);
	char* pSlash = strrchr(strFile,'\\');
	strcpy(pSlash ? pSlash + 1 : strFile,"presets.txt");
	
	std::ofstream fs(strFile);
	if (fs.fail())
		return FALSE;

	int i, j;
	for (i = 0; i < (int)g_vPresets.size(); i++)
	{
		fs << ':' << g_vPresets[i].name.c_str();

		for (j = 0; j < (int)g_vPresets[i].colors.size(); j++)
		{
			if (j % 4 == 0)	fs << "\n ";
			else fs << "  ";

			fs << ColorToHex(g_vPresets[i].colors[j],FALSE).c_str();
		}

		fs << "\n\n";
	}

	g_bPresetsModified = FALSE;

	return TRUE;
}

BOOL SwitchToPreset(int nIndex)
{
	if (nIndex < -1 || nIndex >= (int)g_vPresets.size())
		return FALSE;

	if (!SavePresets(TRUE))
		return FALSE;

	g_bPresetsModified = FALSE;
	g_nSelPreset = nIndex;

	if (g_dlgMain)
	{
		SendDlgItemMessage(g_dlgMain,IDC_PRESET_COLORS,LB_SETCURSEL,-1,0); // deselect old item
		UpdatePresets(FALSE,TRUE,TRUE);
	}

	return TRUE;
}

std::string ColorToHex(COLORREF clr, BOOL bPoundSign)
{
	char strTemp[3];
	std::string strHex = bPoundSign ? "#" : "";

	sprintf(strTemp,"%02X",GetRValue(clr)); strHex += strTemp;
	sprintf(strTemp,"%02X",GetGValue(clr)); strHex += strTemp;
	sprintf(strTemp,"%02X",GetBValue(clr)); strHex += strTemp;

	return strHex;
}

COLORREF ColorFromHex(std::string strHex)
{
	if (strHex.length() < 1)
		return RGB(0,0,0); // black if error

	int n = (strHex[0] == '#') ? 1 : 0;
	if (strHex.length() < (size_t)n + 6)
		return RGB(0,0,0); // black if error

	COLORREF final = 0;
	for (int i = 0; i < 3; i++)
	{
		char a = strHex[n + i * 2]; // first character
		char b = strHex[n + i * 2 + 1]; // second character

		if ('0' <= a && a <= '9')a -= '0';
		else if ('A' <= toupper(a) && toupper(a) <= 'F')a = toupper(a) - 'A' + 10;
		else return RGB(0,0,0); // illegal character

		if ('0' <= b && b <= '9')b -= '0';
		else if ('A' <= toupper(b) && toupper(b) <= 'F')b = toupper(b) - 'A' + 10;
		else return RGB(0,0,0); // illegal character

		((BYTE*)&final)[i] = a * 16 + b;
	}

	return final;
}

BOOL IsSelected(UINT nControlID)
{
	return GetWindowLongPtr(GetDlgItem(g_dlgMain,nControlID),GWL_STYLE) & HCS_SELECTED;
}

void SetSelected(UINT nControlID, BOOL bSelected)
{
	HWND wndControl = GetDlgItem(g_dlgMain,nControlID);
	LONG n = GetWindowLongPtr(wndControl,GWL_STYLE);
	SetWindowLongPtr(wndControl,GWL_STYLE,
		bSelected ? (n | HCS_SELECTED) : (n & ~HCS_SELECTED));
	RedrawWindow(wndControl,NULL,NULL,RDW_INVALIDATE);
}

COLORREF GetColor(UINT nControlID)
{
	switch (nControlID)
	{
		case IDC_COLOR_LEFT:	return g_vColorInfo[0];
		case IDC_COLOR_RIGHT:	return g_vColorInfo[1];
		case IDC_COLOR_EDIT:	return g_vColorInfo[2];
	}

	return 0;
}

void SetColor(UINT nControlID, COLORREF clr)
{
	switch (nControlID)
	{
		case IDC_COLOR_LEFT:	g_vColorInfo[0] = clr; break;
		case IDC_COLOR_RIGHT:	g_vColorInfo[1] = clr; break;
		case IDC_COLOR_EDIT:	g_vColorInfo[2] = clr; break;
	}
}

void SetEditColor(COLORREF clrEdit, BOOL bRefreshGradient, UINT nSourceID)
{
	if (!g_bInFunction) // prevent recursion
	{
		g_bInFunction = TRUE;

		SetColor(IDC_COLOR_EDIT,clrEdit);

		if (bRefreshGradient)
		{
			if (IsSelected(IDC_COLOR_LEFT))
				SetColor(IDC_COLOR_LEFT,clrEdit);
			else
				SetColor(IDC_COLOR_RIGHT,clrEdit);
		}

		HWND wndPresetColors = GetDlgItem(g_dlgMain,IDC_PRESET_COLORS);
		int nCurSelColor;
		if (wndPresetColors && nSourceID != IDC_PRESET_COLORS &&
			(nCurSelColor = SendMessage(wndPresetColors,LB_GETCURSEL,0,0)) != LB_ERR)
		{
			g_bPresetsModified = TRUE;

			g_vPresets[g_nSelPreset].colors[nCurSelColor] = clrEdit;

			RECT rc;
			if (SendMessage(wndPresetColors,LB_GETITEMRECT,nCurSelColor,
				(LPARAM)&rc) != LB_ERR)
				RedrawWindow(wndPresetColors,&rc,NULL,RDW_INVALIDATE);

			UpdatePresets(FALSE,FALSE,TRUE); // update preset toolbar
		}

		UpdateControls(nSourceID);

		g_bInFunction = FALSE;
	}
}

void SetEditColorPart(int nPart, BYTE nValue, UINT nSourceID)
{
	/*if (!g_bInFunction) // prevent recursion
	{
		g_bInFunction = TRUE;

		int r = (nPart == 0) ? nValue : GetRValue(GetColor(IDC_COLOR_EDIT));
		int g = (nPart == 1) ? nValue : GetGValue(GetColor(IDC_COLOR_EDIT));
		int b = (nPart == 2) ? nValue : GetBValue(GetColor(IDC_COLOR_EDIT));

		SetColor(IDC_COLOR_EDIT,RGB(r,g,b));
		if (IsSelected(IDC_COLOR_LEFT))
			SetColor(IDC_COLOR_LEFT,RGB(r,g,b));
		else
			SetColor(IDC_COLOR_RIGHT,RGB(r,g,b));

		g_bInFunction = FALSE;
	}*/

	int r = (nPart == 0) ? nValue : GetRValue(GetColor(IDC_COLOR_EDIT));
	int g = (nPart == 1) ? nValue : GetGValue(GetColor(IDC_COLOR_EDIT));
	int b = (nPart == 2) ? nValue : GetBValue(GetColor(IDC_COLOR_EDIT));

	SetEditColor(RGB(r,g,b),TRUE,nSourceID);
}

void SelectLeftOrRightColor(BOOL bRight, UINT nSourceID)
{
	SetSelected(bRight ? IDC_COLOR_RIGHT : IDC_COLOR_LEFT,TRUE);
	SetSelected(bRight ? IDC_COLOR_LEFT : IDC_COLOR_RIGHT,FALSE);

	SetEditColor(GetColor(bRight ? IDC_COLOR_RIGHT : IDC_COLOR_LEFT),FALSE,nSourceID);
}

void UpdateControls(UINT nSourceID)
{
	//if (!g_bInFunction) // prevent recursion
	//{
		//g_bInFunction = TRUE;

		RedrawWindow(GetDlgItem(g_dlgMain,IDC_COLOR_LEFT),NULL,NULL,RDW_INVALIDATE);
		RedrawWindow(GetDlgItem(g_dlgMain,IDC_COLOR_RIGHT),NULL,NULL,RDW_INVALIDATE);
		RedrawWindow(GetDlgItem(g_dlgMain,IDC_COLOR_EDIT),NULL,NULL,RDW_INVALIDATE);
		RedrawWindow(GetDlgItem(g_dlgMain,IDC_GRADIENT),NULL,NULL,RDW_INVALIDATE);

		COLORREF r = GetRValue(GetColor(IDC_COLOR_EDIT));
		COLORREF g = GetGValue(GetColor(IDC_COLOR_EDIT));
		COLORREF b = GetBValue(GetColor(IDC_COLOR_EDIT));

		if (nSourceID != IDC_EDIT_RED &&
			nSourceID != IDC_EDIT_GREEN &&
			nSourceID != IDC_EDIT_BLUE)
		{
			SetDlgItemInt(g_dlgMain,IDC_EDIT_RED,r,FALSE);
			SetDlgItemInt(g_dlgMain,IDC_EDIT_GREEN,g,FALSE);
			SetDlgItemInt(g_dlgMain,IDC_EDIT_BLUE,b,FALSE);
		}

		/*if (nSourceID != IDC_SCROLL_RED &&
			nSourceID != IDC_SCROLL_GREEN &&
			nSourceID != IDC_SCROLL_BLUE)
		{*/
			SendMessage(GetDlgItem(g_dlgMain,IDC_SCROLL_RED),SBM_SETPOS,r,TRUE);
			SendMessage(GetDlgItem(g_dlgMain,IDC_SCROLL_GREEN),SBM_SETPOS,g,TRUE);
			SendMessage(GetDlgItem(g_dlgMain,IDC_SCROLL_BLUE),SBM_SETPOS,b,TRUE);
		//}

		// update HEX
		if (nSourceID != IDC_HEX)
			SetDlgItemText(g_dlgMain,IDC_HEX,
			ColorToHex(GetColor(IDC_COLOR_EDIT),TRUE).c_str());

		//g_bInFunction = FALSE;
	//}
}

void UpdatePresets(BOOL bSelector, BOOL bColors, BOOL bToolbar)
{
	// update selector combo box
	HWND wndSel = GetDlgItem(g_dlgMain,IDC_PRESETS);
	if (bSelector)
	{
		SendMessage(wndSel,CB_RESETCONTENT,0,0);

		int nCount = g_vPresets.size();
		for (int i = 0; i < nCount; i++)
			SendMessage(wndSel,CB_ADDSTRING,0,(LPARAM)g_vPresets[i].name.c_str());

		if (nCount)
			g_nSelPreset = max(0,min(nCount - 1,g_nSelPreset));
	}

	SendMessage(wndSel,CB_SETCURSEL,g_nSelPreset,0);

	// since colors are drawn on request, only make sure the NUMBER of items
	// in the list is correct
	if (bColors)
	{
		HWND wndColors = GetDlgItem(g_dlgMain,IDC_PRESET_COLORS);

		int nCountInList = SendMessage(wndColors,LB_GETCOUNT,0,0);
		int nCount = (g_nSelPreset < 0) ? 0 : g_vPresets[g_nSelPreset].colors.size();

		if (nCountInList > nCount) // remove items
		{
			int n = nCountInList - nCount;
			while (n--)
				SendMessage(wndColors,LB_DELETESTRING,0,0); // delete an item
		}
		else // add items
		{
			int n = nCount - nCountInList;
			while (n--)
				SendMessage(wndColors,LB_ADDSTRING,0,0); // add an item
		}

		RedrawWindow(wndColors,NULL,NULL,RDW_INVALIDATE);

		// update label w/ number of colors
		char str[256];
		sprintf(str,"%d color(s)",nCount);
		SetDlgItemText(g_dlgMain,IDC_COLOR_COUNT,str);
	}

	// update toolbar (and menu)
	if (bToolbar)
	{
		HMENU hMenu = GetSubMenu(GetMenu(g_dlgMain),0);

		HWND wndTB = GetDlgItem(g_dlgMain,IDC_PRESET_TOOLBAR);
		HWND wndCol = GetDlgItem(g_dlgMain,IDC_PRESET_COLORS);

		int nCount = SendMessage(wndTB,TB_BUTTONCOUNT,0,0);

		// toolbar might not even be initialized, so create it
		if (nCount == 0 || g_imlTB == NULL) 
			LoadToolbar(wndTB,IDR_TB_PRESETS);

		BOOL b;
		b = g_bPresetsModified;
		SendMessage(wndTB,TB_SETSTATE,ID_SAVE_PRESET,MAKELONG((b ? TBSTATE_ENABLED : 0),0));
		EnableMenuItem(hMenu,ID_SAVE_PRESET,MF_BYCOMMAND | (b ? MF_ENABLED : MF_GRAYED));

		b = (g_nSelPreset >= 0 && g_nSelPreset < (int)g_vPresets.size());
		SendMessage(wndTB,TB_SETSTATE,ID_RENAME_PRESET,MAKELONG((b ? TBSTATE_ENABLED : 0),0));
		SendMessage(wndTB,TB_SETSTATE,ID_DELETE_PRESET,MAKELONG((b ? TBSTATE_ENABLED : 0),0));
		SendMessage(wndTB,TB_SETSTATE,ID_NEW_COLOR,MAKELONG((b ? TBSTATE_ENABLED : 0),0));

		EnableMenuItem(hMenu,ID_RENAME_PRESET,MF_BYCOMMAND | (b ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(hMenu,ID_DELETE_PRESET,MF_BYCOMMAND | (b ? MF_ENABLED : MF_GRAYED));
		EnableMenuItem(hMenu,ID_NEW_COLOR,MF_BYCOMMAND | (b ? MF_ENABLED : MF_GRAYED));

		if (wndCol)
		{
			int nSelColor = SendMessage(wndCol,LB_GETCURSEL,0,0);
			if (g_nSelPreset >= 0)
				b = (nSelColor >= 0 && nSelColor < (int)g_vPresets[g_nSelPreset].colors.size());
			else
				b = FALSE;
		}
		else
			b = FALSE;

		SendMessage(wndTB,TB_SETSTATE,ID_DELETE_COLOR,MAKELONG((b ? TBSTATE_ENABLED : 0),0));
		EnableMenuItem(hMenu,ID_DELETE_COLOR,MF_BYCOMMAND | (b ? MF_ENABLED : MF_GRAYED));
	}
}

void HandleScroll(HWND wnd, WPARAM params)
{
	COLORREF clr = GetColor(IDC_COLOR_EDIT);
	int n = 0;

	if (wnd == GetDlgItem(g_dlgMain,IDC_SCROLL_RED))
		n = GetRValue(clr);
	else if (wnd == GetDlgItem(g_dlgMain,IDC_SCROLL_GREEN))
		n = GetGValue(clr);
	else if (wnd == GetDlgItem(g_dlgMain,IDC_SCROLL_BLUE))
		n = GetBValue(clr);
	else
		return;

	int oldn = n;

	switch (LOWORD(params))
	{
		case SB_LEFT:			n = 0;		break;
		case SB_RIGHT:			n = 255;	break;
		case SB_LINELEFT:		n -= 1;		break;
		case SB_LINERIGHT:		n += 1;		break;
		case SB_PAGELEFT:		n -= 15;	break;
		case SB_PAGERIGHT:		n += 15;	break;
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:		n = HIWORD(params); break;
	}

	n = max(0,min(255,n));
	SetScrollPos(wnd,SB_CTL,n,TRUE);

	UINT nID = GetWindowLongPtr(wnd,GWL_ID);
	SetEditColorPart(nID - IDC_SCROLL_RED,n,nID);

	if (g_bLinkScrollbars)
	{
		if (nID != IDC_SCROLL_RED)
			SetEditColorPart(0,max(0,min(255,GetRValue(clr) + n - oldn)),nID);
		if (nID != IDC_SCROLL_GREEN)
			SetEditColorPart(1,max(0,min(255,GetGValue(clr) + n - oldn)),nID);
		if (nID != IDC_SCROLL_BLUE)
			SetEditColorPart(2,max(0,min(255,GetBValue(clr) + n - oldn)),nID);
	}
}

void DrawTransparentBitmap(HDC destdc, int x, int y, HBITMAP hbmp, COLORREF clrTransparent)
{
	BITMAP bmptemp; GetObject(hbmp,sizeof(bmptemp),&bmptemp);

	int cx = bmptemp.bmWidth,
		cy = bmptemp.bmHeight;

	RECT rc = {0,0,cx,cy};

	// create main dc
	HDC hdc = CreateCompatibleDC(destdc);
	HGDIOBJ oldbmp = SelectObject(hdc,hbmp);

	// create monochrome dc
	HDC hdc2 = CreateCompatibleDC(destdc);
	HGDIOBJ oldbmp2 = SelectObject(hdc2,
		CreateBitmap(cx,cy,1,1,NULL));

	SetBkColor(hdc,clrTransparent);
	BitBlt(hdc2,0,0,cx,cy,hdc,0,0,SRCCOPY);
	SetBkColor(hdc,RGB(255,255,255));
	BitBlt(hdc,0,0,cx,cy,hdc2,0,0,SRCPAINT);
	InvertRect(hdc2,&rc);
	BitBlt(destdc,x,y,cx,cy,hdc2,0,0,SRCPAINT);
	BitBlt(destdc,x,y,cx,cy,hdc,0,0,SRCAND);

	// delete monochrome dc
	DeleteObject(SelectObject(hdc2,oldbmp2));
	DeleteDC(hdc2);

	// delete main dc
	SelectObject(hdc,oldbmp);
	DeleteDC(hdc);
}

void DrawColorBox(HDC destdc, LPRECT prc, BOOL bGradient, BOOL bSelected,
				  COLORREF clr1, COLORREF clr2)
{
	int cx = prc->right - prc->left,
		cy = prc->bottom - prc->top;

	HDC hdc = CreateCompatibleDC(destdc);
	HGDIOBJ oldbmp;

	if (bGradient)
	{
		BITMAPINFO bi; memset(&bi,0,sizeof(bi));
		bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
		bi.bmiHeader.biWidth = cx;
		bi.bmiHeader.biHeight = -cy;
		bi.bmiHeader.biBitCount = 32;
		bi.bmiHeader.biPlanes = 1;

		BYTE* pBits = NULL;

		oldbmp = SelectObject(hdc,
			CreateDIBSection(hdc,&bi,DIB_RGB_COLORS,(LPVOID*)&pBits,NULL,0));

		// draw to dib
		int x, y;
		for (y = 0; y < cy; y++)
			for (x = 0; x < cx; x++)
			{
				pBits[2] = (GetRValue(clr2) - GetRValue(clr1)) * x / cx + GetRValue(clr1);
				pBits[1] = (GetGValue(clr2) - GetGValue(clr1)) * x / cx + GetGValue(clr1);
				pBits[0] = (GetBValue(clr2) - GetBValue(clr1)) * x / cx + GetBValue(clr1);

				pBits += 4;
			}
	}
	else
	{
		oldbmp = SelectObject(hdc,CreateCompatibleBitmap(destdc,cx,cy));

		HBRUSH brush = CreateSolidBrush(clr1);
		RECT rcTemp = {0,0,cx,cy};
		FillRect(hdc,&rcTemp,brush);
		DeleteObject(brush);
	}

	if (bSelected)
	{
		LOGBRUSH lb; lb.lbStyle = BS_NULL;
		HGDIOBJ oldbrush = SelectObject(hdc,CreateBrushIndirect(&lb));
		HGDIOBJ oldpen = SelectObject(hdc,CreatePen(PS_SOLID,1,GetSysColor(COLOR_HIGHLIGHT)));

		Rectangle(hdc,0,0,cx,cy);
		Rectangle(hdc,1,1,cx - 1,cy - 1);

		DeleteObject(SelectObject(hdc,oldpen));
		oldpen = SelectObject(hdc,CreatePen(PS_SOLID,1,GetSysColor(COLOR_HIGHLIGHTTEXT)));

		Rectangle(hdc,2,2,cx - 2,cy - 2);

		DeleteObject(SelectObject(hdc,oldbrush));
		DeleteObject(SelectObject(hdc,oldpen));
	}
	else
	{
		RECT rcTemp = {0,0,cx,cy};
		DrawEdge(hdc,&rcTemp,BDR_SUNKENOUTER,BF_RECT);
	}

	BitBlt(destdc,prc->left,prc->top,cx,cy,hdc,0,0,SRCCOPY);

	DeleteObject(SelectObject(hdc,oldbmp));
	DeleteDC(hdc);
}

INT_PTR CALLBACK MainDlgProc(HWND hDlg, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uiMessage) 
	{
		case WM_PAINT:
		{
			RECT rc; RECT rc2;
			GetClientRect(hDlg,&rc);
			GetWindowRect(GetDlgItem(hDlg,IDC_PRESET_TOOLBAR),&rc2);
			ScreenToClient(hDlg,(POINT*)&rc2.left);
			ScreenToClient(hDlg,(POINT*)&rc2.right);

			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hDlg,&ps);

			DrawEdge(hdc,&rc,EDGE_ETCHED,BF_TOP);

			rc.top = rc2.bottom + 1;

			DrawEdge(hdc,&rc,EDGE_ETCHED,BF_TOP);

			EndPaint(hDlg,&ps);
			break;
		}

		case WM_INITDIALOG:
		{
			g_dlgMain = hDlg;

			LoadRegistryData(); // load registry info

			// subclass preset colors
			{
				HWND wndColors = GetDlgItem(hDlg,IDC_PRESET_COLORS);
				SetProp(wndColors,"OLDPROC",
					(HANDLE)SetWindowLongPtr(wndColors,GWL_WNDPROC,(LONG)ColorListProc));
			}

			SendMessage(hDlg,WM_SETICON,ICON_BIG,
				(LPARAM)LoadIcon(g_hInstance,MAKEINTRESOURCE(IDR_MAIN)));
			SendMessage(hDlg,WM_SETICON,ICON_SMALL,
				(LPARAM)LoadImage(g_hInstance,MAKEINTRESOURCE(IDR_MAIN),IMAGE_ICON,
				GetSystemMetrics(SM_CXSMICON),GetSystemMetrics(SM_CYSMICON),LR_SHARED));
			SelectLeftOrRightColor(FALSE,0); // select left color at first
			SetEditColor(g_vColorInfo[2],TRUE,0);

			// update presets
			UpdatePresets();

			// init scrollbars
			SCROLLINFO si;
			si.cbSize = sizeof(si);
			si.fMask = SIF_ALL;
			si.nMin = 0;
			si.nMax = 255;
			si.nPos = 0;
			si.nPage = 0;
			SendMessage(GetDlgItem(hDlg,IDC_SCROLL_RED),SBM_SETSCROLLINFO,FALSE,(LPARAM)&si);
			SendMessage(GetDlgItem(hDlg,IDC_SCROLL_GREEN),SBM_SETSCROLLINFO,FALSE,(LPARAM)&si);
			SendMessage(GetDlgItem(hDlg,IDC_SCROLL_BLUE),SBM_SETSCROLLINFO,FALSE,(LPARAM)&si);

			// init preset color list
			RECT rc; GetClientRect(GetDlgItem(hDlg,IDC_PRESET_COLORS),&rc);
			SendMessage(GetDlgItem(hDlg,IDC_PRESET_COLORS),LB_SETCOLUMNWIDTH,
				rc.bottom - rc.top - 6,0);

			// init hex
			SendMessage(hDlg,EM_LIMITTEXT,7,0);

			SetWindowText(hDlg,g_strTitle);
			
			break;
		}

		case WM_COMMAND:
		{
			// pass manipulate messages to edit color
			if (LOWORD(wParam) >= COLORPOPUPID_FIRST && LOWORD(wParam) <= COLORPOPUPID_LAST)
				//return SendDlgItemMessage(hDlg,IDC_COLOR_EDIT,WM_COMMAND,wParam,lParam);
			{
				NMCHANGECOLOR nmh;
				nmh.hdr.code = HCN_USERCHANGE;
				nmh.hdr.hwndFrom = GetDlgItem(hDlg,IDC_COLOR_EDIT);
				nmh.hdr.idFrom = IDC_COLOR_EDIT;

				COLORREF clr = GetColor(IDC_COLOR_EDIT);
				int r = GetRValue(clr), g = GetGValue(clr), b = GetBValue(clr);

				switch (LOWORD(wParam))
				{
					case ID_MANIPULATE_INVERT:			nmh.clrnew = RGB(255-r,255-g,255-b); break;
					case ID_MANIPULATE_BRIGHTER:
						nmh.clrnew = RGB(
							FitByte(r * (int)0.9 + 29),
							FitByte(g * (int)0.9 + 29),
							FitByte(b * (int)0.9 + 29)); break;
					case ID_MANIPULATE_DARKER:
						nmh.clrnew = RGB(
							FitByte(r * (int)0.86),
							FitByte(g * (int)0.86),
							FitByte(b * (int)0.86)); break;
					case ID_MANIPULATE_MORECONTRAST:
						nmh.clrnew = RGB(
							FitByte(r * (int)(1.1 - 12.8)),
							FitByte(g * (int)(1.1 - 12.8)),
							FitByte(b * (int)(1.1 - 12.8))); break;
					case ID_MANIPULATE_LESSCONTRAST:
						nmh.clrnew = RGB(
							FitByte(r * (int)(0.9 + 12.8)),
							FitByte(g * (int)(0.9 + 12.8)),
							FitByte(b * (int)(0.9 + 12.8))); break;
					case ID_QUICKCOLORS_COMMON_RED:			nmh.clrnew = RGB(255,  0,  0); break;
					case ID_QUICKCOLORS_COMMON_YELLOW:		nmh.clrnew = RGB(255,255,  0); break;
					case ID_QUICKCOLORS_COMMON_GREEN:		nmh.clrnew = RGB(  0,255,  0); break;
					case ID_QUICKCOLORS_COMMON_BLUE:		nmh.clrnew = RGB(  0,  0,255); break;
					case ID_QUICKCOLORS_COMMON_PURPLE:		nmh.clrnew = RGB(128,  0,128); break;
					case ID_QUICKCOLORS_COMMON_ORANGE:		nmh.clrnew = RGB(255,128,  0); break;
					case ID_QUICKCOLORS_COMMON_BLACK:		nmh.clrnew = RGB(  0,  0,  0); break;
					case ID_QUICKCOLORS_COMMON_WHITE:		nmh.clrnew = RGB(255,255,255); break;
					case ID_QUICKCOLORS_COMMON_GRAY:		nmh.clrnew = RGB(128,128,128); break;
					case ID_QUICKCOLORS_COMMON_LIGHTGRAY:	nmh.clrnew = RGB(192,192,192); break;
					case ID_QUICKCOLORS_COMMON_TEAL:		nmh.clrnew = RGB(  0,128,128); break;
					case ID_QUICKCOLORS_COMMON_MAGENTA:		nmh.clrnew = RGB(255,  0,255); break;
					case ID_QUICKCOLORS_COMMON_YELLOWGREEN:	nmh.clrnew = RGB(192,255,  0); break;
					case ID_QUICKCOLORS_COMMON_DARKBLUE:	nmh.clrnew = RGB(  0,  0,128); break;
				}

				SendMessage(hDlg,WM_NOTIFY,IDC_COLOR_EDIT,(LPARAM)&nmh);
				break;
			}

			switch (LOWORD(wParam))
			{
				case IDOK:
					// OK code goes here
					// fall-through
				case IDCANCEL:
					//SendMessage(hDlg,WM_CLOSE,0,0);
					break;

				case ID_HELP_ABOUT:
					DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_ABOUTBOX),g_dlgMain,
						AboutProc,NULL);
					break;
				
				case ID_HELP_CONTENTS:
				{
					char strFile[MAX_PATH]; GetModuleFileName(NULL,strFile,MAX_PATH);
					char* pSlash = strrchr(strFile,'\\');
					strcpy(pSlash ? pSlash + 1 : strFile,"hexcf.chm::/welcome.html");
					HtmlHelp(NULL,strFile,HH_DISPLAY_TOC,0);
					break;
				}

				case IDC_EDIT_RED:
				case IDC_EDIT_GREEN:
				case IDC_EDIT_BLUE:
					if (HIWORD(wParam) == EN_CHANGE)
					{
						BOOL bSuccess;
						UINT n = GetDlgItemInt(hDlg,LOWORD(wParam),&bSuccess,FALSE);
						n = max(0,min(255,n));
						if (bSuccess)
							SetEditColorPart(LOWORD(wParam) - IDC_EDIT_RED,n,LOWORD(wParam));
					}
					break;

				case IDC_HEX:
					if (HIWORD(wParam) == EN_CHANGE)
					{
						char str[10];
						GetDlgItemText(hDlg,IDC_HEX,str,10);

						SetEditColor(ColorFromHex(str),TRUE,IDC_HEX);
					}
					break;

				case IDC_CHK_TOPMOST:
				{
					HWND wnd = GetDlgItem(hDlg,IDC_CHK_TOPMOST);
					if (GetProp(wnd,"SET"))
					{
						SetWindowPos(hDlg,HWND_NOTOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
						RemoveProp(wnd,"SET");
					}
					else
					{
						SetWindowPos(hDlg,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE | SWP_NOSIZE);
						SetProp(wnd,"SET",(HANDLE)1);
					}
					RedrawWindow(wnd,NULL,NULL,RDW_INVALIDATE);
					break;
				}

				case ID_OPTIONS_LINK_SCROLLBARS:
					g_bLinkScrollbars = !g_bLinkScrollbars;
					CheckMenuItem(GetSubMenu(GetMenu(hDlg),3),LOWORD(wParam),
						MF_BYCOMMAND | (g_bLinkScrollbars ? MF_CHECKED : MF_UNCHECKED));
					break;
					
				case ID_OPTIONS_NEW_COLOR_SCREEN_PICK:
					g_bNewColorScreenPick = !g_bNewColorScreenPick;
					CheckMenuItem(GetSubMenu(GetMenu(hDlg),3),LOWORD(wParam),
						MF_BYCOMMAND | (g_bNewColorScreenPick ? MF_CHECKED : MF_UNCHECKED));
					break;

				case IDC_PRESET_COLORS:
					if (HIWORD(wParam) == LBN_DBLCLK)
						SendMessage((HWND)lParam,LB_SETCURSEL,-1,0); // clear selection
					else if (HIWORD(wParam) == LBN_SELCHANGE && g_nSelPreset >= 0)
					{
						int nCurSel = SendMessage((HWND)lParam,LB_GETCURSEL,0,0);
						SetEditColor(g_vPresets[g_nSelPreset].colors[nCurSel],TRUE,
							IDC_PRESET_COLORS);
					}
					else
						break; // bypass update toolbar

					UpdatePresets(FALSE,FALSE,TRUE); // update toolbar
					break;

				case IDC_PRESETS:
					if (HIWORD(wParam) == CBN_SELCHANGE &&
						!SwitchToPreset(SendMessage((HWND)lParam,CB_GETCURSEL,0,0)))
					{
						SendMessage((HWND)lParam,CB_SETCURSEL,g_nSelPreset,0);
						UpdatePresets(FALSE,FALSE,TRUE); // update toolbar
					}
					break;

				case ID_RELOAD_PRESET:
					if (MessageBox(hDlg,"Are you sure you want to revert the presets "
						"to the preset data file contents?",g_strTitle,
						MB_OKCANCEL | MB_ICONEXCLAMATION) == IDOK)
					{
						int nPreset = g_nSelPreset;
						LoadPresets();
						SwitchToPreset(nPreset);
						UpdatePresets(TRUE,TRUE,TRUE);
					}
					break;

				case ID_SAVE_PRESET:
					SavePresets(FALSE);
					UpdatePresets(TRUE,TRUE,TRUE);
					break;

				case ID_NEW_PRESET:
				{
					if (!SavePresets(TRUE))
						break;

					char str[256]; str[0] = 0;
					if (DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_PRESET_NAME),
						g_dlgMain,PresetDlgProc,(LPARAM)str) == IDOK)
					{
						Preset ps; ps.name = str;
						g_vPresets.push_back(ps);
						g_bPresetsModified = TRUE; // for save to work, set flag to modified
						SavePresets(FALSE);
						UpdatePresets(TRUE,FALSE,TRUE); // update combo and toolbar
						SwitchToPreset(g_vPresets.size() - 1); // switch to new preset
					}
					break;
				}

				case ID_RENAME_PRESET:
				{
					if (g_nSelPreset < 0)
						break;

					if (!SavePresets(TRUE))
						break;

					char str[256]; strcpy(str,g_vPresets[g_nSelPreset].name.c_str());
					if (DialogBoxParam(g_hInstance,MAKEINTRESOURCE(IDD_PRESET_NAME),
						g_dlgMain,PresetDlgProc,(LPARAM)str) == IDOK)
					{
						g_vPresets[g_nSelPreset].name = str;
						g_bPresetsModified = TRUE; // for save to work, set flag to modified
						SavePresets(FALSE);
						UpdatePresets(TRUE,FALSE,TRUE); // update combo and toolbar
					}
					break;
				}

				case ID_DELETE_PRESET:
					if (g_nSelPreset < 0)
						break;

					if (MessageBox(hDlg,"Are you sure you want to delete the selected preset?\n"
						"NOTE: This process cannot be undone because the disk file is directly updated.",
						g_strTitle,MB_OKCANCEL | MB_ICONEXCLAMATION) == IDOK)
					{
						g_vPresets.erase(g_vPresets.begin() + g_nSelPreset);
						if (g_vPresets.size())
							SwitchToPreset(max(0,min((int)g_vPresets.size(),g_nSelPreset - 1)));
						else
							SwitchToPreset(-1);

						g_bPresetsModified = TRUE; // for save to work, set flag to modified
						SavePresets(FALSE);
						UpdatePresets(TRUE,TRUE,TRUE); // update all preset controls
					}
					break;

				case ID_NEW_COLOR:
					if (g_nSelPreset < 0)
						break;

					g_vPresets[g_nSelPreset].colors.push_back(
						GetColor(IDC_COLOR_EDIT));
					
					// update list and toolbar
					UpdatePresets(FALSE,TRUE,TRUE);

					g_bPresetsModified = TRUE; // modified the preset
					
					// set the selected color to the new color
					SendDlgItemMessage(hDlg,IDC_PRESET_COLORS,LB_SETCURSEL,
						g_vPresets[g_nSelPreset].colors.size() - 1,0);

					// send the parent window the WM_COMMAND notification since
					// LB_SETCURSEL doesnt do it
					SendMessage(hDlg,WM_COMMAND,MAKEWPARAM(IDC_PRESET_COLORS,LBN_SELCHANGE),
						(LPARAM)GetDlgItem(hDlg,IDC_PRESET_COLORS));
					break;

				case ID_DELETE_COLOR:
				{
					int n = SendDlgItemMessage(hDlg,IDC_PRESET_COLORS,LB_GETCURSEL,0,0);

					if (g_nSelPreset < 0 || n < 0 || n >= (int)g_vPresets[g_nSelPreset].colors.size())
						break;

					g_vPresets[g_nSelPreset].colors.erase(
						g_vPresets[g_nSelPreset].colors.begin() + n);

					// set the selected color to nothing
					SendDlgItemMessage(hDlg,IDC_PRESET_COLORS,LB_SETCURSEL,-1,0);

					g_bPresetsModified = TRUE; // modified the preset

					UpdatePresets(FALSE,TRUE,TRUE);

					g_bPresetsModified = TRUE; // modified the preset
					break;
				}

				case ID_COLOR_COPY_HEX:
					if (OpenClipboard(hDlg))
					{
						if (EmptyClipboard())
						{
							// create text
							COLORREF clr = GetColor(IDC_COLOR_EDIT);
							std::string strHex = ColorToHex(clr,TRUE);

							int nLength = strHex.length();

							HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE,nLength + 1);
							void* pGlobal = (void*)GlobalLock(hGlobal);
							if (hGlobal && pGlobal)
							{
								strcpy((char*)pGlobal,strHex.c_str());
								GlobalUnlock(hGlobal);
								SetClipboardData(CF_TEXT,hGlobal);
							}
						}

						CloseClipboard();
					}
					break;

				case IDC_COLOR_LEFT:
				case IDC_COLOR_RIGHT:
					SelectLeftOrRightColor(LOWORD(wParam) == IDC_COLOR_RIGHT,wParam);
					break;
			}
			return FALSE;
		}

		case WM_HSCROLL:
			if (lParam)
			{
				HandleScroll((HWND)lParam,wParam);
			}
			break;

		case WM_CTLCOLORLISTBOX:
			if (lParam == (LPARAM)GetDlgItem(hDlg,IDC_PRESET_COLORS))
			{
				SetBkColor((HDC)wParam,GetSysColor(COLOR_BTNFACE));
				SetTextColor((HDC)wParam,GetSysColor(COLOR_BTNTEXT));
				return (LRESULT)GetSysColorBrush(COLOR_BTNFACE);
			}
			break;

		case WM_MEASUREITEM:
			if (wParam == IDC_PRESET_COLORS)
			{
				LPMEASUREITEMSTRUCT pMIS = (LPMEASUREITEMSTRUCT)lParam;
				RECT rc; GetClientRect(GetDlgItem(hDlg,IDC_PRESET_COLORS),&rc); 
				pMIS->itemHeight = rc.bottom - rc.top;
			}
			break;

		case WM_DRAWITEM:
		{
			LPDRAWITEMSTRUCT pDIS = (LPDRAWITEMSTRUCT)lParam;

			switch (wParam)
			{
				case IDC_CHK_TOPMOST:
				{
					int cx = pDIS->rcItem.right - pDIS->rcItem.left,
						cy = pDIS->rcItem.bottom - pDIS->rcItem.top;

					HDC hdc = CreateCompatibleDC(pDIS->hDC);
					HGDIOBJ oldbmp = SelectObject(hdc,
						CreateCompatibleBitmap(pDIS->hDC,cx,cy));

					BOOL bOn = (BOOL)GetProp(GetDlgItem(hDlg,IDC_CHK_TOPMOST),"SET");
					BOOL bPushed = pDIS->itemState & ODS_SELECTED;

					DrawFrameControl(hdc,&pDIS->rcItem,
						DFC_BUTTON,DFCS_BUTTONPUSH |
						(bPushed ? DFCS_PUSHED : 0) |
						(bOn ? DFCS_CHECKED : 0));

					int nOffsetBitmap = bPushed ? 1 : 0;
					UINT nBmpID = bOn ? IDB_TOPMOST_ON : IDB_TOPMOST_OFF;

					HBITMAP hbmp = LoadBitmap(g_hInstance,MAKEINTRESOURCE(nBmpID));
					BITMAP bmp; GetObject(hbmp,sizeof(bmp),&bmp);

					DrawTransparentBitmap(hdc,
						(cx - bmp.bmWidth) / 2 + nOffsetBitmap,
						(cy - abs(bmp.bmHeight)) / 2 + nOffsetBitmap,hbmp,
						RGB(255,0,255));

					BitBlt(pDIS->hDC,
						pDIS->rcItem.left,pDIS->rcItem.top,cx,cy,hdc,0,0,SRCCOPY);

					DeleteObject(SelectObject(hdc,oldbmp));
					DeleteDC(hdc);
					break;
				}

				case IDC_PRESET_COLORS:
					if (g_nSelPreset >= 0)
					{
						if (pDIS->itemID >= 0 &&
							pDIS->itemID < g_vPresets[g_nSelPreset].colors.size())
						{
							RECT rc = pDIS->rcItem, rc2;
							HDC hdc = CreateCompatibleDC(pDIS->hDC);
							HGDIOBJ oldbmp = SelectObject(hdc,CreateCompatibleBitmap(
								pDIS->hDC,rc.right - rc.left,rc.bottom - rc.top));

							SetRect(&rc2,0,0,rc.right - rc.left,rc.bottom - rc.top);
							FillRect(hdc,&rc2,GetSysColorBrush(COLOR_BTNFACE));

							InflateRect(&rc2,-1,-1);

							DrawColorBox(hdc,&rc2,
								FALSE,pDIS->itemState & ODS_SELECTED,
								g_vPresets[g_nSelPreset].colors[pDIS->itemID]);

							// if dragging, draw a part of the drag cursor
							if (g_dragData.enState == DragData::DragDragging)
							{
								RECT rc3 = rc;
								OffsetRect(&rc3,-rc.left,-rc.top);

								HGDIOBJ oldpen = SelectObject(hdc,CreatePen(PS_SOLID,
									1,GetSysColor(COLOR_BTNHILIGHT)));

								if (pDIS->itemID == g_dragData.nDragCursor - 1)
								{
									// draw first half of cursor
									SetRect(&rc2,rc3.right - 4,rc3.bottom / 2 - 1,
										rc3.right - 3,rc3.bottom / 2 + 1);
									FillRect(hdc,&rc2,GetSysColorBrush(COLOR_3DDKSHADOW));
									SetRect(&rc2,rc3.right - 3,rc3.bottom / 2 - 2,
										rc3.right - 2,rc3.bottom / 2 + 2);
									FillRect(hdc,&rc2,GetSysColorBrush(COLOR_3DDKSHADOW));
									SetRect(&rc2,rc3.right - 2,rc3.bottom / 2 - 3,
										rc3.right - 1,rc3.bottom / 2 + 3);
									FillRect(hdc,&rc2,GetSysColorBrush(COLOR_3DDKSHADOW));
									SetRect(&rc2,rc3.right - 1,rc3.bottom / 2 - 4,
										rc3.right    ,rc3.bottom / 2 + 4);
									FillRect(hdc,&rc2,GetSysColorBrush(COLOR_3DDKSHADOW));

									MoveToEx(hdc,rc3.right - 5,rc3.bottom / 2 - 1,NULL);
									LineTo(hdc,rc3.right + 1,rc3.bottom / 2 - 7);

									MoveToEx(hdc,rc3.right - 5,rc3.bottom / 2,NULL);
									LineTo(hdc,rc3.right + 1,rc3.bottom / 2 + 6);
								}
								else if (pDIS->itemID == g_dragData.nDragCursor)
								{
									// draw first half of cursor
									SetRect(&rc2,rc3.left    ,rc3.bottom / 2 - 3,
										rc3.left + 1,rc3.bottom / 2 + 3);
									FillRect(hdc,&rc2,GetSysColorBrush(COLOR_3DDKSHADOW));
									SetRect(&rc2,rc3.left + 1,rc3.bottom / 2 - 2,
										rc3.left + 2,rc3.bottom / 2 + 2);
									FillRect(hdc,&rc2,GetSysColorBrush(COLOR_3DDKSHADOW));
									SetRect(&rc2,rc3.left + 2,rc3.bottom / 2 - 1,
										rc3.left + 3,rc3.bottom / 2 + 1);
									FillRect(hdc,&rc2,GetSysColorBrush(COLOR_3DDKSHADOW));

									MoveToEx(hdc,rc3.left,rc3.bottom / 2 - 4,NULL);
									LineTo(hdc,rc3.left + 4,rc3.bottom / 2);

									MoveToEx(hdc,rc3.left,rc3.bottom / 2 + 3,NULL);
									LineTo(hdc,rc3.left + 4,rc3.bottom / 2 - 1);
								}

								DeleteObject(SelectObject(hdc,oldpen));
							}

							BitBlt(pDIS->hDC,rc.left,rc.top,
								rc.right - rc.left,
								rc.bottom - rc.top,hdc,0,0,SRCCOPY);

							DeleteObject(SelectObject(hdc,oldbmp));
							DeleteDC(hdc);
						}
					}
					break;

				default:
					return 0;
			}

			return TRUE;
		}

		case WM_NOTIFY:
			if (((NMHDR*)lParam)->code == TTN_GETDISPINFO)
			{
				NMTTDISPINFO* pTTDI = (NMTTDISPINFO*)lParam;
				pTTDI->szText[0] = 0;
				pTTDI->hinst = NULL;
				LoadString(g_hInstance,pTTDI->hdr.idFrom,
					pTTDI->szText,80);

				break;
			}

			switch (wParam)
			{
				case IDC_COLOR_LEFT:
				case IDC_COLOR_RIGHT:
					if (((NMHDR*)lParam)->code == NM_CLICK)
					{
						SelectLeftOrRightColor(wParam == IDC_COLOR_RIGHT,wParam);
					}
					else if (((NMHDR*)lParam)->code == HCN_USERCHANGE)
					{
						if (IsSelected(wParam))
							SetEditColor(((NMCHANGECOLOR*)lParam)->clrnew,TRUE,wParam);
						else
							SetColor(wParam,((NMCHANGECOLOR*)lParam)->clrnew);

						UpdateControls(wParam);
					}
					break;

				case IDC_COLOR_EDIT:
					if (((NMHDR*)lParam)->code == HCN_USERCHANGE)
					{
						// update everything with the color changes
						SetEditColor(((NMCHANGECOLOR*)lParam)->clrnew,TRUE,wParam);
					}
					break;

				case IDC_SCREEN_PICKER:
					if (((NMHDR*)lParam)->code == NM_CLICK)
					{
						// set color to pixel under mouse
						POINT pt; GetCursorPos(&pt);
						HDC screendc = GetDC(NULL);
						SetEditColor(GetPixel(screendc,pt.x,pt.y),TRUE,IDC_SCREEN_PICKER);
						ReleaseDC(NULL,screendc);
					}
					break;
			}
			break;

		case WM_CLOSE:
			if (SavePresets(TRUE))
				DestroyWindow(hDlg);
			break;

		case WM_DESTROY:
		{
			// put back the original preset color list procedure
			HWND wndColors = GetDlgItem(hDlg,IDC_PRESET_COLORS);
			SetWindowLongPtr(wndColors,GWL_WNDPROC,(LONG_PTR)GetProp(wndColors,"OLDPROC"));

			SaveRegistryData();
			PostQuitMessage(0);
			break;
		}

		default:
			return FALSE;
	}
	
	return TRUE;
}

INT_PTR CALLBACK PresetDlgProc(HWND hDlg, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	LPTSTR str = (LPTSTR)GetWindowLongPtr(hDlg,GWL_USERDATA);

	switch (uiMessage)
	{
		case WM_INITDIALOG:
			SetWindowLongPtr(hDlg,GWL_USERDATA,lParam); // should be a pointer to a string
			str = (LPTSTR)lParam;
			if (!str)
			{
				MessageBox(g_dlgMain,"Internal error.",NULL,MB_ICONERROR);
				DestroyWindow(hDlg);
				return 0;
			}

			SendDlgItemMessage(hDlg,IDC_EDIT_NAME,EM_LIMITTEXT,50,0); // limit to 50 chars

			if (str[0] == 0)
			{
				SetWindowText(hDlg,"New Preset");
			}
			else
			{
				SetWindowText(hDlg,"Edit Preset Name");
				SetDlgItemText(hDlg,IDC_EDIT_NAME,str);
			}
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				{
					char strTemp[51]; strTemp[0] = 0;
					if (GetDlgItemText(hDlg,IDC_EDIT_NAME,strTemp,51) &&
						strTemp[0] != 0)
					{
						strcpy(str,strTemp);
						EndDialog(hDlg,LOWORD(wParam));
					}
					else
					{
						MessageBox(hDlg,"Please enter a valid preset name.",
							g_strTitle,MB_ICONEXCLAMATION);
						SetFocus(GetDlgItem(hDlg,IDC_EDIT_NAME));
					}
					break;
				}

				case IDCANCEL:
					EndDialog(hDlg,LOWORD(wParam));
					break;
			}
			break;

		default:
			return FALSE;
	}
	
	return TRUE;
}

LRESULT CALLBACK ColorProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	UINT nID = GetWindowLongPtr(hWnd,GWL_ID);
	BOOL bSelected = IsSelected(nID);

	switch (uiMessage)
	{
		case WM_PAINT:
		{
			RECT rc;
			GetClientRect(hWnd,&rc);

			PAINTSTRUCT ps;
			HDC paintdc = BeginPaint(hWnd,&ps);

			DrawColorBox(paintdc,&rc,FALSE,bSelected,GetColor(nID));

			EndPaint(hWnd,&ps);
			break;
		}

		/*case WM_CONTEXTMENU:
		{
			if (!(GetWindowLongPtr(hWnd,GWL_STYLE) & HCS_RCLICK))
				break;

			HMENU hMenu = LoadMenu(g_hInstance,MAKEINTRESOURCE(IDR_POPUPS));
			POINT pt; GetCursorPos(&pt);
			TrackPopupMenu(GetSubMenu(hMenu,0),0,pt.x,pt.y,0,hWnd,NULL);
			DestroyMenu(hMenu);
			break;
		}

		case WM_COMMAND:
		{
			if (LOWORD(wParam) == ID_COLOR_COPY_HEX)
			{
				if (OpenClipboard(hWnd))
				{
					if (EmptyClipboard())
					{
						// create text
						COLORREF clr = GetColor(nID);
						std::string strHex = ColorToHex(clr,TRUE);

						int nLength = strHex.length();

						HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE,nLength + 1);
						void* pGlobal = (void*)GlobalLock(hGlobal);
						if (hGlobal && pGlobal)
						{
							strcpy((char*)pGlobal,strHex.c_str());
							GlobalUnlock(hGlobal);
							SetClipboardData(CF_TEXT,hGlobal);
						}
					}

					CloseClipboard();
				}
			}
			else
			{
				NMCHANGECOLOR nmh;
				nmh.hdr.code = HCN_USERCHANGE;
				nmh.hdr.hwndFrom = hWnd;
				nmh.hdr.idFrom = nID;

				COLORREF clr = GetColor(nID);
				int r = GetRValue(clr), g = GetGValue(clr), b = GetBValue(clr);

				switch (LOWORD(wParam))
				{
					case ID_MANIPULATE_INVERT:			nmh.clrnew = RGB(255-r,255-g,255-b); break;
					case ID_MANIPULATE_BRIGHTER:
						nmh.clrnew = RGB(
							FitByte(r * 0.9 + 29),
							FitByte(g * 0.9 + 29),
							FitByte(b * 0.9 + 29)); break;
					case ID_MANIPULATE_DARKER:
						nmh.clrnew = RGB(
							FitByte(r * 0.86),
							FitByte(g * 0.86),
							FitByte(b * 0.86)); break;
					case ID_MANIPULATE_MORECONTRAST:
						nmh.clrnew = RGB(
							FitByte(r * 1.1 - 12.8),
							FitByte(g * 1.1 - 12.8),
							FitByte(b * 1.1 - 12.8)); break;
					case ID_MANIPULATE_LESSCONTRAST:
						nmh.clrnew = RGB(
							FitByte(r * 0.9 + 12.8),
							FitByte(g * 0.9 + 12.8),
							FitByte(b * 0.9 + 12.8)); break;
					case ID_QUICKCOLORS_COMMON_RED:			nmh.clrnew = RGB(255,  0,  0); break;
					case ID_QUICKCOLORS_COMMON_YELLOW:		nmh.clrnew = RGB(255,255,  0); break;
					case ID_QUICKCOLORS_COMMON_GREEN:		nmh.clrnew = RGB(  0,255,  0); break;
					case ID_QUICKCOLORS_COMMON_BLUE:		nmh.clrnew = RGB(  0,  0,255); break;
					case ID_QUICKCOLORS_COMMON_PURPLE:		nmh.clrnew = RGB(128,  0,128); break;
					case ID_QUICKCOLORS_COMMON_ORANGE:		nmh.clrnew = RGB(255,128,  0); break;
					case ID_QUICKCOLORS_COMMON_BLACK:		nmh.clrnew = RGB(  0,  0,  0); break;
					case ID_QUICKCOLORS_COMMON_WHITE:		nmh.clrnew = RGB(255,255,255); break;
					case ID_QUICKCOLORS_COMMON_GRAY:		nmh.clrnew = RGB(128,128,128); break;
					case ID_QUICKCOLORS_COMMON_LIGHTGRAY:	nmh.clrnew = RGB(192,192,192); break;
					case ID_QUICKCOLORS_COMMON_TEAL:		nmh.clrnew = RGB(  0,128,128); break;
					case ID_QUICKCOLORS_COMMON_MAGENTA:		nmh.clrnew = RGB(255,  0,255); break;
					case ID_QUICKCOLORS_COMMON_YELLOWGREEN:	nmh.clrnew = RGB(192,255,  0); break;
					case ID_QUICKCOLORS_COMMON_DARKBLUE:	nmh.clrnew = RGB(  0,  0,128); break;
				}

				SendMessage(GetParent(hWnd),WM_NOTIFY,nID,(LPARAM)&nmh);
			}
			break;
		}*/

		case WM_LBUTTONDOWN:
		{
			NMHDR nmh; nmh.code = NM_CLICK; nmh.hwndFrom = hWnd; nmh.idFrom = nID;
			SendMessage(GetParent(hWnd),WM_NOTIFY,nID,(LPARAM)&nmh);
			break;
		}

		default:
			return DefWindowProc(hWnd,uiMessage,wParam,lParam);
	}

	return 0;
}

LRESULT CALLBACK GradientProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	UINT nID = GetWindowLongPtr(hWnd,GWL_ID);

	switch (uiMessage)
	{
		case WM_PAINT:
		{
			RECT rc;
			GetClientRect(hWnd,&rc);

			PAINTSTRUCT ps;
			HDC paintdc = BeginPaint(hWnd,&ps);

			DrawColorBox(paintdc,&rc,TRUE,FALSE,
				GetColor(IDC_COLOR_LEFT),GetColor(IDC_COLOR_RIGHT));

			EndPaint(hWnd,&ps);
			break;
		}

		case WM_LBUTTONDBLCLK:
			SetEditColor(GetColor(IDC_COLOR_EDIT),TRUE,nID);
			break;

		case WM_LBUTTONUP:
			ReleaseCapture();
			break;
		case WM_LBUTTONDOWN:
			SetCapture(hWnd);
		case WM_MOUSEMOVE:
			if (wParam & MK_LBUTTON && GetCapture() == hWnd)
			{
				RECT rc; GetClientRect(hWnd,&rc);

				int x = GET_X_LPARAM(lParam);
				x = max(0,min(rc.right,x));

				COLORREF clr1 = GetColor(IDC_COLOR_LEFT),
						 clr2 = GetColor(IDC_COLOR_RIGHT);

				COLORREF clr = RGB(
					(GetRValue(clr2) - GetRValue(clr1)) * x / rc.right + GetRValue(clr1),
					(GetGValue(clr2) - GetGValue(clr1)) * x / rc.right + GetGValue(clr1),
					(GetBValue(clr2) - GetBValue(clr1)) * x / rc.right + GetBValue(clr1)
					);

				SetEditColor(clr,FALSE,nID);
			}
			break;

		default:
			return DefWindowProc(hWnd,uiMessage,wParam,lParam);
	}

	return 0;
}

LRESULT CALLBACK ScreenPickerProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	UINT nID = GetWindowLongPtr(hWnd,GWL_ID);
	BOOL bSelected = IsSelected(nID);

	switch (uiMessage)
	{
		case WM_PAINT:
		{
			RECT rc;
			GetClientRect(hWnd,&rc);

			PAINTSTRUCT ps;
			HDC paintdc = BeginPaint(hWnd,&ps);

			HDC hdc = CreateCompatibleDC(paintdc);
			HGDIOBJ oldbmp = SelectObject(hdc,
				CreateCompatibleBitmap(paintdc,rc.right,rc.bottom));

			FillRect(hdc,&rc,GetSysColorBrush(COLOR_BTNFACE));

			if (GetCapture() == hWnd)
			{
				POINT pt; GetCursorPos(&pt);
				HDC screendc = GetDC(NULL);
				SetEditColor(GetPixel(screendc,pt.x,pt.y),TRUE,nID);
				ReleaseDC(NULL,screendc);
			}
			else
			{
				// draw bitmap icon
				HBITMAP hbmp = LoadBitmap(g_hInstance,MAKEINTRESOURCE(IDB_SCREEN_BUTTON));
				BITMAP bmp; GetObject(hbmp,sizeof(bmp),&bmp);
				DrawTransparentBitmap(hdc,
					(rc.right - bmp.bmWidth) / 2,
					(rc.bottom - abs(bmp.bmHeight)) / 2,
					hbmp,RGB(255,0,255));
			}

			DrawEdge(hdc,&rc,BDR_RAISEDINNER,BF_RECT);

			BitBlt(paintdc,0,0,rc.right,rc.bottom,hdc,0,0,SRCCOPY);

			DeleteObject(SelectObject(hdc,oldbmp));
			DeleteDC(hdc);

			EndPaint(hWnd,&ps);
			break;
		}

		case WM_LBUTTONDOWN:
		{
			if (g_bNewColorScreenPick)
				SendMessage(g_dlgMain,WM_COMMAND,MAKEWPARAM(ID_NEW_COLOR,0),
					(LPARAM)GetDlgItem(g_dlgMain,IDC_PRESET_TOOLBAR));

			SetCursor(LoadCursor(g_hInstance,MAKEINTRESOURCE(IDC_SCREEN_BUTTON)));
			SetCapture(hWnd);
			RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE);
			break;
		}

		case WM_MOUSEMOVE:
			if (GetCapture() == hWnd)
				RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE);
			break;

		case WM_LBUTTONUP:
		{
			ReleaseCapture();
			RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE);
			NMHDR nmh; nmh.code = NM_CLICK; nmh.hwndFrom = hWnd; nmh.idFrom = nID;
			SendMessage(GetParent(hWnd),WM_NOTIFY,nID,(LPARAM)&nmh);
			break;
		}

		default:
			return DefWindowProc(hWnd,uiMessage,wParam,lParam);
	}

	return 0;
}

LRESULT CALLBACK ColorListProc(HWND hWnd, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	WNDPROC wpfnOld = (WNDPROC)GetProp(hWnd,"OLDPROC");
	if (!wpfnOld)
		return DefWindowProc(hWnd,uiMessage,wParam,lParam);

	switch (uiMessage)
	{
		case WM_TIMER:
		{
			RECT rc; GetWindowRect(hWnd,&rc);
			POINT pt; GetCursorPos(&pt);

			if (pt.x < rc.left)
				SendMessage(hWnd,WM_HSCROLL,MAKEWPARAM(SB_LINELEFT,0),NULL);
			else if (pt.x > rc.right)
				SendMessage(hWnd,WM_HSCROLL,MAKEWPARAM(SB_LINERIGHT,0),NULL);

			break;
		}

		case WM_LBUTTONDOWN:
		{
			DWORD dwHitTest = SendMessage(hWnd,LB_ITEMFROMPOINT,0,lParam); // pass in x,y from lParam
			if (LOWORD(dwHitTest) >= 0)
			{
				g_dragData.enState = DragData::DragInRect;
				g_dragData.nColorIndex = LOWORD(dwHitTest);

				// create drag rectangle (if mouse is held and moved out of this rect, dragging begins)
				int x = GET_X_LPARAM(lParam);
				int y = GET_Y_LPARAM(lParam);
				int dcx = GetSystemMetrics(SM_CXDRAG);
				int dcy = GetSystemMetrics(SM_CYDRAG);
				SetRect(&g_dragData.rcDrag,x - dcx,y - dcy,x + dcx,y + dcy);
			}
			break;
		}

		case WM_MOUSEMOVE:
		{
			POINT pt = {GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};

			if (wParam & MK_LBUTTON)
			{
				// waiting to move out of rectangle before beginning dragging
				if (g_dragData.enState == DragData::DragInRect &&
					!PtInRect(&g_dragData.rcDrag,pt))
				{
					// mouse moved out of rect, we can begin dragging
					SetCapture(hWnd);
					SetTimer(hWnd,1,250,NULL);
					g_dragData.enState = DragData::DragDragging;
				}
				
				// NOTE: the above if statement might change the state to dragging
				// so we can't do "else if" here because this part would be skipped
				// the first time the mouse moved out of the box
				if (g_dragData.enState == DragData::DragDragging)
				{
					// dragging
					RECT rc; GetWindowRect(hWnd,&rc);
					POINT pt2; GetCursorPos(&pt2);
					UINT nCursor = (GetAsyncKeyState(VK_CONTROL) & 0x8000) ?
						IDC_POINTER_COPY : IDC_POINTER_DRAG;

					if (PtInRect(&rc,pt2))
					{
						// generate "cursor" position
						DWORD dwHitTest = SendMessage(hWnd,LB_ITEMFROMPOINT,0,lParam); // pass in x,y from lParam
						g_dragData.nDragCursor = LOWORD(dwHitTest);

						// if the cursor is 1/2 past the left coordinate of the item,
						// increase the cursor position by one
						RECT rcItem;
						if (LOWORD(dwHitTest) >= 0 &&
							SendMessage(hWnd,LB_GETITEMRECT,LOWORD(dwHitTest),(LPARAM)&rcItem) != LB_ERR)
						{
							if (pt.x > rcItem.left + (rcItem.right - rcItem.left) / 2)
								g_dragData.nDragCursor++;	
						}
					}
					else
					{
						// the mouse is outside of the window, use delete cursor
						g_dragData.nDragCursor = -1;
						nCursor = IDC_POINTER_DELETE;
					}

					SetCursor(LoadCursor(g_hInstance,MAKEINTRESOURCE(nCursor)));
					RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE);
				}
				return 0;
			}
			break;
		}

		case WM_LBUTTONUP:
		{
			if (g_dragData.enState == DragData::DragDragging)
			{
				// end drag
				RECT rcClient; GetClientRect(hWnd,&rcClient);
				POINT pt = {GET_X_LPARAM(lParam),GET_Y_LPARAM(lParam)};

				if (g_nSelPreset >= 0 && g_dragData.nColorIndex >= 0)
				{
					if (g_dragData.nDragCursor < 0)
					{
						// delete color
						g_vPresets[g_nSelPreset].colors.erase(
							g_vPresets[g_nSelPreset].colors.begin() + g_dragData.nColorIndex);
						SendMessage(hWnd,LB_SETCURSEL,g_dragData.nColorIndex,0);

						g_bPresetsModified = TRUE;
						UpdatePresets(FALSE,TRUE,TRUE);
					}
					else if (GetAsyncKeyState(VK_CONTROL) & 0x8000)
					{
						// copy color
						COLORREF clr = g_vPresets[g_nSelPreset].colors[g_dragData.nColorIndex];
						g_vPresets[g_nSelPreset].colors.insert(
							g_vPresets[g_nSelPreset].colors.begin() + g_dragData.nDragCursor,
							clr);
						SendMessage(hWnd,LB_SETCURSEL,g_dragData.nDragCursor,0);

						g_bPresetsModified = TRUE;
						UpdatePresets(FALSE,TRUE,TRUE);
					}
					else
					{
						if (g_dragData.nDragCursor > g_dragData.nColorIndex)
							g_dragData.nDragCursor--;

						if (g_dragData.nDragCursor != g_dragData.nColorIndex)
						{
							// move color
							COLORREF clr = g_vPresets[g_nSelPreset].colors[g_dragData.nColorIndex];
							g_vPresets[g_nSelPreset].colors.erase(
								g_vPresets[g_nSelPreset].colors.begin() + g_dragData.nColorIndex);
							g_vPresets[g_nSelPreset].colors.insert(
								g_vPresets[g_nSelPreset].colors.begin() + g_dragData.nDragCursor,
								clr);
							SendMessage(hWnd,LB_SETCURSEL,g_dragData.nDragCursor,0);

							g_bPresetsModified = TRUE;
							UpdatePresets(FALSE,TRUE,TRUE);
						}
					}
				}

				ReleaseCapture();
			}

			// fall-through
		}

		case WM_CANCELMODE:
			SetCursor(LoadCursor(NULL,MAKEINTRESOURCE(IDC_ARROW)));
			if (uiMessage == WM_CANCELMODE) ReleaseCapture();
			KillTimer(hWnd,1);

			g_dragData.enState = DragData::DragNone;

			RedrawWindow(hWnd,NULL,NULL,RDW_INVALIDATE);
			break;
	}

	return CallWindowProc(wpfnOld,hWnd,uiMessage,wParam,lParam);
}

INT_PTR CALLBACK AboutProc(HWND hDlg, UINT uiMessage, WPARAM wParam, LPARAM lParam)
{
	switch (uiMessage)
	{
		case WM_INITDIALOG:
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam))
			{
				case IDOK:
				case IDCANCEL:
					EndDialog(hDlg,LOWORD(wParam));
					break;
			}
			break;

		default:
			return FALSE;
	}
	
	return TRUE;
}
