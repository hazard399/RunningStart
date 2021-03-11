// RunningStart.cpp : Defines the entry point for the application.
//

#include "framework.h"
#include "windowsx.h"

#include <memory>

#include "MoveButton.h"

// Section for inject
#define MY_SEGMENT_NAME "mySeg"
#define INJECT_CONTROL_STRUCT_NAME "{41FB2C49-62BE-4E0F-AA1F-B99AC8216D4A}"
#define ALREADY_RUNNING_MUTEX_NAME "{84C1D070-52D8-4BB3-AC20-ABAA0A71622F}"
#ifdef _WIN64
#define MY_SIGNATURE 0x1234567812345678
#else
#define MY_SIGNATURE 0x12345678
#endif
#define INJECTPROC __declspec(code_seg(MY_SEGMENT_NAME)) __declspec(safebuffers) __declspec(noinline) static

#pragma optimize("", off )
#pragma runtime_checks("",off)

typedef HMODULE(WINAPI *ProcLoadLibraryA) (LPCSTR lpLibFileName);
typedef FARPROC(WINAPI *ProcGetProcAddress) (HMODULE hModule, LPCSTR lpProcName);
typedef LONG_PTR(WINAPI *ProcSetWindowLongPtrA) (HWND hWnd, int nIndex, LONG_PTR dwNewLong);
typedef LRESULT(WINAPI *ProcCallWindowProcA) (WNDPROC lpPrevWndFunc, HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
typedef HMODULE(WINAPI *ProcGetModuleHandleA) (LPCSTR lpModuleName);
typedef int (WINAPI *ProclstrcmpiA) (LPCSTR lpString1, LPCSTR lpString2);
typedef BOOL(WINAPI *ProcVirtualProtect) (LPVOID lpAddress, SIZE_T dwSize, DWORD flNewProtect, PDWORD lpflOldProtect);
typedef BOOL(WINAPI *ProcWriteProcessMemory) (HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T *lpNumberOfBytesWritten);
typedef BOOL(WINAPI *ProcUpdateLayeredWindowIndirect) (HWND hwnd, UPDATELAYEREDWINDOWINFO *pULWInfo);
typedef HANDLE(WINAPI *ProcCreateFileMappingA) (HANDLE hFile, LPSECURITY_ATTRIBUTES lpFileMappingAttributes, DWORD flProtect, DWORD dwMaximumSizeHigh, DWORD dwMaximumSizeLow, LPCSTR lpName);
typedef LPVOID(WINAPI *ProcMapViewOfFile) (HANDLE hFileMappingObject, DWORD dwDesiredAccess, DWORD dwFileOffsetHigh, DWORD dwFileOffsetLow, SIZE_T dwNumberOfBytesToMap);
typedef BOOL(WINAPI *ProcGetWindowRect) (HWND hWnd, LPRECT lpRect);
typedef HWND(WINAPI *ProcGetWindow) (HWND hWnd, UINT uCmd);
typedef BOOL(WINAPI *ProcUnionRect) (LPRECT lprcDst, const RECT *lprcSrc1, const RECT *lprcSrc2);
typedef BOOL(WINAPI *ProcSubtractRect) (LPRECT lprcDst, const RECT *lprcSrc1, const RECT *lprcSrc2);
typedef BOOL(WINAPI *ProcPtInRect) (const RECT *lprc, POINT pt);
typedef void(*ProcUpdateTrayRect) (void* param);
typedef HWND(WINAPIV *ProcWindowFromDC) (HDC hDC);

struct InjectControlStruct {
	BOOL doChangePosition;
	POINT startButtonPos;
	LONG startButtonWidth;
	LONG startButtonHeight;
};

struct InjectInfo {
	HWND hwndStart;
	WNDPROC wndProcStart;
	WNDPROC wndProcStartOld;
	HWND hwndTray;
	WNDPROC wndProcTray;
	WNDPROC wndProcTrayOld;
	char nameUser32[32];
	char nameSetWindowLongPtrA[32];
	char nameCallWindowProcA[32];
	char nameUpdateLayeredWindowIndirect[32];
	char nameGetWindowRect[32];
	char nameGetWindow[32];
	char nameUnionRect[32];
	char nameSubtractRect[32];
	char namePtInRect[32];
	char nameWindowFromDC[32];
	char nameInjectControl[sizeof(INJECT_CONTROL_STRUCT_NAME)];
	ProcLoadLibraryA procLoadLibraryA;
	ProcGetProcAddress procGetProcAddress;
	ProcCallWindowProcA procCallWindowProcA;	
	ProcGetModuleHandleA procGetModuleHandleA;
	ProclstrcmpiA proclstrcmpiA;
	ProcVirtualProtect procVirtualProtect;
	ProcWriteProcessMemory procWriteProcessMemory;
	ProcUpdateLayeredWindowIndirect procUpdateLayeredWindowIndirect;
	ProcUpdateLayeredWindowIndirect procUpdateLayeredWindowIndirectMy;
	ProcCreateFileMappingA procCreateFileMappingA;
	ProcMapViewOfFile procMapViewOfFile;
	ProcGetWindowRect procGetWindowRect;
	ProcGetWindow procGetWindow;
	ProcUnionRect procUnionRect;
	ProcSubtractRect procSubtractRect;
	ProcPtInRect procPtInRect;
	ProcUpdateTrayRect procUpdateTrayRect;
	ProcWindowFromDC procWindowFromDC;
	volatile InjectControlStruct* injectControl;
	RECT rectTray;
};

// Updating expected Start button position in tray window
INJECTPROC void UpdateTrayRect(InjectInfo* injectInfo) {
	injectInfo->procGetWindowRect(injectInfo->hwndTray, &injectInfo->rectTray);
	RECT rectChildren = {};
	if (HWND hwnd = injectInfo->procGetWindow(injectInfo->hwndTray, GW_CHILD)) {
		injectInfo->procGetWindowRect(hwnd, &rectChildren);
		while (hwnd = injectInfo->procGetWindow(hwnd, GW_HWNDNEXT)) {
			RECT rect;
			injectInfo->procGetWindowRect(hwnd, &rect);
			injectInfo->procUnionRect(&rectChildren, &rectChildren, &rect);
		}
	}
	injectInfo->procSubtractRect(&injectInfo->rectTray, &injectInfo->rectTray, &rectChildren);
}

INJECTPROC DWORD WINAPI InjectThreadProc(LPVOID lpParameter) {
	InjectInfo* injectInfo = (InjectInfo*)lpParameter;
	HANDLE hMap = injectInfo->procCreateFileMappingA(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(InjectControlStruct), injectInfo->nameInjectControl);
	injectInfo->injectControl = (InjectControlStruct*)injectInfo->procMapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, sizeof(InjectControlStruct));
	if (NULL == injectInfo->injectControl) return 0;
	// Getting User32 function pointers. Kernel32 function pointers are already there
	HMODULE hUser32 = injectInfo->procLoadLibraryA(injectInfo->nameUser32);
	injectInfo->procWindowFromDC = (ProcWindowFromDC)injectInfo->procGetProcAddress(hUser32, injectInfo->nameWindowFromDC);
	injectInfo->procGetWindowRect = (ProcGetWindowRect)injectInfo->procGetProcAddress(hUser32, injectInfo->nameGetWindowRect);
	injectInfo->procGetWindow = (ProcGetWindow)injectInfo->procGetProcAddress(hUser32, injectInfo->nameGetWindow);
	injectInfo->procUnionRect = (ProcUnionRect)injectInfo->procGetProcAddress(hUser32, injectInfo->nameUnionRect);
	injectInfo->procSubtractRect = (ProcSubtractRect)injectInfo->procGetProcAddress(hUser32, injectInfo->nameSubtractRect);
	injectInfo->procPtInRect = (ProcPtInRect)injectInfo->procGetProcAddress(hUser32, injectInfo->namePtInRect);
	injectInfo->procCallWindowProcA = (ProcCallWindowProcA)injectInfo->procGetProcAddress(hUser32, injectInfo->nameCallWindowProcA);
	// Subclassing windows
	ProcSetWindowLongPtrA setWindowLongPtrA = (ProcSetWindowLongPtrA)injectInfo->procGetProcAddress(hUser32, injectInfo->nameSetWindowLongPtrA);
	injectInfo->wndProcStartOld = (WNDPROC)setWindowLongPtrA(injectInfo->hwndStart, GWLP_WNDPROC, (LONG_PTR)injectInfo->wndProcStart);
	injectInfo->procUpdateTrayRect(injectInfo);
	injectInfo->wndProcTrayOld = (WNDPROC)setWindowLongPtrA(injectInfo->hwndTray, GWLP_WNDPROC, (LONG_PTR)injectInfo->wndProcTray);
	// Hooking UpdateLayeredWindowIndirect
	IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)injectInfo->procGetModuleHandleA(NULL);
	IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)((BYTE*)dosHeader + dosHeader->e_lfanew);
	IMAGE_DATA_DIRECTORY& importDirectory = ntHeaders->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
	if (0 == importDirectory.Size) return 0;
	IMAGE_IMPORT_DESCRIPTOR* importDescriptor = (IMAGE_IMPORT_DESCRIPTOR*)((BYTE*)dosHeader + importDirectory.VirtualAddress);
	for (; importDescriptor->Name; importDescriptor++) {
		LPSTR modName = (LPSTR)((BYTE*)dosHeader + importDescriptor->Name);
		if (0 == injectInfo->proclstrcmpiA(modName, injectInfo->nameUser32)) break;
	}
	if (0 == importDescriptor->Name) return 0;
	injectInfo->procUpdateLayeredWindowIndirect = (ProcUpdateLayeredWindowIndirect)injectInfo->procGetProcAddress(hUser32, injectInfo->nameUpdateLayeredWindowIndirect);
	IMAGE_THUNK_DATA* pThunk = (PIMAGE_THUNK_DATA)((PBYTE)dosHeader + importDescriptor->FirstThunk);
	for (; pThunk->u1.Function; pThunk++) {
		PROC* ppfn = (PROC*)&pThunk->u1.Function;
		if (*ppfn == (PROC)injectInfo->procUpdateLayeredWindowIndirect) {
			DWORD oldProtect;
			if (injectInfo->procVirtualProtect(ppfn, sizeof(PROC), PAGE_EXECUTE_READWRITE, &oldProtect)) {
				injectInfo->procWriteProcessMemory((HANDLE)-1, ppfn, &injectInfo->procUpdateLayeredWindowIndirectMy, sizeof(PROC), NULL);
				injectInfo->procVirtualProtect(ppfn, sizeof(PROC), oldProtect, &oldProtect);
			}
			break;
		}
	}
	return 0;
}

