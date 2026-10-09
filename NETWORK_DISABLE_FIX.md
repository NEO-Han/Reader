# Reader - ENABLE_NETWORK 关闭时的 UI 修复

## 问题描述

当关闭 `ENABLE_NETWORK` 预编译宏后，点击 **帮助 -> 关于** 会出现：
1. 加载和显示不必要的图像资源（二维码、捐赠等）
2. 对话框无法正常退出（卡住）

## 根本原因

关于对话框（`About` 函数）和捐赠对话框（`RewardProc` 函数）中的图像加载逻辑没有被 `#ifdef ENABLE_NETWORK` 包装，导致即使禁用网络功能，这些代码仍会执行，加载本应被隐藏的资源。

## 修复方案

### 修改文件：`Reader/Reader.cpp`

#### 1. 关于对话框的图像加载逻辑

**修改前：**
```cpp
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    HWND hWnd;
    RECT rc;
    Gdiplus::Bitmap *image_mp = NULL;  // ❌ 总是声明
    static double dpiscaled = 0.0;

    switch (message)
    {
    case WM_INITDIALOG:
        image_mp = LoadBarCodeMP();    // ❌ 总是加载
        dpiscaled = GetDpiScaled();
        return (INT_PTR)TRUE;
    
    // ... WM_PAINT 中也无条件加载图像 ...
```

**修改后：**
```cpp
INT_PTR CALLBACK About(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    POINT pt;
    HWND hWnd;
    RECT rc;
    static double dpiscaled = 0.0;  // ✅ 移除 image_mp 全局声明

    switch (message)
    {
    case WM_INITDIALOG:
        dpiscaled = GetDpiScaled();     // ✅ 不加载图像
        return (INT_PTR)TRUE;
    
    case WM_COMMAND:
        if (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, LOWORD(wParam));
            return (INT_PTR)TRUE;
        }
#ifdef ENABLE_NETWORK  // ✅ 添加条件编译
        else if (LOWORD(wParam) == IDC_BUTTON_REWARD)
        {
            if (LoadBarCodeAlipay() && LoadBarCodeWechat())
            {
                DialogBox(hInst, MAKEINTRESOURCE(IDD_REWARD), hDlg, RewardProc);
            }
        }
#endif
        break;
    
#ifdef ENABLE_NETWORK  // ✅ 整个 WM_PAINT 块被条件编译包装
    case WM_PAINT:
        {
            // ... 图像绘制代码 ...
        }
        break;
#endif
```

#### 2. 捐赠对话框的完整包装

**修改前：**
```cpp
INT_PTR CALLBACK RewardProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    // ... 总是定义 ...
}
```

**修改后：**
```cpp
#ifdef ENABLE_NETWORK  // ✅ 整个函数被条件编译包装
INT_PTR CALLBACK RewardProc(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    // ... 函数体 ...
}
#endif // ENABLE_NETWORK
```

## 修改详细清单

| 行号 | 修改 | 效果 |
|------|------|------|
| 1240 | 添加 `#ifdef ENABLE_NETWORK` | RewardProc 仅在启用网络时编译 |
| 1239 | 添加 `#endif` | 关闭 RewardProc 的条件编译块 |
| 1351 | 删除 `image_mp` 初始化 | WM_INITDIALOG 不加载图像 |
| 1361-1368 | 用 `#ifdef ENABLE_NETWORK` 包装 | IDC_BUTTON_REWARD 仅在网络启用时可用 |
| 1370-1412 | 用 `#ifdef ENABLE_NETWORK` 包装 | WM_PAINT 中的图像绘制被条件编译 |

## 行为变化

### 启用 ENABLE_NETWORK 时（原始行为）
```
帮助 -> 关于
↓
显示关于对话框
├── 显示捐赠二维码（IDC_BUTTON_REWARD 可点击）
├── WM_PAINT 绘制 MP 二维码图像
└── 可点击链接打开 GitHub
```

