# Steam 重启工具 / Steam Restart Tool

![Steam重启工具图标](128.ico)

这是一个简单而强大的 Steam 客户端重启工具，能够智能定位并重启您的 Steam 客户端，即使桌面上没有 Steam 图标也能正常工作。
This is a simple yet powerful Steam client restart tool that can intelligently locate and restart your Steam client, even if there's no Steam icon on your desktop.

## 使用方法 / Usage

1. 双击 `restart_steam.exe` 即可
   Simply double-click `restart_steam.exe`
2. 程序会自动找到并重启 Steam，无需任何用户界面
   The program will automatically find and restart Steam without any user interface
3. 后续使用会从缓存中快速找到 Steam 位置
   Subsequent use will quickly find the Steam location from the cache


## 功能特点 / Features

- 静默运行，无控制台窗口，不干扰用户体验
  Silent operation, no console window, does not interfere with user experience
- 智能查找 Steam 安装路径（支持8种查找方法）
  Intelligent Steam installation path finding (supports 8 different methods)
- 自动终止所有 Steam 相关进程
  Automatically terminates all Steam-related processes
- 安全快速重启 Steam 客户端
  Safely and quickly restarts the Steam client
- 路径缓存机制，提高后续启动速度
  Path caching mechanism to improve subsequent startup speed
- 精美的 Steam 图标 (128.ico)
  Beautiful Steam icon (128.ico)
- 高度优化的代码，最小的资源占用
  Highly optimized code with minimal resource consumption

## 工作原理 / How It Works

该工具使用多种方法来查找 Steam 客户端：
The tool uses multiple methods to locate the Steam client:

1. 从本地缓存文件读取上次找到的路径
   Reads the last found path from a local cache file
2. 从 Windows 注册表查询 Steam 安装路径
   Queries the Steam installation path from the Windows registry
3. 从当前运行的 Steam 进程获取路径
   Gets the path from the currently running Steam process
4. 查找标准安装路径（Program Files 和 x86）
   Searches standard installation paths (Program Files and x86)
5. 查找桌面上的 Steam 快捷方式
   Locates Steam shortcuts on the desktop
6. 查找开始菜单的 Steam 快捷方式
   Finds Steam shortcuts in the Start menu
7. 扫描所有驱动器中的常见 Steam 安装位置
   Scans common Steam installation locations on all drives

一旦找到 Steam，程序会：
Once Steam is found, the program will:
- 结束所有运行中的 Steam 进程
  Terminate all running Steam processes
- 等待 0.5 秒以确保所有进程都已终止
  Wait 0.5 seconds to ensure all processes have been terminated
- 启动新的 Steam 实例
  Launch a new Steam instance

## 编译方法 / Compilation Methods

### 使用提供的编译脚本 / Using the Provided Compilation Script

最简单的方法是使用包含的批处理文件：
The simplest method is to use the included batch file:

1. 确保已经安装 w64devkit (https://github.com/skeeto/w64devkit)
   Make sure w64devkit is installed (https://github.com/skeeto/w64devkit)
2. 运行 `build.bat`
   Run `build.bat`

### 手动编译 / Manual Compilation

如果您想手动编译，请执行以下步骤：
If you want to compile manually, follow these steps:

```batch
:: 编译资源文件 / Compile resource file
windres -i restart_steam.rc -o restart_steam_res.o

:: 编译源代码并链接资源 / Compile source code and link resources
gcc -o restart_steam.exe restart_steam.c restart_steam_res.o -mwindows -luser32 -lshell32 -lole32 -lcomctl32 -luuid
```

## 技术特点 / Technical Features

- Windows GUI 应用程序设计，无控制台窗口
  Windows GUI application design, no console window
- 使用 Windows API 和 COM 接口查找和操作 Steam
  Uses Windows API and COM interfaces to find and operate Steam
- 高效的多线程 COM 初始化
  Efficient multi-threaded COM initialization
- 内存安全设计，防止缓冲区溢出
  Memory-safe design to prevent buffer overflows
- 全面的错误处理
  Comprehensive error handling
- 优化的路径查找算法，按可能性排序
  Optimized path finding algorithms, ordered by probability
- 极低的资源占用和处理时间
  Extremely low resource consumption and processing time

## 注意事项 / Notes

- 程序需要一定的权限才能终止 Steam 进程
  The program needs certain permissions to terminate Steam processes
- 如果 Steam 安装在非标准位置，首次运行可能需要几秒钟来定位它
  If Steam is installed in a non-standard location, the first run may take a few seconds to locate it
- 生成的缓存文件 (steam_path.cache) 存储在程序同一目录下
  The generated cache file (steam_path.cache) is stored in the same directory as the program
- 该工具仅适用于 Windows 操作系统
  This tool is only applicable to Windows operating systems

## 贡献 / Contributions

欢迎提交问题报告和改进建议！
Issue reports and improvement suggestions are welcome!