INJECTPROC LRESULT CALLBACK InjectWndProcStart(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	InjectInfo* injectInfo = (InjectInfo*)MY_SIGNATURE;
	BOOL ret = FALSE;
	// Win10. Don't let Start button to change size or move
	if (WM_WINDOWPOSCHANGING == message) {
		WINDOWPOS* winPos = (WINDOWPOS*)lParam;
		if ((!(winPos->flags & SWP_NOMOVE) || !(winPos->flags & SWP_NOSIZE)) && injectInfo->injectControl->doChangePosition) {
			winPos->x = injectInfo->injectControl->startButtonPos.x;
			winPos->y = injectInfo->injectControl->startButtonPos.y;
			winPos->cx = injectInfo->injectControl->startButtonWidth;
			winPos->cy = injectInfo->injectControl->startButtonHeight;
			return 0;
		}
	}
	// Win7. System paints Start button two times - once itself and once in tray window (maybe they have some legacy code or it covers some corner cases what I don't know about). So prevent doing it second time
	else if (WM_PRINTCLIENT == message) {
		HWND hwnd = injectInfo->procWindowFromDC((HDC)wParam);
		if (hwnd == injectInfo->hwndTray && injectInfo->injectControl->doChangePosition) return 0;
	}
	return injectInfo->procCallWindowProcA(injectInfo->wndProcStartOld, hWnd, message, wParam, lParam);
}

