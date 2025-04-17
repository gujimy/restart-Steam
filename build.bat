@echo off
chcp 65001 > nul
echo 编译带有资源的restart_steam程序...

rem 设置环境变量
set W64_PATH=C:\w64devkit
set PATH=%W64_PATH%\bin;%PATH%

rem 编译资源文件
echo 编译资源文件...
windres -i restart_steam.rc -o restart_steam_res.o

rem 编译C源文件并链接资源
echo 编译源代码...
gcc -o restart_steam.exe restart_steam.c restart_steam_res.o -mwindows -luser32 -lshell32 -lole32 -lcomctl32 -luuid

if %ERRORLEVEL% equ 0 (
    echo 编译成功！生成了restart_steam.exe
    exit 0
) else (
    echo 编译失败！错误代码: %ERRORLEVEL%
    exit 1
) 