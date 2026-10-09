#ifdef ENABLE_AUTHOR
#pragma once

#include <string>
#include <vector>
struct AuthResult
{
    bool success = false;
    std::string msg;
};

class AuthClient
{
public:
    AuthClient();
    AuthClient(const std::string& serverUrl);
    AuthResult VerifyLicense(const std::string& licenseKey);
    bool ShowAuthDialog(HWND hWnd);  // ������Ȩ

    // 从配置文件读取服务器地址
    static std::string LoadServerUrl();
    static std::string GetConfigFilePath();  // 获取配置文件路径

private:
    std::string m_serverUrl;
    std::string WinHttpPost(const std::string& url, const std::string& jsonData);
    std::string GetMotherboardUUID();           // 获取主板UUID
    bool SaveLicenseKey(const std::string& key);    // 保存密钥到用户目录
    std::string LoadLicenseKey();               // 从用户目录读取密钥
    bool DeleteLicenseKey();                    // 删除密钥文件
    std::string GetLicenseFilePath();           // 获取密钥文件路径
};
#endif // ENABLE_AUTHOR