INJECTPROC LRESULT CALLBACK InjectWndProcTray(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
	InjectInfo* injectInfo = (InjectInfo*)MY_SIGNATURE;
	// Tray window moved somehow
	if (WM_SETTINGCHANGE == message && SPI_SETWORKAREA == wParam) {
		injectInfo->procUpdateTrayRect(injectInfo);
	}
	// Disable clicking on tray window where Start button is supposed to be - otherwise it will trigger that way as well
	else if (WM_NCLBUTTONDOWN == message || WM_NCMOUSEMOVE == message) {
		if (HTCAPTION == wParam) {
			if (injectInfo->injectControl->doChangePosition
				&& injectInfo->procPtInRect(&injectInfo->rectTray, { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) })) {
				return 0;
			}
		}
	}
	return injectInfo->procCallWindowProcA(injectInfo->wndProcTrayOld, hWnd, message, wParam, lParam);
}

// Win7
INJECTPROC BOOL WINAPI UpdateLayeredWindowIndirectMy(HWND hwnd, UPDATELAYEREDWINDOWINFO *pULWInfo) {
	InjectInfo* injectInfo = (InjectInfo*)MY_SIGNATURE;
	// Prevent moving Start button
	if (hwnd == injectInfo->hwndStart) {
		if (NULL != pULWInfo->pptDst) {
			if (injectInfo->injectControl->doChangePosition) {
				((POINT*)pULWInfo->pptDst)->x = injectInfo->injectControl->startButtonPos.x;
				((POINT*)pULWInfo->pptDst)->y = injectInfo->injectControl->startButtonPos.y;
			}
		}
	}
	return injectInfo->procUpdateLayeredWindowIndirect(hwnd, pULWInfo);
}

