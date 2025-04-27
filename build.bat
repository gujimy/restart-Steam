@echo off
chcp 65001 > nul
echo 编译带有资源的restart_steam程序...

rem 设置环境变量
set W64_PATH=C:\w64devkit
set PATH=%W64_PATH%\bin;%PATH%

rem 创建dist目录（如果不存在）
if not exist dist mkdir dist

rem 清理旧文件
echo 清理旧文件...
if exist restart_steam_res.o del /f /q restart_steam_res.o
if exist restart_steam.exe del /f /q restart_steam.exe
if exist dist\restart_steam.exe del /f /q dist\restart_steam.exe

rem 清理其他可能的临时文件
if exist *.o del /f /q *.o
if exist *.res del /f /q *.res

rem 编译资源文件
echo 编译资源文件...
windres -i restart_steam.rc -o restart_steam_res.o

if %ERRORLEVEL% neq 0 (
    echo 资源编译失败！错误代码: %ERRORLEVEL%
    exit 1
)

rem 编译C源文件并链接资源
echo 编译源代码...
gcc -o dist\restart_steam.exe restart_steam.c restart_steam_res.o -mwindows -luser32 -lshell32 -lole32 -lcomctl32 -luuid

if %ERRORLEVEL% equ 0 (
    echo 编译成功！生成了dist\restart_steam.exe

    rem 复制图标文件到dist目录
    copy "Steam restart.ico" dist\ > nul

    rem 清理中间文件
    echo 清理中间文件...
    if exist restart_steam_res.o del /f /q restart_steam_res.o

    echo 编译完成！程序已保存到dist文件夹
    exit 0
) else (
    echo 编译失败！错误代码: %ERRORLEVEL%
    exit 1
)