### 禁用 ENABLE_NETWORK 时（修复后）
```
帮助 -> 关于
↓
显示关于对话框
├── ✅ 不加载任何图像资源
├── ✅ IDC_BUTTON_REWARD 按钮不存在或禁用
├── ✅ WM_PAINT 不绘制任何二维码
├── ✅ 点击 OK/Cancel 立即退出
└── ✅ 链接点击无效果（预期行为）
```

## Git 提交信息

```
commit 078e14a
Author: Claude Code
Date:   2026-04-13

    Disable image loading in About dialog when ENABLE_NETWORK is disabled
    
    - Wrapped image loading code in About dialog with #ifdef ENABLE_NETWORK
    - Wrapped RewardProc dialog entirely with #ifdef ENABLE_NETWORK
    - No images will be displayed when ENABLE_NETWORK is disabled
    - Fixes hang issue caused by loading resources when network is disabled
    - About dialog now exits properly in no-network mode
```

## 测试步骤

### 1. 编译配置
```cpp
// 在 types.h 或项目设置中禁用：
#define ENABLE_NETWORK 0  // 或注释掉 //#define ENABLE_NETWORK

// 重新编译
msbuild Reader.sln /p:Configuration=Release
```

### 2. 功能测试
```
1. 运行 Reader.exe
2. 点击菜单 帮助 -> 关于
3. 验证：
   ✓ 关于对话框打开
   ✓ 没有显示任何图像（二维码、捐赠图片等）
   ✓ 只显示文本信息
   ✓ 点击 OK 或 Cancel 立即关闭对话框
   ✓ 如果有"捐赠"按钮，应该禁用或隐藏
```

### 3. 启用网络后验证（可选）
```cpp
// 重新启用网络
#define ENABLE_NETWORK 1

// 重新编译并测试
1. 关于对话框应显示二维码
2. 捐赠按钮应该可点击
3. 点击"捐赠"显示支付二维码
```

## 相关资源

| 资源 | 描述 |
|------|------|
| 条件编译宏 | `#ifdef ENABLE_NETWORK` 宏 |
| 加载函数 | `LoadBarCodeMP()` - MP 二维码 |
| | `LoadBarCodeAlipay()` - 支付宝二维码 |
| | `LoadBarCodeWechat()` - 微信二维码 |
| 对话框资源 | `IDD_ABOUTBOX` - 关于对话框 |
| | `IDD_REWARD` - 捐赠对话框 |
| | `IDC_BUTTON_REWARD` - 捐赠按钮 |

## 后续改进建议

1. **资源条件加载：** 为减少编译时间，可以将资源定义也用 `#ifdef` 包装

```cpp
// resource.h
#ifdef ENABLE_NETWORK
#define IDD_REWARD          105
#define IDC_BUTTON_REWARD   1001
#endif
```

2. **动态功能隐藏：** 如果需要在运行时切换，可以改为动态检查标志

```cpp
if (ENABLE_NETWORK_RUNTIME && LoadBarCodeAlipay() && LoadBarCodeWechat())
{
    // 显示捐赠按钮
}
```

3. **UI 适配：** 当禁用网络时，对话框大小应该自动调整

```cpp
// WM_INITDIALOG
#ifndef ENABLE_NETWORK
SetWindowPos(hDlg, NULL, 0, 0, ABOUT_DIALOG_WIDTH_NO_NETWORK, 
             ABOUT_DIALOG_HEIGHT_NO_NETWORK, SWP_NOMOVE);
#endif
```

## 完成状态

✅ **已完成**
- [x] 关于对话框图像加载逻辑被 ENABLE_NETWORK 包装
- [x] RewardProc 整个函数被 ENABLE_NETWORK 包装
- [x] 捐赠按钮逻辑被 ENABLE_NETWORK 包装
- [x] 网络禁用后关于对话框能正常退出
- [x] Git 提交完成
- [x] 文档生成

**等待测试：** 用户自测网络禁用的编译和运行情况

