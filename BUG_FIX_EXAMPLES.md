# Reader 项目 - Bug 修复建议代码

## Bug #1: EpubBook 内存泄漏 - 修复方案

### 原问题代码 (EpubBook.cpp:116-208)
```cpp
bool EpubBook::UnzipBook(void)
{
    // ... 代码 ...
    
end:
    if (uf)
    {
        unzCloseCurrentFile(uf);
        unzClose(uf);
    }
    if (buf)
        free(buf);

    if (err != UNZ_OK)
        FreeFilelist();  // ❌ 重复释放问题
    return err == UNZ_OK;
}

bool EpubBook::ParserBook(HWND hWnd)
{
    // ... 代码 ...
    
end:
    // free epub
    for (itor = epub.mainfest.begin(); itor != epub.mainfest.end(); itor++)
    {
        delete itor->second;
    }
    epub.mainfest.clear();
    for (it = epub.navmap.begin(); it != epub.navmap.end(); it++)
    {
        delete it->second;
    }
    epub.navmap.clear();
    epub.spine.clear();
    FreeFilelist();  // ❌ 可能重复调用
    // ...
}
```

### 修复方案
```cpp
bool EpubBook::ParserBook(HWND hWnd)
{
    bool ret = false;
    epub_t epub;
    mainfest_t::iterator itor;
    navmap_t::iterator it;
    bool filelist_freed = false;  // ✅ 添加标记

    // unzip epub file
    if (!UnzipBook())
        goto end;

    // parser epub file
    epub.ocf = "META-INF/container.xml";
    if (!ParserOcf(epub))
        goto end;

    if (!ParserOpf(epub))
        goto end;

    if (!ParserNcx(epub))
        goto end;

    // parser epub cover image
    ParserCover(epub);

    // Parser epub chapters & text
    if (!ParserChapters(epub))
        goto end;

    ret = true;

end:
    // free epub
    for (itor = epub.mainfest.begin(); itor != epub.mainfest.end(); itor++)
    {
        delete itor->second;
    }
    epub.mainfest.clear();
    for (it = epub.navmap.begin(); it != epub.navmap.end(); it++)
    {
        delete it->second;
    }
    epub.navmap.clear();
    epub.spine.clear();
    
    // ✅ 只释放一次
    if (!filelist_freed)
    {
        FreeFilelist();
        filelist_freed = true;
    }
    
    if (!ret)
    {
        if (m_Cover)
        {
            delete m_Cover;
            m_Cover = NULL;
        }
        CloseBook();
    }

    return ret;
}

// ✅ 修改 UnzipBook，移除末尾的 FreeFilelist 调用
bool EpubBook::UnzipBook(void)
{
    // ... 代码保持不变到 end: ...
    
end:
    if (uf)
    {
        unzCloseCurrentFile(uf);
        unzClose(uf);
    }
    if (buf)
        free(buf);

    // ❌ 删除这行：if (err != UNZ_OK) FreeFilelist();
    // ✅ 改为只返回结果，由 ParserBook 统一管理释放
    return err == UNZ_OK;
}
```

---

## Bug #2: TextBook 缓冲区越界 - 修复方案

### 原问题代码 (TextBook.cpp:307-349)
```cpp
bool TextBook::FormatText(wchar_t *text, int *len, bool flag)
{
    wchar_t *buf = NULL;
    int i, index = 0;
    if (!text || *len == 0)
        return false;

    buf = (wchar_t *)malloc(((*len) + 1) * sizeof(wchar_t));
    for (i = 0; i < (*len); i++)
    {
        if (flag && IsBlanks(text[i]))
            continue;
        flag = false;
        
        if (i < ((*len) - 1) && text[i] == 0x00)
            continue; 

        if (text[i] == 0x0D)
            continue;

        // Remove extra spaces
        if (0x20 == text[i] || 0xA0 == text[i])
        {
            if (index > 3 && buf[index - 1] == text[i] && buf[index - 2] == text[i] 
                && buf[index - 3] == text[i] && buf[index - 4] == text[i])
                continue;  // ❌ 未检查 index >= 4
        }
        else if (IsBlanks(text[i]))
        {
            if (index > 1 && buf[index - 1] == text[i] && buf[index - 2] == text[i])
                continue;  // ❌ 未检查 index >= 2
        }
		
        buf[index++] = text[i];
    }
    buf[index] = 0;
    *len = index;
    memcpy(text, buf, ((*len) + 1) * sizeof(wchar_t));
    free(buf);
    return true;
}
```

