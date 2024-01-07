//#include <vld.h> // Visual Leak Detector

#include <atlbase.h>       // base ATL classes
#include <atlapp.h>        // base WTL classes
#include "Architecture.h"

CAppModule _Module;

#include "MainGui.h"
#include "Scylla.h"

MainGui* pMainGui = NULL; // for Logger
HINSTANCE hDllModule = 0;
bool IsDllMode = false;

LONG WINAPI HandleUnknownException(struct _EXCEPTION_POINTERS *ExceptionInfo);
void AddExceptionHandler();
void RemoveExceptionHandler();
int InitializeGui(HINSTANCE hInstance, LPARAM param);

int APIENTRY _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow)
{
	AddExceptionHandler();

	return InitializeGui(hInstance, (LPARAM)0);
}

int InitializeGui(HINSTANCE hInstance, LPARAM param)
{
	CoInitialize(NULL);

	AtlInitCommonControls(ICC_LISTVIEW_CLASSES | ICC_TREEVIEW_CLASSES);

	Scylla::initAsGuiApp();
	Scylla::config[DEBUG_PRIVILEGE].setTrue();
	Scylla::config[IAT_SECTION_NAME].setString(_T(".SCY"));
	Scylla::config[IAT_FIX_AND_OEP_FIX].setTrue();
	Scylla::config[USE_ADVANCED_IAT_SEARCH].setTrue();

	IsDllMode = false;

	HRESULT hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	

	int nRet = 0;
	// BLOCK: Run application
	{
		MainGui dlgMain;
		pMainGui = &dlgMain; // o_O

		CMessageLoop loop;
		_Module.AddMessageLoop(&loop);

		dlgMain.Create(GetDesktopWindow(), param);

		dlgMain.ShowWindow(SW_SHOW);

		loop.Run();
	}

	_Module.Term();
	CoUninitialize();

	return nRet;
}

void InitializeDll(HINSTANCE hinstDLL)
{
	hDllModule = hinstDLL;
	IsDllMode = true;
	Scylla::initAsDll();
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	// Perform actions based on the reason for calling.
	switch(fdwReason) 
	{
	case DLL_PROCESS_ATTACH:
		// Initialize once for each new process.
		// Return FALSE to fail DLL load.
		AddExceptionHandler();
		InitializeDll(hinstDLL);
		break;

	case DLL_THREAD_ATTACH:
		// Do thread-specific initialization.
		break;

	case DLL_THREAD_DETACH:
		// Do thread-specific cleanup.
		break;

	case DLL_PROCESS_DETACH:
		// Perform any necessary cleanup.
		RemoveExceptionHandler();
		break;
	}
	return TRUE;  // Successful DLL_PROCESS_ATTACH.
}

LPTOP_LEVEL_EXCEPTION_FILTER oldFilter;

void AddExceptionHandler()
{
	oldFilter = SetUnhandledExceptionFilter(HandleUnknownException);
}
void RemoveExceptionHandler()
{
	SetUnhandledExceptionFilter(oldFilter);
}

LONG WINAPI HandleUnknownException(struct _EXCEPTION_POINTERS *ExceptionInfo)
{
	WCHAR registerInfo[220];
	WCHAR filepath[MAX_PATH] = {0};
	WCHAR file[MAX_PATH] = {0};
	WCHAR message[MAX_PATH + 200 + _countof(registerInfo)];
	WCHAR osInfo[100];
	DWORD_PTR baseAddress = 0;
	DWORD_PTR address = (DWORD_PTR)ExceptionInfo->ExceptionRecord->ExceptionAddress;

	wcscpy_s(filepath, L"unknown");
	wcscpy_s(file, L"unknown");

	if (GetMappedFileNameW(GetCurrentProcess(), (LPVOID)address, filepath, _countof(filepath)) > 0)
	{
		WCHAR *temp = wcsrchr(filepath, '\\');
		if (temp)
		{
			temp++;
			wcscpy_s(file, temp);
		}
	}

	swprintf_s(osInfo, _countof(osInfo), TEXT("Exception! Please report it! OS: %X"), GetVersion());

	DWORD_PTR moduleBase = (DWORD_PTR)GetModuleHandleW(file);
	
	swprintf_s(message, _countof(message), TEXT("ExceptionCode %08X\r\nExceptionFlags %08X\r\nNumberParameters %08X\r\nExceptionAddress VA ")TEXT(PRINTF_DWORD_PTR_FULL_S)TEXT(" - Base ")TEXT(PRINTF_DWORD_PTR_FULL_S)TEXT("\r\nExceptionAddress module %s\r\n\r\n"), 
	ExceptionInfo->ExceptionRecord->ExceptionCode,
	ExceptionInfo->ExceptionRecord->ExceptionFlags, 
	ExceptionInfo->ExceptionRecord->NumberParameters, 
	address,
	moduleBase,
	file);

#ifdef _WIN64
	swprintf_s(registerInfo, _countof(registerInfo),TEXT("rax=0x" PRINTF_DWORD_PTR_FULL ", rbx=0x" PRINTF_DWORD_PTR_FULL ", rdx=0x" PRINTF_DWORD_PTR_FULL ", rcx=0x" PRINTF_DWORD_PTR_FULL ", rsi=0x" PRINTF_DWORD_PTR_FULL ", rdi=0x" PRINTF_DWORD_PTR_FULL ", rbp=0x" PRINTF_DWORD_PTR_FULL ", rsp=0x" PRINTF_DWORD_PTR_FULL ", rip=0x" PRINTF_DWORD_PTR_FULL ""),
		ExceptionInfo->ContextRecord->Rax,
		ExceptionInfo->ContextRecord->Rbx,
		ExceptionInfo->ContextRecord->Rdx,
		ExceptionInfo->ContextRecord->Rcx,
		ExceptionInfo->ContextRecord->Rsi,
		ExceptionInfo->ContextRecord->Rdi,
		ExceptionInfo->ContextRecord->Rbp,
		ExceptionInfo->ContextRecord->Rsp,
		ExceptionInfo->ContextRecord->Rip
		);
#else
	swprintf_s(registerInfo, _countof(registerInfo),"eax=0x" PRINTF_DWORD_PTR_FULL ", ebx=0x" PRINTF_DWORD_PTR_FULL ", edx=0x" PRINTF_DWORD_PTR_FULL ", ecx=0x" PRINTF_DWORD_PTR_FULL ", esi=0x" PRINTF_DWORD_PTR_FULL ", edi=0x" PRINTF_DWORD_PTR_FULL ", ebp=0x" PRINTF_DWORD_PTR_FULL ", esp=0x" PRINTF_DWORD_PTR_FULL ", eip=0x" PRINTF_DWORD_PTR_FULL "",
		ExceptionInfo->ContextRecord->Eax,
		ExceptionInfo->ContextRecord->Ebx,
		ExceptionInfo->ContextRecord->Edx,
		ExceptionInfo->ContextRecord->Ecx,
		ExceptionInfo->ContextRecord->Esi,
		ExceptionInfo->ContextRecord->Edi,
		ExceptionInfo->ContextRecord->Ebp,
		ExceptionInfo->ContextRecord->Esp,
		ExceptionInfo->ContextRecord->Eip
		);
#endif

	wcscat_s(message, _countof(message), registerInfo);

	MessageBox(0, message, osInfo, MB_ICONERROR);

	return EXCEPTION_CONTINUE_SEARCH;
}
