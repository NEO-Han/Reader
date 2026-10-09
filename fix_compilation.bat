@echo off
REM ============================================
REM Reader 编译问题快速修复脚本
REM ============================================
setlocal enabledelayedexpansion

echo.
echo ========== Reader LNK1104 错误修复脚本 ==========
echo.

REM 步骤 1: 关闭所有 Reader 进程
echo [步骤 1] 关闭正在运行的 Reader.exe...
taskkill /IM Reader.exe /F >nul 2>&1
taskkill /IM Reader.exe /F >nul 2>&1
if %errorlevel% equ 0 (
    echo ✓ Reader 进程已关闭
) else (
    echo ✓ 没有运行中的 Reader 进程
)

REM 步骤 2: 等待文件锁定被释放
echo [步骤 2] 等待系统释放文件锁...
timeout /t 2 /nobreak

REM 步骤 3: 设置项目目录
set PROJECT_PATH=C:\Users\hanzhi\Desktop\gh-repo-clone-binbyu-Reader-master
cd /d "%PROJECT_PATH%"

if not exist "%PROJECT_PATH%\Reader.sln" (
    echo ✗ 错误：找不到 Reader.sln 文件
    echo 请确保当前目录是项目根目录
    pause
    exit /b 1
)

echo ✓ 项目路径：%PROJECT_PATH%

REM 步骤 4: 清理 Release 目录
echo.
echo [步骤 3] 清理编译缓存...
if exist Release (
    rmdir /s /q Release >nul 2>&1
    echo ✓ Release 目录已删除
) else (
    echo ✓ Release 目录不存在
)

if exist Debug (
    rmdir /s /q Debug >nul 2>&1
    echo ✓ Debug 目录已删除
) else (
    echo ✓ Debug 目录不存在
)

if exist .vs (
    rmdir /s /q .vs >nul 2>&1
    echo ✓ VS 临时文件已删除
) else (
    echo ✓ VS 临时文件不存在
)

REM 步骤 5: 检查 MSBuild 可用性
echo.
echo [步骤 4] 检查 MSBuild...
where msbuild >nul 2>&1
if %errorlevel% neq 0 (
    echo ✗ 错误：找不到 MSBuild
    echo 请确保已安装 Visual Studio
    pause
    exit /b 1
)
echo ✓ MSBuild 已找到

REM 步骤 6: 清理解决方案
echo.
echo [步骤 5] 清理解决方案...
msbuild Reader.sln /t:Clean /p:Configuration=Release /p:Platform=Win32 >nul 2>&1
if %errorlevel% equ 0 (
    echo ✓ 清理成功
) else (
    echo ⚠ 清理过程中可能出错，继续重新编译...
)

REM 步骤 7: 重新编译
echo.
echo [步骤 6] 重新编译解决方案...
echo.
msbuild Reader.sln /t:Rebuild /p:Configuration=Release /p:Platform=Win32

if %errorlevel% equ 0 (
    echo.
    echo ========== ✓ 编译成功！ ==========
    echo.
    if exist Release\Reader.exe (
        echo ✓ Reader.exe 已生成
        for %%A in (Release\Reader.exe) do (
            echo ✓ 文件大小: %%~zA 字节
            echo ✓ 生成时间: %%~TA
        )
    )
) else (
    echo.
    echo ========== ✗ 编译失败 ==========
    echo.
    echo 可能原因：
    echo 1. 代码中有语法错误（查看输出窗口获取详细信息）
    echo 2. 缺少依赖库或包含文件
    echo 3. 项目配置问题
    echo.
    echo 请检查 Visual Studio 的"错误列表"窗口
    echo.
)

echo.
pause
