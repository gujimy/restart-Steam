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
    
    // 先尝试64位路径
    result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, STEAM_REG_PATH, 0, KEY_READ, &hKey);
    if (result != ERROR_SUCCESS) {
        // 再尝试32位路径
        result = RegOpenKeyExA(HKEY_LOCAL_MACHINE, STEAM_REG_PATH_WOW64, 0, KEY_READ, &hKey);
        if (result != ERROR_SUCCESS) {
            // 尝试当前用户的注册表
            result = RegOpenKeyExA(HKEY_CURRENT_USER, STEAM_REG_PATH, 0, KEY_READ, &hKey);
            if (result != ERROR_SUCCESS) {
                // 最后尝试当前用户的32位路径
                result = RegOpenKeyExA(HKEY_CURRENT_USER, STEAM_REG_PATH_WOW64, 0, KEY_READ, &hKey);
                if (result != ERROR_SUCCESS) {
                    return -1;
                }
            }
        }
    }
    
    result = RegQueryValueExA(hKey, STEAM_REG_KEY, NULL, &dataType, (LPBYTE)steamPath, &dataSize);
    RegCloseKey(hKey);
    
    if (result != ERROR_SUCCESS) {
        return -1;
    }
    
    // 确保路径以null结尾
    if (dataSize >= steamPathSize) {
        steamPath[steamPathSize - 1] = '\0';
    } else if (dataSize > 0 && steamPath[dataSize - 1] != '\0') {
        steamPath[dataSize] = '\0';
    }
    
    return 0;
}

// 获取当前运行的Steam进程路径
int getSteamPathFromProcess(char* steamPath, size_t steamPathSize) {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    int found = 0;
    
    if (!steamPath || steamPathSize == 0) {
        return -1;
    }
    
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return -1;
    }
    
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            if (_stricmp(pe32.szExeFile, STEAM_PROCESS_NAME) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pe32.th32ProcessID);
                if (hProcess) {
                    if (GetModuleFileNameExA(hProcess, NULL, steamPath, (DWORD)steamPathSize)) {
                        found = 1;
                    }
                    CloseHandle(hProcess);
                }
                break;
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }
    
    CloseHandle(hProcessSnap);
    return found ? 0 : -1;
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
    if (!file) {
        return -1;
    }
    
    if (fgets(steamPath, (int)steamPathSize, file)) {
        fclose(file);
        // 去除可能的换行符
        size_t len = strlen(steamPath);
        if (len > 0 && (steamPath[len-1] == '\n' || steamPath[len-1] == '\r')) {
            steamPath[len-1] = '\0';
        }
        
        // 检查路径是否有效
        if (GetFileAttributesA(steamPath) != INVALID_FILE_ATTRIBUTES) {
            return 0;
        }
    }
    
    fclose(file);
    return -1;
}

// 查找常见驱动器中的Steam
int findSteamInCommonDrives(char* steamPath, size_t steamPathSize) {
    char driveLetter;
    char testPath[MAX_PATH] = {0};
    
    // 检查所有可能的驱动器盘符
    for (driveLetter = 'C'; driveLetter <= 'Z'; driveLetter++) {
        // 检查 X:\Steam\steam.exe
        sprintf(testPath, "%c:\\Steam\\%s", driveLetter, STEAM_PROCESS_NAME);
        if (GetFileAttributesA(testPath) != INVALID_FILE_ATTRIBUTES) {
            if (SUCCEEDED(StringCchCopyA(steamPath, steamPathSize, testPath))) {
                return 0;
            }
        }
        
        // 检查 X:\Games\Steam\steam.exe
        sprintf(testPath, "%c:\\Games\\Steam\\%s", driveLetter, STEAM_PROCESS_NAME);
        if (GetFileAttributesA(testPath) != INVALID_FILE_ATTRIBUTES) {
            if (SUCCEEDED(StringCchCopyA(steamPath, steamPathSize, testPath))) {
                return 0;
            }
        }
        
        // 检查 X:\SteamLibrary\steam.exe
        sprintf(testPath, "%c:\\SteamLibrary\\%s", driveLetter, STEAM_PROCESS_NAME);
        if (GetFileAttributesA(testPath) != INVALID_FILE_ATTRIBUTES) {
            if (SUCCEEDED(StringCchCopyA(steamPath, steamPathSize, testPath))) {
                return 0;
            }
        }
    }
    
    return -1;
}

