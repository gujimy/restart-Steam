// restart_steam.c
#include <shobjidl.h>
#include <shlguid.h>
#include <objbase.h>
#include <stdio.h>
#include <tlhelp32.h>
#include <shlobj.h>
#include "resource.h"  // 包含资源头文件

#define MAX_PATH_LENGTH 1024

// 获取 Steam 快捷方式的目标路径
int getSteamPathFromShortcut(const char* shortcutPath, char* targetPath) {
    HRESULT hr;
    IShellLink* pShellLink = NULL;
    IPersistFile* pPersistFile = NULL;
    char wTargetPath[MAX_PATH];
    WCHAR wShortcutPath[MAX_PATH];
    
    // 初始化 COM
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) {
        return -1;
    }
    
    // 创建 ShellLink 实例
    hr = CoCreateInstance(&CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER,
                         &IID_IShellLink, (void**)&pShellLink);
    if (SUCCEEDED(hr)) {
        hr = pShellLink->lpVtbl->QueryInterface(pShellLink, &IID_IPersistFile,
                                               (void**)&pPersistFile);
        if (SUCCEEDED(hr)) {
            MultiByteToWideChar(CP_ACP, 0, shortcutPath, -1, wShortcutPath, MAX_PATH);
            hr = pPersistFile->lpVtbl->Load(pPersistFile, wShortcutPath, STGM_READ);
            if (SUCCEEDED(hr)) {
                hr = pShellLink->lpVtbl->GetPath(pShellLink, wTargetPath,
                                                MAX_PATH, NULL, SLGP_UNCPRIORITY);
                if (SUCCEEDED(hr)) {
                    strcpy(targetPath, wTargetPath);
                }
            }
            pPersistFile->lpVtbl->Release(pPersistFile);
        }
        pShellLink->lpVtbl->Release(pShellLink);
    }
    
    CoUninitialize();
    return SUCCEEDED(hr) ? 0 : -1;
}

// 终止 Steam 进程
void killSteam() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return;
    }
    
    pe32.dwSize = sizeof(PROCESSENTRY32);
    if (Process32First(hProcessSnap, &pe32)) {
        do {
            if (_stricmp(pe32.szExeFile, "steam.exe") == 0) {
                HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE,
                                           pe32.th32ProcessID);
                if (hProcess) {
                    TerminateProcess(hProcess, 0);
                    CloseHandle(hProcess);
                }
            }
        } while (Process32Next(hProcessSnap, &pe32));
    }
    
    CloseHandle(hProcessSnap);
}

// 启动 Steam
void startSteam() {
    char desktopPath[MAX_PATH];
    char targetPath[MAX_PATH];
    
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_DESKTOP, NULL, 0, desktopPath))) {
        // 使用 strncat 来连接路径，避免缓冲区溢出
        if (strncat(desktopPath, "\\Steam.lnk", MAX_PATH - strlen(desktopPath) - 1) != NULL) {
            if (getSteamPathFromShortcut(desktopPath, targetPath) == 0) {
                HINSTANCE hResult = ShellExecuteA(NULL, "open", targetPath,
                                                NULL, NULL, SW_SHOWNORMAL);
                if ((INT_PTR)hResult > 32) {
                    printf("Steam started successfully.\n");
                } else {
                    printf("Failed to start Steam. Error code: %d\n", (int)(INT_PTR)hResult);
                }
            }
        } else {
            printf("Failed to concatenate path for Steam shortcut.\n");
        }
    }
}

int main() {
    killSteam();
    Sleep(2000); // 等待2秒
    startSteam();

    // 加载应用程序图标
    HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
    if (hIcon) {
        // 在控制台应用程序中设置图标并不直接支持
        // 可以考虑创建一个隐藏的窗口来加载图标
        WNDCLASS wc = {0};
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = "RestartSteamClass";
        wc.hIcon = hIcon;
        RegisterClass(&wc);

        HWND hwnd = CreateWindow("RestartSteamClass", "", 0, 0, 0, 0, 0, NULL, NULL, GetModuleHandle(NULL), NULL);

        if (hwnd) {
            DestroyWindow(hwnd);
        }
    }

    return 0;
}
