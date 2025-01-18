# 一键重启 Steam 工具

这是一个使用 C 语言编写的工具，用于快速重启 Steam。

## 使用说明

1. 确保桌面上有 Steam 的快捷方式 (`Steam.lnk`)。
2. 运行 `restart_steam.exe` 程序。

## 功能

- 自动终止正在运行的 Steam 进程。
- 通过桌面快捷方式重新启动 Steam。

## 注意事项

- 请确保桌面上存在名为 `Steam.lnk` 的快捷方式。
- 该工具仅适用于 Windows 操作系统。

## 编译

使用以下命令编译该项目：

```sh
gcc -o restart_steam.exe restart_steam.c -lole32 -loleaut32 -luuid -lshlwapi -luser32
```