// 获取 Steam 快捷方式的目标路径
int getSteamPathFromShortcut(const char* shortcutPath, char* targetPath, size_t targetPathSize) {
    HRESULT hr;
    IShellLink* pShellLink = NULL;
    IPersistFile* pPersistFile = NULL;
    char wTargetPath[MAX_PATH] = {0};
    WCHAR wShortcutPath[MAX_PATH] = {0};
    int result = -1;
    
    if (!shortcutPath || !targetPath || targetPathSize == 0) {
        printf("无效的参数\n");
        return -1;
    }
    
    // 初始化 COM
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        printf("COM 初始化失败: 0x%08X\n", (unsigned int)hr);
        return -1;
    }
    
    // 创建 ShellLink 实例
    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IShellLinkA, (void**)&pShellLink);
    if (SUCCEEDED(hr)) {
        hr = pShellLink->lpVtbl->QueryInterface(pShellLink, &IID_IPersistFile,
                                               (void**)&pPersistFile);
        if (SUCCEEDED(hr)) {
            MultiByteToWideChar(CP_ACP, 0, shortcutPath, -1, wShortcutPath, MAX_PATH);
            hr = pPersistFile->lpVtbl->Load(pPersistFile, wShortcutPath, STGM_READ);
            if (SUCCEEDED(hr)) {
                hr = pShellLink->lpVtbl->GetPath(pShellLink, wTargetPath,
                                                MAX_PATH, NULL, SLGP_UNCPRIORITY);
                if (SUCCEEDED(hr) && wTargetPath[0] != '\0') {
                    // 安全地复制路径
                    hr = StringCchCopyA(targetPath, targetPathSize, wTargetPath);
                    if (SUCCEEDED(hr)) {
                        result = 0;
                    } else {
                        printf("复制目标路径失败: 0x%08X\n", (unsigned int)hr);
                    }
                } else {
                    printf("无法获取快捷方式目标路径\n");
                }
            } else {
                printf("无法加载快捷方式: %s (错误码: 0x%08X)\n", shortcutPath, (unsigned int)hr);
            }
            pPersistFile->lpVtbl->Release(pPersistFile);
        }
        pShellLink->lpVtbl->Release(pShellLink);
    } else {
        printf("创建 ShellLink 对象失败: 0x%08X\n", (unsigned int)hr);
    }
    
    CoUninitialize();
    return result;
}

// 终止 Steam 进程
int killSteam() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    int count = 0;
    
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        printf("创建进程快照失败: %lu\n", GetLastError());
        return 0;
    }
    
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            if (_stricmp(pe32.szExeFile, STEAM_PROCESS_NAME) == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE,
                                           pe32.th32ProcessID);
                if (hProcess) {
                    if (TerminateProcess(hProcess, 0)) {
                        count++;
                        printf("成功终止 Steam 进程 (PID: %lu)\n", pe32.th32ProcessID);
                    } else {
                        printf("终止 Steam 进程失败 (PID: %lu, 错误码: %lu)\n", 
                               pe32.th32ProcessID, GetLastError());
                    }
                    CloseHandle(hProcess);
                } else {
                    printf("无法访问 Steam 进程 (PID: %lu, 错误码: %lu)\n", 
                           pe32.th32ProcessID, GetLastError());
                }
            }
        } while (Process32Next(hProcessSnap, &pe32));
    } else {
        printf("获取第一个进程信息失败: %lu\n", GetLastError());
    }
    
    CloseHandle(hProcessSnap);
    return count;
}

