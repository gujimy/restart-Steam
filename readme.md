# 一键重启 Steam 工具 / One-click Steam Restart Tool

这是一个使用 C 语言编写的工具，用于快速重启 Steam。  
This is a tool written in C for quickly restarting Steam.

## 使用说明 / Usage Instructions

1. 确保桌面上有 Steam 的快捷方式 (`Steam.lnk`)。  
   Ensure there is a Steam shortcut (`Steam.lnk`) on the desktop.
2. 运行 `restart_steam.exe` 程序。  
   Run the `restart_steam.exe` program.

## 功能 / Features

- 自动终止正在运行的 Steam 进程。  
  Automatically terminates the running Steam process.
- 通过桌面快捷方式重新启动 Steam。  
  Restarts Steam via the desktop shortcut.

## 注意事项 / Notes

- 请确保桌面上存在名为 `Steam.lnk` 的快捷方式。  
  Ensure there is a shortcut named `Steam.lnk` on the desktop.
- 该工具仅适用于 Windows 操作系统。  
  This tool is only applicable to Windows operating systems.

## 编译 / Compilation

使用以下命令编译该项目：  
Compile the project using the following command:

```sh
gcc -o restart_steam.exe restart_steam.c -lole32 -loleaut32 -luuid -lshlwapi -luser32
