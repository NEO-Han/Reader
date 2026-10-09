#include "stdafx.h"

#ifdef ENABLE_AUTHOR

#include "AuthClient.h"
#include <windows.h>
#include <winhttp.h>
#include <sstream>
#include <string>
#include <comdef.h>
#include <Wbemidl.h>
#include <ShlObj.h>
#include <fstream>
#include "resource.h"
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "Shell32.lib")

INT_PTR CALLBACK LicenseDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
    case WM_INITDIALOG:
    {
        std::string* pOutKey = reinterpret_cast<std::string*>(lParam);
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)pOutKey);
        SetFocus(GetDlgItem(hDlg, IDC_KEY));
        return TRUE;
    }
    case WM_COMMAND:
    {
        if (LOWORD(wParam) == IDOK)
        {
            char key[256] = { 0 };
            GetDlgItemTextA(hDlg, IDC_KEY, key, sizeof(key));
            if (strlen(key) > 0)
            {
                std::string* pOutKey = (std::string*)GetWindowLongPtr(hDlg, DWLP_USER);
                if (pOutKey)
                    *pOutKey = key;
            }
            EndDialog(hDlg, IDOK);
            return TRUE;
        }
        else if (LOWORD(wParam) == IDCANCEL)
        {
            EndDialog(hDlg, IDCANCEL);
            return TRUE;
        }
        break;
    }
    }
    return FALSE;
}

bool GetLicenseKeyFromUser(HWND hWnd, std::string& outKey)
{
    INT_PTR result = DialogBoxParam(GetModuleHandle(NULL),
        MAKEINTRESOURCE(IDD_LICENSE_DLG),
        hWnd,
        LicenseDlgProc,
        (LPARAM)&outKey);
    return result == IDOK;
}

bool AuthClient::ShowAuthDialog(HWND hWnd)
{
    // 1. 先尝试从用户目录读取保存的密钥
    std::string savedKey = LoadLicenseKey();

    // 调试信息
    char debugMsg[512];
    sprintf_s(debugMsg, "密钥文件路径: %s\n读取到的密钥: %s",
        GetLicenseFilePath().c_str(),
        savedKey.empty() ? "(空)" : savedKey.c_str());
    //MessageBoxA(hWnd, debugMsg, "调试信息-读取密钥", MB_OK);

    if (!savedKey.empty())
    {
        // 2. 如果有保存的密钥，先尝试验证
        AuthResult ret = VerifyLicense(savedKey);

        // 调试信息
        sprintf_s(debugMsg, "自动验证结果: %s\n消息: %s",
            ret.success ? "成功" : "失败",
            ret.msg.c_str());
        //MessageBoxA(hWnd, debugMsg, "调试信息-自动验证", MB_OK);

        if (ret.success)
        {
            // 验证成功，直接返回
            return true;
        }
        // 验证失败，继续下面的流程要求用户重新输入
    }

    // 3. 没有保存的密钥或验证失败，弹出对话框要求用户输入
    std::string key;
    if (!GetLicenseKeyFromUser(hWnd, key))
    {
        // 用户取消了输入
        return false;
    }

    // 4. 验证用户输入的密钥
    AuthResult ret = VerifyLicense(key);

    // 调试信息
    sprintf_s(debugMsg, "手动验证结果: %s\n消息: %s",
        ret.success ? "成功" : "失败",
        ret.msg.c_str());
 /*   MessageBoxA(hWnd, debugMsg, "验证结果", MB_OK);*/

    if (ret.success)
    {
        // 5. 验证成功，保存密钥到用户目录
        bool saved = SaveLicenseKey(key);
        sprintf_s(debugMsg, "密钥保存: %s\n路径: %s",
            saved ? "成功" : "失败",
            GetLicenseFilePath().c_str());
  /*      MessageBoxA(hWnd, debugMsg, "保存结果", MB_OK);*/
        return true;
    }
    else
    {
        // 6. 验证失败，删除密钥文件（如果存在）
        DeleteLicenseKey();
        return false;
    }
}

// ==========================
// JSON 转义（完善版）
// ==========================
static std::string EscapeJson(const std::string& s)
{
    std::string res;
    for (char c : s)
    {
        switch (c)
        {
        case '"':  res += "\\\""; break;
        case '\\': res += "\\\\"; break;
        case '\b': res += "\\b";  break;
        case '\f': res += "\\f";  break;
        case '\n': res += "\\n";  break;
        case '\r': res += "\\r";  break;
        case '\t': res += "\\t";  break;
        default:   res += c; break;
        }
    }
    return res;
}

AuthClient::AuthClient(const std::string& serverUrl)
    : m_serverUrl(serverUrl)
{

}

AuthClient::AuthClient()
{
    m_serverUrl = LoadServerUrl();
}

