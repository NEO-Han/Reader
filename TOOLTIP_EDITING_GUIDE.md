# Reader 项目 - 工具提示（Tooltip）修改指南

## 📌 工具提示文本的存储位置

工具提示和所有UI文本都存储在**资源文件（.rc 文件）**中，而不是硬编码在代码里。

### 📂 关键文件位置

```
Reader 项目/
├── Reader.rc          ← 英文版本的资源文件
└── Reader_zh-cn.rc    ← 中文（简体）版本的资源文件
```

---

## 🔍 工具提示的结构

### 1️⃣ 资源文件中的定义

在 `.rc` 文件中，所有的文本都定义在 **STRINGTABLE** 块中：

```rc
STRINGTABLE
BEGIN
    IDS_HIDE_SHOW_WINDOW    "Hide/Show window"
    IDS_HIDE_SHOW_BORDER    "Hide/Show border"
    IDS_FULLSCREEN          "Full screen"
    IDS_TOPMOST             "Topmost"
    IDS_OPEN_FILE           "Open file"
    IDS_ADD_BOOKMARK        "Add book mark"
    IDS_AUTO_PAGE           "Auto page"
    IDS_TEXT_SEARCH         "Text search"
    IDS_PROGRESS_JUMP       "Progress jump"
    IDS_EDIT_MODE           "Edit mode"
    IDS_PAGE_UP             "Page up"
    IDS_PAGE_DOWN           "Page down"
    IDS_LINE_UP             "Line up"
    IDS_LINE_DOWN           "Line down"
    IDS_CHAPTER_UP          "Chapter up"
    IDS_CHAPTER_DOWN        "Chapter down"
END
```

### 2️⃣ 中文版本

在 `Reader_zh-cn.rc` 中对应的中文翻译：

```rc
STRINGTABLE
BEGIN
    IDS_HIDE_SHOW_WINDOW    "隐藏/显示窗口"
    IDS_HIDE_SHOW_BORDER    "隐藏/显示边框"
    IDS_FULLSCREEN          "全屏"
    IDS_TOPMOST             "置顶"
    IDS_OPEN_FILE           "打开文件"
    IDS_ADD_BOOKMARK        "添加书签"
    IDS_AUTO_PAGE           "自动翻页"
    IDS_TEXT_SEARCH         "文本搜索"
    IDS_PROGRESS_JUMP       "进度条跳转"
    IDS_EDIT_MODE           "编辑模式"
    IDS_PAGE_UP             "上一页"
    IDS_PAGE_DOWN           "下一页"
    IDS_LINE_UP             "向上滚动"
    IDS_LINE_DOWN           "向下滚动"
    IDS_CHAPTER_UP          "上一章"
    IDS_CHAPTER_DOWN        "下一章"
END
```

---

## 💻 代码中如何使用这些文本

### 在 Reader.cpp 中的使用

代码通过 `LoadString()` API 来加载这些文本：

```cpp
// 示例：在菜单中设置文本
TCHAR buf[256];
LoadString(hInst, IDS_HIDE_SHOW_WINDOW, buf, 256);
// 现在 buf 中包含了 "隐藏/显示窗口"（如果运行中文版本）
```

### 常见的使用模式

```cpp
// 1. 菜单项文本
LoadString(hInst, IDS_MENU_OPEN, buf, MAX_LOADSTRING);
SetMenuItemText(menu, IDM_OPEN, buf);

// 2. 工具栏提示
LoadString(hInst, IDS_HIDE_SHOW_WINDOW, buf, 256);
CreateToolTip(hwnd, buf);

// 3. 消息框标题和内容
LoadString(hInst, IDS_ERROR, title, 256);
MessageBox(hwnd, errorMsg, title, MB_OK);

// 4. 状态栏文本
LoadString(hInst, IDS_EDIT_MODE_TIPS, buf, 256);
SetStatusBarText(buf);
```

---

## 📝 修改工具提示的步骤

### ✅ 步骤 1：找到资源ID

首先在资源文件中找到要修改的文本的ID。例如：

```rc
IDS_AUTO_PAGE    "Auto page"  // 英文版本
```

对应的中文ID也是 `IDS_AUTO_PAGE`：

```rc
IDS_AUTO_PAGE    "自动翻页"   // 中文版本
```

### ✅ 步骤 2：修改文本

#### 方法 A：使用 Visual Studio 资源编辑器（推荐）

1. 在 Visual Studio 中打开项目
2. 在 Solution Explorer 中找到 `Reader.rc` 或 `Reader_zh-cn.rc`
3. 双击打开资源编辑器
4. 找到要修改的 STRINGTABLE
5. 找到对应的ID（如 `IDS_AUTO_PAGE`）
6. 修改右侧的值
7. 保存文件（Ctrl+S）

#### 方法 B：直接编辑 .rc 文件（需要小心）

```rc
// 修改前
IDS_AUTO_PAGE    "自动翻页"

// 修改后
IDS_AUTO_PAGE    "自动翻页（每5秒翻一页）"
```

### ✅ 步骤 3：重新编译

```bash
msbuild Reader.sln /p:Configuration=Release
```

---

## 📚 常见的工具提示ID列表

### 快捷键提示

| ID | 英文 | 中文 | 用途 |
|----|------|------|------|
| IDS_HIDE_SHOW_WINDOW | Hide/Show window | 隐藏/显示窗口 | 快捷键提示 |
| IDS_HIDE_SHOW_BORDER | Hide/Show border | 隐藏/显示边框 | 快捷键提示 |
| IDS_FULLSCREEN | Full screen | 全屏 | 快捷键提示 |
| IDS_TOPMOST | Topmost | 置顶 | 快捷键提示 |
| IDS_OPEN_FILE | Open file | 打开文件 | 快捷键提示 |
| IDS_AUTO_PAGE | Auto page | 自动翻页 | 快捷键提示 |
| IDS_EDIT_MODE | Edit mode | 编辑模式 | 快捷键提示 |

