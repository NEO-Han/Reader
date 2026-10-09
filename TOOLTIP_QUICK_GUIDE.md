# 工具提示修改 - 快速参考

## 位置
```
Reader/Reader.rc          ← 英文版本
Reader/Reader_zh-cn.rc    ← 中文版本
```

## 结构
```rc
STRINGTABLE
BEGIN
    IDS_AUTO_PAGE       "自动翻页"
    IDS_HIDE_SHOW_WINDOW "隐藏/显示窗口"
    // ... 更多提示
END
```

## 修改步骤

### 方法 1：Visual Studio 资源编辑器（推荐）
```
1. 打开 Reader.rc 或 Reader_zh-cn.rc
2. 双击打开资源编辑器
3. 找到要修改的 STRINGTABLE
4. 修改对应 ID 的文本
5. Ctrl+S 保存
6. Ctrl+Shift+B 重新生成
```

### 方法 2：直接编辑文本
```
1. 用记事本或代码编辑器打开 .rc 文件
2. 找到要修改的 ID
3. 修改引号中的文本
4. 保存
5. 重新编译
```

## 关键ID 列表

| ID | 说明 |
|----|------|
| IDS_AUTO_PAGE | 自动翻页 |
| IDS_HIDE_SHOW_WINDOW | 隐藏/显示窗口 |
| IDS_HIDE_SHOW_BORDER | 隐藏/显示边框 |
| IDS_FULLSCREEN | 全屏 |
| IDS_TOPMOST | 置顶 |
| IDS_OPEN_FILE | 打开文件 |
| IDS_ADD_BOOKMARK | 添加书签 |
| IDS_PAGE_UP | 上一页 |
| IDS_PAGE_DOWN | 下一页 |

## 注意事项

⚠️ **同时修改两个文件**：英文和中文版本都要改

⚠️ **ID 必须相同**：两个文件中的 ID 必须对应

⚠️ **格式要对**：
```rc
IDS_XXXX    "text"   ✓ 正确
IDS_XXXX="text"      ✗ 错误
```

## 特殊字符

```
\r\n     → 换行
\t       → 制表符
\"       → 双引号
```

## 验证修改

编译后，鼠标悬停在对应的菜单项或按钮上，应该看到修改后的文本。