std::string AuthClient::WinHttpPost(const std::string& url, const std::string& jsonData)
{
    std::string response;
    URL_COMPONENTS urlComp = { sizeof(urlComp) };
    wchar_t host[256] = { 0 }, path[256] = { 0 };
    urlComp.lpszHostName = host;
    urlComp.dwHostNameLength = _countof(host);
    urlComp.lpszUrlPath = path;
    urlComp.dwUrlPathLength = _countof(path);

    std::wstring wurl(url.begin(), url.end());
    if (!WinHttpCrackUrl(wurl.c_str(), (DWORD)wurl.size(), 0, &urlComp))
        return "url error";

    HINTERNET hSession = WinHttpOpen(L"AuthClient/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, NULL, NULL, 0);
    if (!hSession) return "open session failed";

    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    std::string errMsg;

    hConnect = WinHttpConnect(hSession, urlComp.lpszHostName, urlComp.nPort, 0);
    if (!hConnect) { errMsg = "connect failed"; goto cleanup; }

    DWORD flags = 0;
    if (urlComp.nPort == INTERNET_DEFAULT_HTTPS_PORT)
        flags |= WINHTTP_FLAG_SECURE;

    hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComp.lpszUrlPath, NULL, NULL, NULL, flags);
    if (!hRequest) { errMsg = "open request failed"; goto cleanup; }

    LPCWSTR headers = L"Content-Type: application/json\r\n";
    WinHttpAddRequestHeaders(hRequest, headers, (DWORD)wcslen(headers), WINHTTP_ADDREQ_FLAG_ADD);

    BOOL sendOk = WinHttpSendRequest(hRequest, NULL, 0,
        (LPVOID)jsonData.data(), (DWORD)jsonData.size(), (DWORD)jsonData.size(), 0);

    if (!sendOk) { errMsg = "send failed"; goto cleanup; }

    WinHttpReceiveResponse(hRequest, NULL);
    char buf[4096];
    DWORD read = 0;
    while (WinHttpReadData(hRequest, buf, sizeof(buf), &read) && read > 0)
        response.append(buf, read);

cleanup:
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    if (!errMsg.empty()) return errMsg;
    return response;
}

// ==========================
// 获取主板UUID
// ==========================
std::string AuthClient::GetMotherboardUUID()
{
    std::string uuid;
    HRESULT hr;

    // 初始化COM（可能已经初始化过，所以失败也继续）
    hr = CoInitializeEx(0, COINIT_MULTITHREADED);
    bool needUninit = SUCCEEDED(hr);

    // 设置COM安全级别（可能已经设置过，失败也继续）
    hr = CoInitializeSecurity(
        NULL,
        -1,
        NULL,
        NULL,
        RPC_C_AUTHN_LEVEL_DEFAULT,
        RPC_C_IMP_LEVEL_IMPERSONATE,
        NULL,
        EOAC_NONE,
        NULL
    );
    // RPC_E_TOO_LATE 表示已经初始化过，这是正常的

    IWbemLocator* pLoc = NULL;
    IWbemServices* pSvc = NULL;
    IEnumWbemClassObject* pEnumerator = NULL;
    IWbemClassObject* pclsObj = NULL;

    do {
        // 创建WMI连接
        hr = CoCreateInstance(
            CLSID_WbemLocator,
            0,
            CLSCTX_INPROC_SERVER,
            IID_IWbemLocator,
            (LPVOID*)&pLoc);

        if (FAILED(hr))
            break;

        // 连接到WMI
        hr = pLoc->ConnectServer(
            _bstr_t(L"ROOT\\CIMV2"),
            NULL,
            NULL,
            0,
            NULL,
            0,
            0,
            &pSvc
        );

        if (FAILED(hr))
            break;

        // 设置代理安全级别
        hr = CoSetProxyBlanket(
            pSvc,
            RPC_C_AUTHN_WINNT,
            RPC_C_AUTHZ_NONE,
            NULL,
            RPC_C_AUTHN_LEVEL_CALL,
            RPC_C_IMP_LEVEL_IMPERSONATE,
            NULL,
            EOAC_NONE
        );

        if (FAILED(hr))
            break;

        // 查询主板信息
        hr = pSvc->ExecQuery(
            bstr_t("WQL"),
            bstr_t("SELECT UUID FROM Win32_ComputerSystemProduct"),
            WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY,
            NULL,
            &pEnumerator);

        if (FAILED(hr))
            break;

        ULONG uReturn = 0;
        hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);

        if (uReturn == 0)
            break;

        VARIANT vtProp;
        VariantInit(&vtProp);

        hr = pclsObj->Get(L"UUID", 0, &vtProp, 0, 0);
        if (SUCCEEDED(hr) && vtProp.vt == VT_BSTR)
        {
            _bstr_t bstrUUID(vtProp.bstrVal, false);
            uuid = (const char*)bstrUUID;
        }

        VariantClear(&vtProp);

    } while (false);

    // 清理
    if (pclsObj)
        pclsObj->Release();
    if (pEnumerator)
        pEnumerator->Release();
    if (pSvc)
        pSvc->Release();
    if (pLoc)
        pLoc->Release();

    if (needUninit)
        CoUninitialize();

    return uuid;
}