// 增强版的查找Steam可执行文件路径
int findSteamPath(char* steamPath, size_t steamPathSize) {
    char executablePath[MAX_PATH] = {0};
    char tempPath[MAX_PATH] = {0};
    
    // 1. 先尝试从缓存读取
    if (getSteamPathFromCache(steamPath, steamPathSize) == 0) {
        printf("从缓存找到 Steam 路径: %s\n", steamPath);
        return 0;
    }
    
    // 2. 尝试从注册表获取Steam路径
    if (getSteamPathFromRegistry(tempPath, MAX_PATH) == 0) {
        if (SUCCEEDED(StringCchPrintfA(executablePath, MAX_PATH, "%s\\%s", tempPath, STEAM_PROCESS_NAME))) {
            if (GetFileAttributesA(executablePath) != INVALID_FILE_ATTRIBUTES) {
                if (SUCCEEDED(StringCchCopyA(steamPath, steamPathSize, executablePath))) {
                    printf("从注册表找到 Steam 路径: %s\n", steamPath);
                    saveSteamPathToCache(steamPath);
                    return 0;
                }
            }
        }
    }
    
    // 3. 从当前运行的Steam进程获取路径
    if (getSteamPathFromProcess(steamPath, steamPathSize) == 0) {
        printf("从运行进程找到 Steam 路径: %s\n", steamPath);
        saveSteamPathToCache(steamPath);
        return 0;
    }
    
    // 4. 从桌面快捷方式查找
    char desktopPath[MAX_PATH] = {0};
    char shortcutPath[MAX_PATH] = {0};
    
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath))) {
        if (SUCCEEDED(StringCchPrintfA(shortcutPath, MAX_PATH, "%s\\%s", desktopPath, STEAM_SHORTCUT_NAME))) {
            if (GetFileAttributesA(shortcutPath) != INVALID_FILE_ATTRIBUTES) {
                if (getSteamPathFromShortcut(shortcutPath, steamPath, steamPathSize) == 0) {
                    printf("从桌面快捷方式找到 Steam 路径: %s\n", steamPath);
                    saveSteamPathToCache(steamPath);
                    return 0;
                }
            }
        }
    }
    
    // 5. 尝试从用户程序菜单找快捷方式
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROGRAMS, NULL, 0, tempPath))) {
        if (SUCCEEDED(StringCchPrintfA(shortcutPath, MAX_PATH, "%s\\Steam\\%s", tempPath, STEAM_SHORTCUT_NAME))) {
            if (GetFileAttributesA(shortcutPath) != INVALID_FILE_ATTRIBUTES) {
                if (getSteamPathFromShortcut(shortcutPath, steamPath, steamPathSize) == 0) {
                    printf("从程序菜单找到 Steam 路径: %s\n", steamPath);
                    saveSteamPathToCache(steamPath);
                    return 0;
                }
            }
        }
    }
    
    // 6. 尝试标准安装路径 (Program Files)
    char programFilesPath[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, 0, programFilesPath))) {
        if (SUCCEEDED(StringCchPrintfA(shortcutPath, MAX_PATH, "%s\\Steam\\%s", programFilesPath, STEAM_PROCESS_NAME))) {
            if (GetFileAttributesA(shortcutPath) != INVALID_FILE_ATTRIBUTES) {
                if (SUCCEEDED(StringCchCopyA(steamPath, steamPathSize, shortcutPath))) {
                    printf("从标准安装路径找到 Steam: %s\n", steamPath);
                    saveSteamPathToCache(steamPath);
                    return 0;
                }
            }
        }
    }
    
    // 7. 尝试 Program Files (x86)
    char programFilesx86Path[MAX_PATH] = {0};
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILESX86, NULL, 0, programFilesx86Path))) {
        if (SUCCEEDED(StringCchPrintfA(shortcutPath, MAX_PATH, "%s\\Steam\\%s", programFilesx86Path, STEAM_PROCESS_NAME))) {
            if (GetFileAttributesA(shortcutPath) != INVALID_FILE_ATTRIBUTES) {
                if (SUCCEEDED(StringCchCopyA(steamPath, steamPathSize, shortcutPath))) {
                    printf("从 Program Files (x86) 找到 Steam: %s\n", steamPath);
                    saveSteamPathToCache(steamPath);
                    return 0;
                }
            }
        }
    }
    
    // 8. 尝试搜索所有常见驱动器
    if (findSteamInCommonDrives(steamPath, steamPathSize) == 0) {
        printf("从常见位置找到 Steam: %s\n", steamPath);
        saveSteamPathToCache(steamPath);
        return 0;
    }
    
    printf("未找到 Steam 可执行文件路径\n");
    return -1;
}

// 启动 Steam
int startSteam() {
    char steamPath[MAX_PATH] = {0};
    
    if (findSteamPath(steamPath, MAX_PATH) == 0) {
        HINSTANCE hResult = ShellExecuteA(NULL, "open", steamPath, NULL, NULL, SW_SHOWNORMAL);
        if ((INT_PTR)hResult > 32) {
            printf("Steam 启动成功\n");
            return 0;
        } else {
            printf("启动 Steam 失败，错误码: %d\n", (int)(INT_PTR)hResult);
            return -1;
        }
    }
    
    return -1;
}

// 设置控制台输出编码为UTF-8
void setConsoleOutputToUTF8() {
    // 设置控制台代码页为UTF-8
    SetConsoleOutputCP(65001);
    // 设置C运行时的本地化为中文
    setlocale(LC_ALL, "zh_CN.UTF-8");
}

int main() {
    // 设置控制台输出为UTF-8，解决中文乱码问题
    setConsoleOutputToUTF8();
    
    printf("正在重启 Steam...\n");
    
    // 终止 Steam 进程
    int killed = killSteam();
    if (killed > 0) {
        printf("成功关闭 %d 个 Steam 进程\n", killed);
        printf("等待 2 秒...\n");
        Sleep(2000); // 等待2秒
    } else {
        printf("未找到正在运行的 Steam 进程\n");
    }
    
    // 启动 Steam
    if (startSteam() != 0) {
        printf("无法启动 Steam，请手动启动\n");
        return 1;
    }

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