### 修复方案
```cpp
bool TextBook::FormatText(wchar_t *text, int *len, bool flag)
{
    wchar_t *buf = NULL;
    int i, index = 0;
    if (!text || *len == 0)
        return false;

    buf = (wchar_t *)malloc(((*len) + 1) * sizeof(wchar_t));
    if (!buf)  // ✅ 检查内存分配
        return false;
        
    for (i = 0; i < (*len); i++)
    {
        if (flag && IsBlanks(text[i]))
            continue;
        flag = false;
        
        if (i < ((*len) - 1) && text[i] == 0x00)
            continue; 

        if (text[i] == 0x0D)
            continue;

        // Remove extra spaces
        if (0x20 == text[i] || 0xA0 == text[i])
        {
            // ✅ 修正：使用 >= 确保安全访问
            if (index >= 4 && buf[index - 1] == text[i] && buf[index - 2] == text[i] 
                && buf[index - 3] == text[i] && buf[index - 4] == text[i])
                continue;
        }
        else if (IsBlanks(text[i]))
        {
            // ✅ 修正：使用 >= 确保安全访问
            if (index >= 2 && buf[index - 1] == text[i] && buf[index - 2] == text[i])
                continue;
        }
		
        buf[index++] = text[i];
    }
    buf[index] = 0;
    *len = index;
    memcpy(text, buf, ((*len) + 1) * sizeof(wchar_t));
    free(buf);
    return true;
}
```

---

## Bug #3: Cache::save memcmp 错误 - 修复方案

### 原问题代码 (Cache.cpp:107-145)
```cpp
bool Cache::save()
{
    bool result = false;
    char* json = NULL;
    int size = 0;

    json = create_json(get_header());
    if (json)
    {
        size = strlen(json);
        encode(json, size);

        // check if data is changed
        if (size != m_jsonlen
            || 0 != memcmp(m_jsonbak, json, m_jsonlen))  // ❌ 错误：比较长度用的 m_jsonlen
        {
            result = write(json, size);

            // backup
            if (size != m_jsonlen)
            {
                m_jsonlen = size;
                m_jsonbak = realloc(m_jsonbak, m_jsonlen);
                if (!m_jsonbak)
                {
                    m_jsonlen = 0;
                }
            }
            if (m_jsonbak)
                memcpy(m_jsonbak, json, m_jsonlen);  // ❌ 可能超界
        }
        else
        {
            result = true;
        }
    }
    create_json_free(json);
    return result;
}
```

### 修复方案
```cpp
bool Cache::save()
{
    bool result = false;
    char* json = NULL;
    int size = 0;

    json = create_json(get_header());
    if (json)
    {
        size = strlen(json);
        encode(json, size);

        // ✅ 修复：分离比较长度，避免数据不一致
        bool data_changed = false;
        if (size != m_jsonlen)
        {
            data_changed = true;
        }
        else if (m_jsonbak)
        {
            // ✅ 使用实际的 size 作为比较长度
            data_changed = (0 != memcmp(m_jsonbak, json, size));
        }

        if (data_changed)
        {
            result = write(json, size);

            // ✅ 更新备份
            if (size != m_jsonlen)
            {
                m_jsonlen = size;
                void* new_bak = realloc(m_jsonbak, m_jsonlen);
                if (new_bak)
                {
                    m_jsonbak = new_bak;
                }
                else
                {
                    free(m_jsonbak);
                    m_jsonbak = NULL;
                    m_jsonlen = 0;
                    create_json_free(json);
                    return false;  // 内存不足，返回失败
                }
            }
            
            if (m_jsonbak && m_jsonlen > 0)
                memcpy(m_jsonbak, json, m_jsonlen);
        }
        else
        {
            result = true;  // no change, don't need write file
        }
    }
    create_json_free(json);
    return result;
}
```

---

## Bug #4: DecodeText 空指针检查 - 修复方案

### 原问题代码 (Book.cpp:236-305)
```cpp
bool Book::DecodeText(const char *src, int srcsize, wchar_t **dst, int *dstsize)
{
    type_t bom = Unknown;

    if (Unknown != (bom = Utils::check_bom(src, srcsize)))
    {
        if (utf8 == bom)
        {
            src += 3;
            srcsize -= 3;
            *dst = Utils::utf8_to_utf16_ex(src, srcsize, dstsize);  // ❌ 未检查返回值
            (*dstsize)--;
        }
        // ... 其他分支类似 ...
    }
    else if (Utils::is_ascii(src, srcsize > 1024 ? 1024 : srcsize))
    {
        *dst = Utils::utf8_to_utf16_ex(src, srcsize, dstsize);  // ❌ 未检查返回值
    }
    // ...
    
    FormatText(*dst, dstsize);  // ❌ *dst 可能为 NULL

    return true;
}
```