// ==========================
// 获取密钥文件路径
// ==========================
std::string AuthClient::GetLicenseFilePath()
{
    char path[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, path)))
    {
        std::string dirPath = std::string(path) + "\\Fisher";
        CreateDirectoryA(dirPath.c_str(), NULL);
        return dirPath + "\\license.key";
    }
    return "";
}

// ==========================
// 保存密钥到用户目录
// ==========================
bool AuthClient::SaveLicenseKey(const std::string& key)
{
    std::string filePath = GetLicenseFilePath();
    if (filePath.empty())
        return false;

    std::ofstream file(filePath, std::ios::binary);
    if (!file.is_open())
        return false;

    file << key;
    file.close();
    return true;
}

// ==========================
// 从用户目录读取密钥
// ==========================
std::string AuthClient::LoadLicenseKey()
{
    std::string filePath = GetLicenseFilePath();
    if (filePath.empty())
        return "";

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open())
        return "";

    std::string key;
    std::getline(file, key);
    file.close();
    return key;
}

// ==========================
// 删除密钥文件
// ==========================
bool AuthClient::DeleteLicenseKey()
{
    std::string filePath = GetLicenseFilePath();
    if (filePath.empty())
        return false;

    return DeleteFileA(filePath.c_str()) != 0;
}

// ==========================
// 使用主板UUID验证密钥
// ==========================
AuthResult AuthClient::VerifyLicense(const std::string& licenseKey)
{
    AuthResult res{};
    res.success = false;

    if (licenseKey.empty()) {
        res.msg = "key empty";
        return res;
    }

    // 获取主板UUID
    std::string uuid = GetMotherboardUUID();

    // 调试信息
    char debugMsg[512];
    sprintf_s(debugMsg, "获取到的主板UUID: %s", uuid.empty() ? "(获取失败)" : uuid.c_str());
 /*   MessageBoxA(NULL, debugMsg, "调试信息-UUID", MB_OK);*/

    if (uuid.empty()) {
        res.msg = "failed to get motherboard UUID";
        return res;
    }

    // 构建JSON请求
    std::string json = "{"
        "\"key\":\"" + EscapeJson(licenseKey) + "\","
        "\"uuid\":\"" + EscapeJson(uuid) + "\""
        "}";

    // 调试信息
    sprintf_s(debugMsg, "发送到服务器的JSON:\n%s\n\n服务器URL:\n%s",
        json.c_str(), m_serverUrl.c_str());
    //MessageBoxA(NULL, debugMsg, "调试信息-请求数据", MB_OK);

    // 发送请求到服务器
    std::string resp = WinHttpPost(m_serverUrl, json);

    // 调试信息
    sprintf_s(debugMsg, "服务器响应:\n%s", resp.c_str());
   /* MessageBoxA(NULL, debugMsg, "调试信息-服务器响应", MB_OK);*/

    if (resp.find("\"status\":\"success\"") != std::string::npos)
    {
        res.success = true;
        res.msg = "authorized (uuid: " + uuid + ")";
    }
    else
    {
        res.msg = "verification failed: " + resp;
    }

    return res;
}

// ==========================
// 获取配置文件路径（exe当前目录下的config.json）
// ==========================
std::string AuthClient::GetConfigFilePath()
{
    char exePath[MAX_PATH];
    if (GetModuleFileNameA(NULL, exePath, MAX_PATH) == 0)
        return "";

    // 获取exe所在目录
    std::string path(exePath);
    size_t pos = path.find_last_of("\\/");
    if (pos == std::string::npos)
        return "";

    return path.substr(0, pos + 1) + "config.json";
}

// ==========================
// 从配置文件读取服务器地址
// ==========================
std::string AuthClient::LoadServerUrl()
{
    std::string configPath = GetConfigFilePath();
    if (configPath.empty())
        return "";

    // 读取配置文件
    std::ifstream file(configPath, std::ios::binary);
    if (!file.is_open())
        return "";

    std::string content;
    std::string line;
    while (std::getline(file, line))
        content += line;
    file.close();

    // 简单的JSON解析：查找 "server_url" 字段
    size_t pos = content.find("\"server_url\"");
    if (pos == std::string::npos)
        return "";

    // 查找冒号后的值
    pos = content.find(":", pos);
    if (pos == std::string::npos)
        return "";

    // 跳过空白字符
    pos++;
    while (pos < content.length() && (content[pos] == ' ' || content[pos] == '\t' || content[pos] == '\n' || content[pos] == '\r'))
        pos++;

    // 查找引号
    if (pos >= content.length() || content[pos] != '"')
        return "";

    pos++; // 跳过开始引号

    // 提取URL值
    size_t endPos = content.find('"', pos);
    if (endPos == std::string::npos)
        return "";

    return content.substr(pos, endPos - pos);
}

#endif