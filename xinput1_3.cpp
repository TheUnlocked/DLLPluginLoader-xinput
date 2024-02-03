#include <Windows.h>
#include <DbgHelp.h>
#include <vector>
#include <fstream>
#include "ReplaceImport.h"


std::vector<const char *> sImportFunctionNames = 
{ 
"DllMain_stub",
"XInputGetState",
"XInputSetState",
"XInputGetCapabilities",
"XInputEnable",
"XInputGetDSoundAudioDeviceGuids",
"XInputGetBatteryInformation",
"XInputGetKeystroke",
};

extern "C" uintptr_t	iImportFunctions[8] = { 0 };

extern "C" void DllMain_stub();
extern "C" void XInputGetState();
extern "C" void XInputSetState();
extern "C" void XInputGetCapabilities();
extern "C" void XInputEnable();
extern "C" void XInputGetDSoundAudioDeviceGuids();
extern "C" void XInputGetBatteryInformation();
extern "C" void XInputGetKeystroke();

HINSTANCE				pOriginalHinst = nullptr;
HINSTANCE				pWarpperHinst = nullptr;
std::vector<HINSTANCE>  loadedPlugins;

PROC					Init_Original = nullptr;


std::string GetPluginsDirectory()
{
	return "NativeMods\\";
}

void Error(const char * msg)
{
	MessageBoxA(nullptr, msg, "NativeModLoader", 0);
}

int LoadDLLPlugin(const char * path)
{
	int state = -1;
	__try
	{
		HINSTANCE plugin = LoadLibraryA(path);
		if (!plugin)
			return 0;
		
		int ok = 1;
		FARPROC fnInit = GetProcAddress(plugin, "Init");
		if (fnInit != nullptr)
		{
			state = -2;
			((void(__cdecl *)())fnInit)();
			ok = 2;
		}

		loadedPlugins.push_back(plugin);
		return ok;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{

	}

	return state;
}


void LoadLib()
{
	static bool isLoaded = false;
	if (!isLoaded)
	{
		isLoaded = true;

		std::ofstream fLog = std::ofstream("NativeModLoader.log");
		WIN32_FIND_DATAA wfd;
		std::string dir = GetPluginsDirectory();
		std::string search_dir = dir + "*.dll";
		HANDLE hFind = FindFirstFileA(search_dir.c_str(), &wfd);
		if (hFind != INVALID_HANDLE_VALUE)
		{
			int i_error = 0;
			do
			{
				if ((wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0)
					continue;

				std::string name = wfd.cFileName;
				name = dir + name;

				if (fLog.good())
					fLog << "Checking \"" << name.c_str() << "\" ... ";

				int result = LoadDLLPlugin(name.c_str());
				switch (result)
				{
				case 2:
				{
					if (fLog.good())
						fLog << "OK - loaded and called Init().\n";
					break;
				}

				case 1:
				{
					if (fLog.good())
						fLog << "OK - loaded.\n";
					break;
				}

				case 0:
				{
					if (fLog.good())
						fLog << "LoadLibrary failed!\n";
					i_error = 1;
					std::string err = "LoadLibrary failed on ";
					err = err + name;
					Error(err.c_str());
					break;
				}

				case -1:
				{
					if (fLog.good())
						fLog << "LoadLibrary crashed! This means there's a problem in the plugin DLL file.\n";
					i_error = 1;
					std::string err = "LoadLibrary crashed on ";
					err = err + name;
					err = err + ". This means there's a problem in the plugin DLL file. Contact the author of that plugin.";
					Error(err.c_str());
					break;
				}

				case -2:
				{
					if (fLog.good())
						fLog << "Init() crashed! This means there's a problem in the plugin DLL file.\n";
					i_error = 1;
					std::string err = "Init() crashed on ";
					err = err + name;
					err = err + ". This means there's a problem in the plugin DLL file. Contact the author of that plugin.";
					Error(err.c_str());
					break;
				}
				}
			} while (i_error == 0 && FindNextFileA(hFind, &wfd));

			FindClose(hFind);
		}
		else
		{
			if (fLog.good())
				fLog << "Failed to get search handle to \"" << search_dir.c_str() << "\"!\n";
		}
	}
}

PVOID Init_Hook(PVOID arg1, PVOID arg2)
{
	LoadLib();

	return ((PVOID(*)(PVOID, PVOID))Init_Original)(arg1, arg2);
}


BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	pWarpperHinst = hinstDLL;
	if (fdwReason == DLL_PROCESS_ATTACH)
	{
		char dllpath[MAX_PATH];
		GetSystemDirectoryA(dllpath, MAX_PATH);
		strcat(dllpath, "\\xinput1_3.dll");
		pOriginalHinst = LoadLibraryA(dllpath);
		if (!pOriginalHinst)
		{
			Error("Failed to load xinput1_3.dll!");
			return FALSE;
		}

		for (size_t i = 0; i < sImportFunctionNames.size(); i++)
			iImportFunctions[i] = reinterpret_cast<uintptr_t>(GetProcAddress(pOriginalHinst, sImportFunctionNames[i]));

		int result = ReplaceImport::Replace("api-ms-win-crt-runtime-l1-1-0.dll", "_initterm_e", (PROC)Init_Hook, &Init_Original);
		switch (result)
		{
		case 0: break;
		case 1: Error("Failed to get handle to main module!"); break;
		case 2: Error("Failed to find import table in executable!"); break;
		case 3: Error("Failed to change protection flags on memory page!"); break;
		case 4: Error("Failed to find API function in module!"); break;
		case 5: Error("Failed to find module!"); break;
		default: Error("Unknown error occurred!"); break;
		}
	}
	else if (fdwReason == DLL_PROCESS_DETACH)
	{
		FreeLibrary(pOriginalHinst);

		if (!loadedPlugins.empty())
		{
			for(auto plugin : loadedPlugins)
				FreeLibrary(plugin);
			loadedPlugins.clear();
		}
	}
	return TRUE;
}