#pragma runtime_checks("", restore)
#pragma optimize("", on)

BOOL GetWindows(HWND* hwndStart, HWND* hwndTray) {
	*hwndTray = FindWindow(_T("Shell_TrayWnd"), NULL);
	*hwndStart = FindWindow(_T("Button"), _T("Start"));
	if (NULL == *hwndStart) *hwndStart = FindWindow(_T("Start"), _T("Start"));
	if (NULL == *hwndStart) *hwndStart = FindWindowEx(*hwndTray, NULL, _T("Start"), _T("Start"));	
	return *hwndStart != NULL && *hwndTray != NULL;
}

BOOL MakeInject(HWND hwndStart, HWND hwndTray) {
#ifndef _WIN64
	BOOL is64 = FALSE;
	// Can't inject from 32bit process into 64bit system
	if (IsWow64Process(GetCurrentProcess(), &is64) && is64) return FALSE;
#endif
	InjectInfo injectInfo = {};
	injectInfo.hwndStart = hwndStart;
	injectInfo.hwndTray = hwndTray;
	BYTE* sectionPtr = NULL;
	DWORD sectionSize = 0;
	IMAGE_DOS_HEADER* dosHeader = (IMAGE_DOS_HEADER*)GetModuleHandle(NULL);
	//if (IMAGE_DOS_SIGNATURE != dosHeader->e_magic) return FALSE;
	IMAGE_NT_HEADERS* ntHeaders = (IMAGE_NT_HEADERS*)((BYTE*)dosHeader + dosHeader->e_lfanew);
	//if (IMAGE_NT_SIGNATURE != ntHeaders->Signature) return FALSE;
	IMAGE_FILE_HEADER& fileHeader = ntHeaders->FileHeader;
	IMAGE_SECTION_HEADER* sectionHeader = IMAGE_FIRST_SECTION(ntHeaders);
	for (int i = 0; i < fileHeader.NumberOfSections; ++i, ++sectionHeader) {
		if (0 == strcmp((const char*)sectionHeader->Name, MY_SEGMENT_NAME)) {
			sectionPtr = (BYTE*)dosHeader + sectionHeader->VirtualAddress;
			sectionSize = sectionHeader->Misc.VirtualSize;
			break;
		}
	}
	if (0 == sectionSize) return FALSE;
	DWORD procId;
	GetWindowThreadProcessId(injectInfo.hwndStart, &procId);
	HANDLE hProcess = OpenProcess(PROCESS_CREATE_THREAD | PROCESS_QUERY_INFORMATION | PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_VM_READ, FALSE, procId);
	if (NULL == hProcess) return FALSE;
	LPVOID injectInfoRemote = VirtualAllocEx(hProcess, NULL, sizeof(injectInfo) + sectionSize, MEM_COMMIT, PAGE_EXECUTE_READWRITE);
	if (NULL == injectInfoRemote) return FALSE;
	LPVOID sectionPtrRemote = (BYTE*)injectInfoRemote + sizeof(injectInfo);
	strcpy_s(injectInfo.nameUser32, "user32.dll");
	strcpy_s(injectInfo.nameCallWindowProcA, "CallWindowProcA");
#ifdef _WIN64
	strcpy_s(injectInfo.nameSetWindowLongPtrA, "SetWindowLongPtrA");
#else
	strcpy_s(injectInfo.nameSetWindowLongPtrA, "SetWindowLongA");
#endif
	strcpy_s(injectInfo.nameGetWindowRect, "GetWindowRect");
	strcpy_s(injectInfo.nameGetWindow, "GetWindow");
	strcpy_s(injectInfo.nameUnionRect, "UnionRect");
	strcpy_s(injectInfo.nameSubtractRect, "SubtractRect");
	strcpy_s(injectInfo.namePtInRect, "PtInRect");
	strcpy_s(injectInfo.nameInjectControl, INJECT_CONTROL_STRUCT_NAME);
	HMODULE hKernel32 = LoadLibrary(_T("kernel32.dll"));
	injectInfo.procLoadLibraryA = (ProcLoadLibraryA)GetProcAddress(hKernel32, "LoadLibraryA");
	injectInfo.procGetProcAddress = (ProcGetProcAddress)GetProcAddress(hKernel32, "GetProcAddress");
	injectInfo.wndProcStart = (WNDPROC)((BYTE*)sectionPtrRemote + ((BYTE*)&InjectWndProcStart - sectionPtr));
	injectInfo.wndProcTray = (WNDPROC)((BYTE*)sectionPtrRemote + ((BYTE*)&InjectWndProcTray - sectionPtr));
	injectInfo.procUpdateTrayRect = (ProcUpdateTrayRect)((BYTE*)sectionPtrRemote + ((BYTE*)&UpdateTrayRect - sectionPtr));
	injectInfo.procGetModuleHandleA = (ProcGetModuleHandleA)GetProcAddress(hKernel32, "GetModuleHandleA");
	injectInfo.proclstrcmpiA = (ProclstrcmpiA)GetProcAddress(hKernel32, "lstrcmpiA");
	injectInfo.procVirtualProtect = (ProcVirtualProtect)GetProcAddress(hKernel32, "VirtualProtect");
	injectInfo.procWriteProcessMemory = (ProcWriteProcessMemory)GetProcAddress(hKernel32, "WriteProcessMemory");
	injectInfo.procCreateFileMappingA = (ProcCreateFileMappingA)GetProcAddress(hKernel32, "CreateFileMappingA");
	injectInfo.procMapViewOfFile = (ProcMapViewOfFile)GetProcAddress(hKernel32, "MapViewOfFile");
	strcpy_s(injectInfo.nameUpdateLayeredWindowIndirect, "UpdateLayeredWindowIndirect");
	injectInfo.procUpdateLayeredWindowIndirectMy = (ProcUpdateLayeredWindowIndirect)((BYTE*)sectionPtrRemote + ((BYTE*)&UpdateLayeredWindowIndirectMy - sectionPtr));
	strcpy_s(injectInfo.nameWindowFromDC, "WindowFromDC");
	auto injectBuf = std::make_unique<BYTE[]>(sectionSize);
	std::copy(sectionPtr, sectionPtr + sectionSize, injectBuf.get());
	for (BYTE* p = injectBuf.get(); p < injectBuf.get() + (sectionSize - sizeof(DWORD_PTR) + 1); ++p) {
		if (*((DWORD_PTR*)p) == MY_SIGNATURE) {
			*((DWORD_PTR**)p) = (DWORD_PTR*)injectInfoRemote;
			p += sizeof(DWORD_PTR) - 1;
		}
	}
	// Injecting information structure
	if (WriteProcessMemory(hProcess, injectInfoRemote, &injectInfo, sizeof(injectInfo), NULL)
		// Injecting whole section with code
		&& WriteProcessMemory(hProcess, sectionPtrRemote, injectBuf.get(), sectionSize, NULL)) {
		// Run
		CloseHandle(CreateRemoteThread(hProcess, NULL, 0, (LPTHREAD_START_ROUTINE)((BYTE*)sectionPtrRemote + ((BYTE*)&InjectThreadProc - sectionPtr)), injectInfoRemote, 0, NULL));
	}
	CloseHandle(hProcess);
	return TRUE;
}

