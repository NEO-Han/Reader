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
    AuthClient(const std::string& serverUrl);
    AuthResult VerifyLicense(const std::string& licenseKey);
    bool ShowAuthDialog(HWND hWnd);  // µ¯´°ÊÚÈ¨

private:
    std::string m_serverUrl;
    std::string WinHttpPost(const std::string& url, const std::string& jsonData);
    std::vector<std::string> GetAllValidMacAddresses();
};
#endif // ENABLE_AUTHOR