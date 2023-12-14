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

        bool TestOrderBook(CApplication* pApp, std::string& srcDataDir, bool createfile = false, bool isxxfile = false);

    private:
        int SplitSecurityFile(std::string srcDataDir, bool isOrder);
        int SplitSecurityFileOrderQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID);
        int SplitSecurityFileTradeQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID);

    private:
        std::unordered_map<std::string, FILE*> m_watchOrderFILE;
        std::unordered_map<std::string, FILE*> m_watchTradeFILE;
    };

}
#endif //BALIBALI_TEST_H
