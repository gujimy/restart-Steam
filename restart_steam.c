// restart_steam.c
#include <shobjidl.h>
#include <shlguid.h>
#include <objbase.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include <strsafe.h>
#include <locale.h>  // 添加locale.h以使用setlocale函数
#include <psapi.h>   // 添加psapi.h以使用GetModuleFileNameEx函数
#include "resource.h"  // 包含资源头文件

// 添加注册表查询需要的头文件
#include <windows.h>
#include <winreg.h>

#define MAX_PATH_LENGTH 1024
#define STEAM_PROCESS_NAME "steam.exe"
#define STEAM_SHORTCUT_NAME "Steam.lnk"
#define STEAMPATH_CACHE_FILE "steam_path.cache"

// Steam注册表路径
#define STEAM_REG_PATH "SOFTWARE\\Valve\\Steam"
#define STEAM_REG_PATH_WOW64 "SOFTWARE\\Wow6432Node\\Valve\\Steam"
#define STEAM_REG_KEY "InstallPath"

// 为GCC编译添加COM接口GUID定义
#ifdef __GNUC__
// IID_IPersistFile的GUID
const GUID IID_IPersistFile = 
    {0x0000010b, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

// IID_IShellLinkA的GUID
const GUID IID_IShellLinkA = 
    {0x000214ee, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};

// CLSID_ShellLink的GUID
const GUID CLSID_ShellLink = 
    {0x00021401, 0x0000, 0x0000, {0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46}};
#endif

// 从注册表获取Steam路径
int getSteamPathFromRegistry(char* steamPath, size_t steamPathSize) {
    HKEY hKey;
    DWORD dataType = REG_SZ;
    DWORD dataSize = (DWORD)steamPathSize;
    LONG result;
    
    // 检查参数有效性
    if (!steamPath || steamPathSize == 0) return -1;
    
    // 按优先顺序尝试不同注册表位置
    const char* regPaths[] = {
        STEAM_REG_PATH,         // HKLM 64位
        STEAM_REG_PATH_WOW64,   // HKLM 32位
        STEAM_REG_PATH,         // HKCU 64位
        STEAM_REG_PATH_WOW64    // HKCU 32位
    };
    
    HKEY roots[] = {
        HKEY_LOCAL_MACHINE,
        HKEY_LOCAL_MACHINE,
        HKEY_CURRENT_USER,
        HKEY_CURRENT_USER
    };
    
    for (int i = 0; i < 4; i++) {
        result = RegOpenKeyExA(roots[i], regPaths[i], 0, KEY_READ, &hKey);
        if (result == ERROR_SUCCESS) {
            result = RegQueryValueExA(hKey, STEAM_REG_KEY, NULL, &dataType, (LPBYTE)steamPath, &dataSize);
            RegCloseKey(hKey);
            
            if (result == ERROR_SUCCESS) {
                // 确保路径以null结尾
                if (dataSize >= steamPathSize) {
                    steamPath[steamPathSize - 1] = '\0';
                } else if (dataSize > 0 && steamPath[dataSize - 1] != '\0') {
                    steamPath[dataSize] = '\0';
                }
                return 0;
            }
        }
    }
    
    return -1;
}

// 获取当前运行的Steam进程路径
int getSteamPathFromProcess(char* steamPath, size_t steamPathSize) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    
    if (!steamPath || steamPathSize == 0) return -1;
    
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) return -1;
    
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            if (_stricmp(pe32.szExeFile, STEAM_PROCESS_NAME) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    DWORD result = GetModuleFileNameExA(hProcess, NULL, steamPath, (DWORD)steamPathSize);
                    CloseHandle(hProcess);
                    CloseHandle(hProcessSnap);
                    return result ? 0 : -1;
                }
                break;
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }
    
    CloseHandle(hProcessSnap);
    return -1;
}

// 保存Steam路径到缓存文件
void saveSteamPathToCache(const char* steamPath) {
    FILE* file = fopen(STEAMPATH_CACHE_FILE, "w");
    if (file) {
        fprintf(file, "%s", steamPath);
        fclose(file);
    }
}