int APIENTRY wWinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE /*hPrevInstance*/, _In_ LPWSTR /*lpCmdLine*/, _In_ int /*nCmdShow*/) {
	// Only one instance per user
	HANDLE hMutex = CreateMutex(NULL, FALSE, _T(ALREADY_RUNNING_MUTEX_NAME));
	if ((NULL == hMutex) || (ERROR_ALREADY_EXISTS == ::GetLastError())) return 0;
	HWND hwndStart, hwndTray;
	if (!GetWindows(&hwndStart, &hwndTray)) return 0;
	// Hot key for exit
	if (!RegisterHotKey(NULL, 1, MOD_CONTROL | MOD_NOREPEAT, VK_F12)) return 0;
	HANDLE hMap = CreateFileMapping(INVALID_HANDLE_VALUE, NULL, PAGE_READWRITE, 0, sizeof(InjectControlStruct), _T(INJECT_CONTROL_STRUCT_NAME));
	DWORD mappingErr = GetLastError();
	volatile InjectControlStruct* injectControl = (InjectControlStruct*)MapViewOfFile(hMap, FILE_MAP_WRITE, 0, 0, sizeof(InjectControlStruct));	
	if (0 == mappingErr) {
		if (!MakeInject(hwndStart, hwndTray)) return 0;
	}
	else if (ERROR_ALREADY_EXISTS != mappingErr) return 0;
	RECT rectStart;
	GetWindowRect(hwndStart, &rectStart);
	// Win10. Setting Start button free to go
	LONG_PTR styleStart = GetWindowLongPtr(hwndStart, GWL_STYLE);
	if (!(WS_POPUP & styleStart)) {
		SetWindowLongPtr(hwndStart, GWL_STYLE, styleStart | WS_POPUP);
		SetParent(hwndStart, NULL);
		SetWindowLongPtr(hwndStart, GWLP_HWNDPARENT, (LONG_PTR)hwndTray);
		SetWindowPos(hwndStart, HWND_TOPMOST, rectStart.left, rectStart.top, 0, 0, SWP_NOSENDCHANGING | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE);
	}
	injectControl->startButtonPos.x = rectStart.left;
	injectControl->startButtonPos.y = rectStart.top;
	injectControl->startButtonWidth = rectStart.right - rectStart.left;
	injectControl->startButtonHeight = rectStart.bottom - rectStart.top;
	injectControl->doChangePosition = TRUE;
	MoveButton moveButton(hwndStart, &rectStart, &injectControl->startButtonPos);
	POINT lastCursorPos;
	GetCursorPos(&lastCursorPos);
	UINT_PTR timer = SetTimer(NULL, 0, 30, NULL);;
	RECT rectTray;
	BOOL bRet;
	MSG msg;
	// Main cycle 
	while ((bRet = GetMessage(&msg, NULL, 0, 0)) != 0) {
		if (-1 == bRet) continue;
		// Exiting
		if (WM_HOTKEY == msg.message) {
			injectControl->doChangePosition = FALSE;
			LONG_PTR styleStart = GetWindowLongPtr(hwndStart, GWL_STYLE);
			// Painting Start button back
			if (WS_POPUP & styleStart) {
				if (WS_CHILD & styleStart) {
					SetWindowLongPtr(hwndStart, GWL_STYLE, styleStart ^ WS_POPUP);
					SetParent(hwndStart, hwndTray);
					SetWindowPos(hwndStart, 0, 0, 0, 0, 0, SWP_NOSENDCHANGING | SWP_NOACTIVATE | SWP_NOOWNERZORDER | SWP_NOSIZE | SWP_NOZORDER);
				}
				else {
					SendMessage(hwndStart, WM_MOUSEMOVE, 0, 0);
					RedrawWindow(hwndTray, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);
				}
			}
			return 0;
		}
		// Updating Start button position if needed
		else if (WM_TIMER == msg.message) {
			if (!moveButton.IsMoving()) {
				if (moveButton.BeginMoveIfNeeded(msg.pt, lastCursorPos)) GetWindowRect(hwndTray, &rectTray);
			}
			if (moveButton.IsMoving()) {
				moveButton.GetButtonRect(&rectStart);
				BOOL isIntersect = IntersectRect(&rectStart, &rectStart, &rectTray);
				moveButton.Step(msg.pt, lastCursorPos);
				// Removing visible artefacts from tray window
				if (isIntersect) RedrawWindow(hwndTray, NULL, NULL, RDW_INVALIDATE | RDW_ALLCHILDREN);				
			}
			lastCursorPos = msg.pt;
		}
	}
	return (int)msg.wParam;
}