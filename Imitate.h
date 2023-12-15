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
    };

}
#endif //BALIBALI_TEST_H
