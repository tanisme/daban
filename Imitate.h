#ifndef BALIBALI_TEST_H
#define BALIBALI_TEST_H

#include "defines.h"

class CApplication;
namespace test {
    using namespace TORALEV2API;

    class Imitate {
    public:
        Imitate() = default;
        ~Imitate() = default;
        void TestOrderBook(CApplication* pApp, std::string& srcDataDir);

    private:
        int SplitSecurityFile(std::string srcDataDir, bool isOrder);
        int SplitSecurityFileOrderQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID);
        int SplitSecurityFileTradeQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID);

    private:
        CApplication* m_pApp = nullptr;
        std::unordered_map<std::string, bool> m_watchSecurity;
        std::unordered_map<std::string, FILE*> m_watchOrderFILE;
        std::unordered_map<std::string, FILE*> m_watchTradeFILE;
    };

}
#endif //BALIBALI_TEST_H