// 从缓存文件读取Steam路径
int getSteamPathFromCache(char* steamPath, size_t steamPathSize) {
    FILE* file = fopen(STEAMPATH_CACHE_FILE, "r");
    if (!file) return -1;
    
    if (fgets(steamPath, (int)steamPathSize, file)) {
        // 去除可能的换行符
        size_t len = strlen(steamPath);
        if (len > 0 && (steamPath[len-1] == '\n' || steamPath[len-1] == '\r')) {
            steamPath[len-1] = '\0';
        }
        
        fclose(file);
        // 快速检查路径是否有效
        return (GetFileAttributesA(steamPath) != INVALID_FILE_ATTRIBUTES) ? 0 : -1;
    }
    
    fclose(file);
    return -1;
}

// 查找常见驱动器中的Steam
int findSteamInCommonDrives(char* steamPath, size_t steamPathSize) {
    // 常见的Steam安装位置
    static const char* commonPaths[] = {
        ":\\Steam\\%s",
        ":\\Games\\Steam\\%s",
        ":\\SteamLibrary\\%s",
        ":\\Program Files\\Steam\\%s",
        ":\\Program Files (x86)\\Steam\\%s"
    };
    
    char testPath[MAX_PATH];
    // 检查所有可能的驱动器盘符
    for (char driveLetter = 'C'; driveLetter <= 'Z'; driveLetter++) {
        for (int i = 0; i < sizeof(commonPaths)/sizeof(commonPaths[0]); i++) {
            sprintf(testPath, "%c%s", driveLetter, commonPaths[i]);
            char fullPath[MAX_PATH];
            sprintf(fullPath, testPath, STEAM_PROCESS_NAME);
            
            if (GetFileAttributesA(fullPath) != INVALID_FILE_ATTRIBUTES) {
                StringCchCopyA(steamPath, steamPathSize, fullPath);
                return 0;
            }
        }
    }
    
    return -1;
}

// 获取Steam快捷方式的目标路径
int getSteamPathFromShortcut(const char* shortcutPath, char* targetPath, size_t targetPathSize) {
    HRESULT hr;
    IShellLink* pShellLink = NULL;
    IPersistFile* pPersistFile = NULL;
    char wTargetPath[MAX_PATH] = {0};
    WCHAR wShortcutPath[MAX_PATH] = {0};
    int result = -1;
    
    if (!shortcutPath || !targetPath || targetPathSize == 0) return -1;
    
    // 初始化COM (使用更高效的模式)
    hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hr)) return -1;
    
    // 创建ShellLink实例
    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IShellLinkA, (void**)&pShellLink);
    if (SUCCEEDED(hr)) {
        hr = pShellLink->lpVtbl->QueryInterface(pShellLink, &IID_IPersistFile,
                                              (void**)&pPersistFile);
        if (SUCCEEDED(hr)) {
            MultiByteToWideChar(CP_ACP, 0, shortcutPath, -1, wShortcutPath, MAX_PATH);
            if (SUCCEEDED(pPersistFile->lpVtbl->Load(pPersistFile, wShortcutPath, STGM_READ))) {
                if (SUCCEEDED(pShellLink->lpVtbl->GetPath(pShellLink, wTargetPath,
                                                       MAX_PATH, NULL, SLGP_UNCPRIORITY)) 
                    && wTargetPath[0] != '\0') {
                    StringCchCopyA(targetPath, targetPathSize, wTargetPath);
                    result = 0;
                }
            }
            pPersistFile->lpVtbl->Release(pPersistFile);
        }
        pShellLink->lpVtbl->Release(pShellLink);
    }
    
    CoUninitialize();
    return result;
}

// 终止Steam进程
int killSteam() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    int count = 0;
    
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) return 0;
    
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            if (_stricmp(pe32.szExeFile, STEAM_PROCESS_NAME) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    if (TerminateProcess(hProcess, 0)) count++;
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }
    
    CloseHandle(hProcessSnap);
    return count;
}