### 修复方案
```cpp
bool Book::DecodeText(const char *src, int srcsize, wchar_t **dst, int *dstsize)
{
    type_t bom = Unknown;

    if (!src || srcsize <= 0 || !dst || !dstsize)  // ✅ 参数验证
        return false;

    if (Unknown != (bom = Utils::check_bom(src, srcsize)))
    {
        if (utf8 == bom)
        {
            src += 3;
            srcsize -= 3;
            *dst = Utils::utf8_to_utf16_ex(src, srcsize, dstsize);
            if (*dst == NULL)  // ✅ 检查返回值
                return false;
            (*dstsize)--;
        }
        else if (utf16_le == bom)
        {
            src += 2;
            srcsize -= 2;
            *dstsize = srcsize / 2;
            *dst = (wchar_t *)malloc(sizeof(wchar_t) * ((*dstsize) + 1));
            if (*dst == NULL)  // ✅ 检查内存分配
                return false;
            memcpy(*dst, src, srcsize);
            (*dst)[*dstsize] = 0;
        }
        else if (utf16_be == bom)
        {
            src += 2;
            srcsize -= 2;
            *dstsize = srcsize / 2;
            *dst = (wchar_t *)malloc(sizeof(wchar_t) * ((*dstsize) + 1));
            if (*dst == NULL)  // ✅ 检查内存分配
                return false;
            memcpy(*dst, src, srcsize);
            (*dst)[*dstsize] = 0;
            *dst = (wchar_t *)Utils::be_to_le((char *)(*dst), srcsize);
        }
        else if (utf32_le == bom || utf32_be == bom)
        {
            return false;  // not support
        }
    }
    else if (Utils::is_ascii(src, srcsize > 1024 ? 1024 : srcsize))
    {
        *dst = Utils::utf8_to_utf16_ex(src, srcsize, dstsize);
        if (*dst == NULL)  // ✅ 检查返回值
            return false;
    }
    else if (Utils::is_utf8(src, srcsize > 1024 ? 1024 : srcsize))
    {
        *dst = Utils::utf8_to_utf16_ex(src, srcsize, dstsize);
        if (*dst == NULL)  // ✅ 检查返回值
            return false;
    }
    else
    {
        *dst = Utils::ansi_to_utf16_ex(src, srcsize, dstsize);
        if (*dst == NULL)  // ✅ 检查返回值
            return false;
    }

    if (!FormatText(*dst, dstsize))  // ✅ 检查返回值
        return false;

    return true;
}
```

---

## Bug #13: Utils 线程不安全 - 修复方案

### 原问题代码 (Utils.cpp:177-243)
```cpp
char* Utils::g_result = NULL;        // ❌ 全局变量，线程不安全
int Utils::g_len = 0;
wchar_t* Utils::g_wresult = NULL;    // ❌ 全局变量，线程不安全
int Utils::g_wlen = 0;

char* Utils::Utf16ToUtf8(const wchar_t* str)
{
    int len;
    len = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    if (!g_result)
    {
        g_len = len + 1;
        g_result = (char*)malloc(g_len * sizeof(char));
    }
    else if (g_len < len + 1)
    {
        g_len = len + 1;
        g_result = (char*)realloc(g_result, g_len * sizeof(char));
    }

    WideCharToMultiByte(CP_UTF8, 0, str, -1, g_result, len, NULL, NULL);
    g_result[len] = 0;
    return g_result;  // ❌ 返回全局指针，不安全
}
```

### 修复方案 A - 使用线程本地存储 (推荐 C++11+)
```cpp
// Utils.h
#if __cplusplus >= 201103L
    // C++11 及以后
    #define THREAD_LOCAL thread_local
#else
    // 较早版本，使用 Windows API
    #define THREAD_LOCAL __declspec(thread)
#endif

// Utils.cpp
THREAD_LOCAL char* Utils::g_result = NULL;
THREAD_LOCAL int Utils::g_len = 0;
THREAD_LOCAL wchar_t* Utils::g_wresult = NULL;
THREAD_LOCAL int Utils::g_wlen = 0;

// 其余代码保持不变，但现在是线程安全的
```

