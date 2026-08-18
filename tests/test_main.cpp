#include <cstdio>

int RunSourceMathTests();
int RunPositionMappingTests();
int RunReticleProjectionTests();

int main() {
    std::printf("Portal2HeadTracking tests\n=========================\n");
    const int failures = RunSourceMathTests() + RunPositionMappingTests()
                          + RunReticleProjectionTests();
    if (failures == 0) {
        std::printf("\nAll tests passed\n");
        return 0;
    }
    std::printf("\n%d test(s) FAILED\n", failures);
    return 1;
}
