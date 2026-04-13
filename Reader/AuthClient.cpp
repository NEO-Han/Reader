#include "stdafx.h"

#ifdef ENABLE_AUTHOR
#include "AuthClient.h"
#include <windows.h>
#include <iphlpapi.h>
#include <winhttp.h>
#include <sstream>
#include <string>
#include <vector>
#include "resource.h"
#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "winhttp.lib")

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
    std::string key;
    if (!GetLicenseKeyFromUser(hWnd, key))
        return false;

    AuthResult ret = VerifyLicense(key);
    return ret.success;
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
// ✅ 新功能：获取 所有有效 MAC（有线 + Wi‑Fi）
// ==========================
std::vector<std::string> AuthClient::GetAllValidMacAddresses()
{
    std::vector<std::string> macList;
    ULONG outLen = 0;

    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, NULL, &outLen) != ERROR_BUFFER_OVERFLOW)
        return macList;

    PIP_ADAPTER_ADDRESSES pAddrs = (PIP_ADAPTER_ADDRESSES)malloc(outLen);
    if (!pAddrs) return macList;

    if (GetAdaptersAddresses(AF_UNSPEC, GAA_FLAG_INCLUDE_ALL_INTERFACES, NULL, pAddrs, &outLen) == NO_ERROR)
    {
        PIP_ADAPTER_ADDRESSES p = pAddrs;
        while (p)
        {
            // 只收集：有线以太网 或 Wi‑Fi
            bool isEthernet = (p->IfType == IF_TYPE_ETHERNET_CSMACD);
            bool isWifi = (p->IfType == IF_TYPE_IEEE80211);

            if ((isEthernet || isWifi) && p->PhysicalAddressLength >= 6)
            {
                char buf[32] = { 0 };
                sprintf_s(buf, "%02X-%02X-%02X-%02X-%02X-%02X",
                    p->PhysicalAddress[0],
                    p->PhysicalAddress[1],
                    p->PhysicalAddress[2],
                    p->PhysicalAddress[3],
                    p->PhysicalAddress[4],
                    p->PhysicalAddress[5]);

                macList.push_back(buf);
            }
            p = p->Next;
        }
    }

    free(pAddrs);
    return macList;
}

// ==========================
// ✅ 宽松验证：循环发送所有 MAC，任意一个匹配就成功
// ==========================
AuthResult AuthClient::VerifyLicense(const std::string& licenseKey)
{
    AuthResult res{};
    res.success = false;

    std::vector<std::string> macList = GetAllValidMacAddresses();
    if (macList.empty()) {
        res.msg = "no valid mac found";
        return res;
    }
    if (licenseKey.empty()) {
        res.msg = "key empty";
        return res;
    }

    // 循环发送所有 MAC，只要一个成功就立即返回成功
    for (const std::string& mac : macList)
    {
        std::string json = "{"
            "\"key\":\"" + EscapeJson(licenseKey) + "\","
            "\"mac\":\"" + EscapeJson(mac) + "\""
            "}";

        std::string resp = WinHttpPost(m_serverUrl, json);
        if (resp.find("\"status\":\"success\"") != std::string::npos)
        {
            res.success = true;
            res.msg = "authorized (mac: " + mac + ")";
            return res;
        }
    }

    res.msg = "all mac failed";
    return res;
}

#endif