// 增强版的查找Steam可执行文件路径
int findSteamPath(char* steamPath, size_t steamPathSize) {
    char executablePath[MAX_PATH] = {0};
    char tempPath[MAX_PATH] = {0};
    char shortcutPath[MAX_PATH] = {0};
    
    // 优化查找顺序，先检查最可能的位置
    
    // 1. 缓存是最快的查找方式
    if (getSteamPathFromCache(steamPath, steamPathSize) == 0) {
        return 0;
    }
    
    // 2. 从注册表获取Steam路径 (最常见的位置)
    if (getSteamPathFromRegistry(tempPath, MAX_PATH) == 0) {
        if (SUCCEEDED(StringCchPrintfA(executablePath, MAX_PATH, "%s\\%s", tempPath, STEAM_PROCESS_NAME))) {
            if (GetFileAttributesA(executablePath) != INVALID_FILE_ATTRIBUTES) {
                StringCchCopyA(steamPath, steamPathSize, executablePath);
                saveSteamPathToCache(steamPath);
                return 0;
            }
        }
    }
    
    // 3. 从当前运行的Steam进程获取路径
    if (getSteamPathFromProcess(steamPath, steamPathSize) == 0) {
        saveSteamPathToCache(steamPath);
        return 0;
    }
    
    // 以下查找方式需要更多资源，按可能性顺序排列
    
    // 4. 标准安装位置 (Program Files和x86)
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, 0, tempPath))) {
        StringCchPrintfA(shortcutPath, MAX_PATH, "%s\\Steam\\%s", tempPath, STEAM_PROCESS_NAME);
        if (GetFileAttributesA(shortcutPath) != INVALID_FILE_ATTRIBUTES) {
            StringCchCopyA(steamPath, steamPathSize, shortcutPath);
            saveSteamPathToCache(steamPath);
            return 0;
        }
    }
    
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILESX86, NULL, 0, tempPath))) {
        StringCchPrintfA(shortcutPath, MAX_PATH, "%s\\Steam\\%s", tempPath, STEAM_PROCESS_NAME);
        if (GetFileAttributesA(shortcutPath) != INVALID_FILE_ATTRIBUTES) {
            StringCchCopyA(steamPath, steamPathSize, shortcutPath);
            saveSteamPathToCache(steamPath);
            return 0;
        }
    }
    
    // 5. 从桌面快捷方式查找
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, tempPath))) {
        StringCchPrintfA(shortcutPath, MAX_PATH, "%s\\%s", tempPath, STEAM_SHORTCUT_NAME);
        if (GetFileAttributesA(shortcutPath) != INVALID_FILE_ATTRIBUTES &&
            getSteamPathFromShortcut(shortcutPath, steamPath, steamPathSize) == 0) {
            saveSteamPathToCache(steamPath);
            return 0;
        }
    }
    
    // 6. 从用户程序菜单查找
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROGRAMS, NULL, 0, tempPath))) {
        StringCchPrintfA(shortcutPath, MAX_PATH, "%s\\Steam\\%s", tempPath, STEAM_SHORTCUT_NAME);
        if (GetFileAttributesA(shortcutPath) != INVALID_FILE_ATTRIBUTES &&
            getSteamPathFromShortcut(shortcutPath, steamPath, steamPathSize) == 0) {
            saveSteamPathToCache(steamPath);
            return 0;
        }
    }
    
    // 7. 最后尝试搜索所有常见驱动器(代价最高)
    if (findSteamInCommonDrives(steamPath, steamPathSize) == 0) {
        saveSteamPathToCache(steamPath);
        return 0;
    }
    
    return -1;
}

// 启动Steam
int startSteam() {
    char steamPath[MAX_PATH] = {0};
    
    if (findSteamPath(steamPath, MAX_PATH) == 0) {
        HINSTANCE hResult = ShellExecuteA(NULL, "open", steamPath, NULL, NULL, SW_SHOWNORMAL);
        return ((INT_PTR)hResult > 32) ? 0 : -1;
    }
    
    return -1;
}

// 主函数改为WINAPI格式，隐藏控制台窗口
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 终止Steam进程
    int killed = killSteam();
    
    // 如果成功终止了Steam进程，等待一小段时间再启动
    if (killed > 0) {
        Sleep(500); // 减少等待时间为0.秒，提高效率
    }
    
    // 启动Steam
    startSteam();
    
    // 加载应用程序图标
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
    if (hIcon) {
        // 创建一个隐藏的窗口来设置图标
        WNDCLASSA wc = {0};
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "RestartSteamClass";
        wc.hIcon = hIcon;
        RegisterClassA(&wc);

        HWND hwnd = CreateWindowA("RestartSteamClass", "重启Steam", 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);
        if (hwnd) {
            DestroyWindow(hwnd);
        }
        UnregisterClassA("RestartSteamClass", GetModuleHandle(NULL));
    }

    return 0;
}