### 其他提示

| ID | 英文 | 中文 | 用途 |
|----|------|------|------|
| IDS_EDIT_MODE_TIPS | Currently in [Edit Mode]... | 当前在[编辑模式]... | 状态栏提示 |
| IDS_ADD_BOOKMARK_TIPS | Are you sure to add bookmarks? | 确定要添加书签吗？ | 确认对话框 |
| IDS_SAVE_TEXT_TIPS | The text is changed... | 文本有改动... | 保存提示 |

---

## 🎯 实际例子

### 例子 1：修改"自动翻页"提示

**当前中文版本：**
```rc
IDS_AUTO_PAGE    "自动翻页"
```

**修改后（添加更详细的说明）：**
```rc
IDS_AUTO_PAGE    "自动翻页（按设定的时间间隔自动翻页）"
```

### 例子 2：添加新的工具提示

如果要添加一个新的快捷键提示，比如"字体缩小"：

1. 在 Reader.h 中定义新的ID：
```cpp
#define IDS_FONT_ZOOM_OUT   1234  // 选择一个未使用的ID
```

2. 在 Reader.rc 中添加字符串：
```rc
IDS_FONT_ZOOM_OUT    "Zoom out"
```

3. 在 Reader_zh-cn.rc 中添加中文翻译：
```rc
IDS_FONT_ZOOM_OUT    "缩小字体"
```

4. 在代码中使用：
```cpp
TCHAR buf[256];
LoadString(hInst, IDS_FONT_ZOOM_OUT, buf, 256);
// 使用 buf 中的文本
```

---

## 🔧 文件编辑提示

### Reader.rc 的结构

```
Reader.rc
├── STRINGTABLE (ID 1-50) - 基础字符串
├── STRINGTABLE (ID 51-100) - 错误消息
├── STRINGTABLE (ID 101-150) - 对话框消息
├── STRINGTABLE (ID 151-200) - 菜单文本
├── STRINGTABLE (ID 201-250) - 快捷键提示
└── ...其他资源定义
```

### 编辑注意事项

⚠️ **重要提示：**

1. **必须修改两个文件**
   - 英文版：`Reader.rc`
   - 中文版：`Reader_zh-cn.rc`
   - 两个文件中的 ID 必须相同

2. **保持格式一致**
   ```rc
   // 正确的格式
   IDS_SOME_ID    "Your text here"
   
   // 避免这样做
   IDS_SOME_ID="Your text here"  // 不要用等号
   IDS_SOME_ID  "Your text"       // 中间要有空格或制表符
   ```

3. **特殊字符转义**
   ```rc
   // 换行符
   IDS_TIPS    "First line\r\nSecond line"
   
   // 制表符
   IDS_TABS    "Column1\tColumn2"
   
   // 引号
   IDS_QUOTE   "He said \"Hello\""
   ```

4. **避免重复的ID**
   - 确保同一个ID在同一文件中只出现一次
   - 不同的STRINGTABLE块中不要重复定义相同ID

---

## 🌍 多语言支持

Reader 项目支持多语言，通过 `.rc` 文件实现：

### 语言版本
- `Reader.rc` - 英文版本
- `Reader_zh-cn.rc` - 中文（简体）版本

### 如何实现多语言

在 Visual Studio 中：
```
项目 → 项目属性 → 配置属性 → 一般 → 字符集
```

编译时会自动根据系统语言加载对应的资源文件。

---

## 📞 常见问题

### Q: 修改了 .rc 文件后没有生效？
**A:** 
1. 确保保存了文件（Ctrl+S）
2. 执行"清理解决方案"→"重新生成解决方案"
3. 注意检查是否修改的是正确的文件（英文还是中文版本）

### Q: 怎样知道 ID 是多少？
**A:** 
1. 在 Resource.h 中查找对应的 #define
2. 或者在 Resource Editor 中的属性窗口查看

### Q: 可以添加新的工具提示吗？
**A:** 
1. 选择一个未使用的ID
2. 在 Resource.h 中 #define 这个ID
3. 在两个 .rc 文件中添加对应的字符串
4. 在代码中用 LoadString 加载使用

### Q: 文本太长，会不会导致显示问题？
**A:** 
1. Windows 会自动换行或截断
2. 建议保持文本简洁（50字以内）
3. 如果需要长文本，可以在代码中手动处理

---

## 🔗 相关的资源文件

```
C:\Users\hanzhi\Desktop\gh-repo-clone-binbyu-Reader-master\Reader\
├── Reader.rc              ← 主要资源文件（英文）
├── Reader_zh-cn.rc        ← 中文资源文件  
├── Reader.sln             ← Visual Studio 解决方案
└── resource.h             ← ID 定义文件
```

---

## ✅ 总结

**修改工具提示的流程：**

1. 在资源文件 `Reader.rc` 或 `Reader_zh-cn.rc` 中找到对应的 ID
2. 修改 ID 对应的文本字符串
3. 同时修改英文和中文版本（保持 ID 一致）
4. 保存文件
5. 在 Visual Studio 中重新生成解决方案
6. 重新编译和运行程序

**位置总结：**
- 英文提示：`Reader.rc` 中的 STRINGTABLE
- 中文提示：`Reader_zh-cn.rc` 中的 STRINGTABLE
- 代码使用：通过 `LoadString(hInst, ID, buf, size)` 加载

