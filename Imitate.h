#ifndef BALIBALI_TEST_H
#define BALIBALI_TEST_H

#include "defines.h"
#include "MemoryPool.h"
#include "MDL2Impl.h"

namespace test {
    using namespace TORALEV2API;

    class Imitate {
    public:
        Imitate() = default;
        ~Imitate() = default;

        bool TestOrderBook(std::string& srcDataDir, std::string& watchsecurity, bool createfile = false, bool isusevec = true);

    private:
        int SplitSecurityFile(std::string srcDataDir, bool isOrder);
        int SplitSecurityFileOrderQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID);
        int SplitSecurityFileTradeQuot(std::string &dstDataDir, TTORATstpSecurityIDType SecurityID);

    private:
        //std::list<stNotifyData*> m_dataList;
        PROMD::MDL2Impl* m_md = nullptr;
        std::unordered_map<std::string, bool> m_watchSecurity;
        std::unordered_map<std::string, FILE*> m_watchOrderFILE;
        std::unordered_map<std::string, FILE*> m_watchTradeFILE;
    };

}
#endif //BALIBALI_TEST_H
