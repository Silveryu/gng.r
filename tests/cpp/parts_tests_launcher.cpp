#include <gtest/gtest.h>
#include "../../inst/include/gng/GNG.h"
DebugCollector dbg;
using namespace gmum;
//#include "parts_tests.cpp"

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