### 修复方案 B - 每次分配释放（兼容性最好）
```cpp
// Utils.h - 修改函数签名或添加新函数
class Utils
{
public:
    // ✅ 新函数：调用者负责释放
    static char* Utf16ToUtf8Safe(const wchar_t* str, int* out_len);
    static wchar_t* Utf8ToUtf16Safe(const char* str, int* out_len);
    
    // 保留旧函数用于向后兼容（但标记为不安全）
    static char* Utf16ToUtf8(const wchar_t* str);  // 已弃用
    
private:
    // 删除全局缓冲区声明
    // static char* g_result;
    // ...
};

// Utils.cpp
char* Utils::Utf16ToUtf8Safe(const wchar_t* str, int* out_len)
{
    if (!str || !out_len)
        return NULL;
    
    int len = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    if (len <= 0)
    {
        *out_len = 0;
        return NULL;
    }
    
    char* result = (char*)malloc(len * sizeof(char));
    if (!result)
    {
        *out_len = 0;
        return NULL;
    }
    
    if (!WideCharToMultiByte(CP_UTF8, 0, str, -1, result, len, NULL, NULL))
    {
        free(result);
        *out_len = 0;
        return NULL;
    }
    
    *out_len = len - 1;  // 不包括 null 终止符
    return result;  // 调用者必须 free
}

// ✅ 保留旧函数用于兼容，但使用内部静态缓冲
char* Utils::Utf16ToUtf8(const wchar_t* str)
{
    // 警告：此函数不线程安全！使用 Utf16ToUtf8Safe 代替
    static char* g_result = NULL;
    static int g_len = 0;
    
    int len = WideCharToMultiByte(CP_UTF8, 0, str, -1, NULL, 0, NULL, NULL);
    if (!g_result)
    {
        g_len = len + 1;
        g_result = (char*)malloc(g_len * sizeof(char));
    }
    else if (g_len < len + 1)
    {
        g_len = len + 1;
        g_result = (char*)realloc(g_result, g_len * sizeof(char));
    }

    if (g_result)
    {
        WideCharToMultiByte(CP_UTF8, 0, str, -1, g_result, len, NULL, NULL);
        g_result[len] = 0;
    }
    return g_result;
}
```

---

## Bug #11: OnlineBook 索引越界 - 修复方案

### 原问题代码 (OnlineBook.cpp:149-164)
```cpp
void OnlineBook::JumpChapter(HWND hWnd, int index)
{
    if (index < 0 || index > (int)m_Chapters.size())  // ❌ 应该用 >=
        return;

    if (m_Chapters[index].index == -1)
    {
        m_TagetIndex = index;
        ParserContent(hWnd, index, todo_jump);
        PlayLoading(hWnd);
    }
    else
    {
        Book::JumpChapter(hWnd, index);
    }
}
```

### 修复方案
```cpp
void OnlineBook::JumpChapter(HWND hWnd, int index)
{
    // ✅ 修正：使用 >= 而非 >，因为有效索引是 [0, size-1]
    if (index < 0 || index >= (int)m_Chapters.size())
        return;

    // ✅ 添加额外验证
    if (m_Chapters.empty())
        return;

    if (m_Chapters[index].index == -1)
    {
        m_TagetIndex = index;
        ParserContent(hWnd, index, todo_jump);
        PlayLoading(hWnd);
    }
    else
    {
        Book::JumpChapter(hWnd, index);
    }
}

// ✅ 同样修复其他地方
void OnlineBook::JumpPrevChapter(HWND hWnd)
{
    int cur = GetCurChapterIndex();
    int prev = cur - 1;

    if (cur == -1)
        return;

    if (m_Chapters.empty())
        return;

    if (prev >= 0 && prev < (int)m_Chapters.size())  // ✅ 修复
    {
        if (m_Chapters[prev].index == -1)
        {
            m_TagetIndex = prev;
            ParserContent(hWnd, prev, todo_jump);
            PlayLoading(hWnd);
        }
        else
        {
            (*m_CurrentPos) = m_Chapters[prev].index;
            Reset(hWnd);
        }
    }
}

void OnlineBook::JumpNextChapter(HWND hWnd)
{
    int cur = GetCurChapterIndex();
    int next = cur + 1;

    if (cur == -1)
        return;

    if (m_Chapters.empty())
        return;

    if (next >= 0 && next < (int)m_Chapters.size())  // ✅ 修复
    {
        if (m_Chapters[next].index == -1)
        {
            m_TagetIndex = next;
            ParserContent(hWnd, next, todo_jump);
            PlayLoading(hWnd);
        }
        else
        {
            (*m_CurrentPos) = m_Chapters[next].index;
            Reset(hWnd);
        }
    }
}
```

---

## 总结

| Bug | 文件 | 修复方案 | 优先级 |
|-----|------|---------|--------|
| #1 | EpubBook.cpp | 统一内存释放点 | 🔴 P1 |
| #2 | TextBook.cpp | 修正边界检查 | 🔴 P1 |
| #3 | Cache.cpp | 分离长度比较 | 🔴 P1 |
| #4 | Book.cpp | 添加空指针检查 | 🟠 P2 |
| #6 | Utils.cpp | 重新计算分配大小 | 🟠 P2 |
| #11 | OnlineBook.cpp | 修正索引比较 | 🟠 P2 |
| #13 | Utils.cpp | 使用线程本地存储 | 🟠 P2 |

建议按照优先级顺序修复这些 Bug。

