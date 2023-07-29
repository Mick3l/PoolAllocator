#include "gtest/gtest.h"

#include <vector>
#include <list>
#include <map>
#include <queue>
#include <unordered_set>
#include <chrono>
#include <fstream>

#include "lib/allocator.h"

TEST(FunctionalityTestSuite, SequenceTemplateTest) {
    mtl::PoolAllocator<int, std::list<std::byte *>> allocator(1, 400);
    auto array = allocator.allocate(100);
    for (int i = 0; i < 100; ++i) {
        array[i] = i;
    }
    ASSERT_EQ(array[99], 99);
}

TEST(FunctionalityTestSuite, ChangeTypeTemplateTest) {
    mtl::PoolAllocator<int> allocator(1, 400);
    mtl::PoolAllocator<double> allocator1(allocator);
    auto array = allocator1.allocate(50);
    for (int i = 0; i < 50; ++i) {
        array[i] = static_cast<double>(i) / 30.l;
    }
    ASSERT_EQ(array[49], 49.l / 30.l);
}

TEST(FunctionalityTestSuite, ConfigTest) {
    mtl::PoolAllocator<int8_t> allocator(std::string("config.json"));
    auto array = allocator.allocate(400);
    array[399] = 100;
    ASSERT_EQ(array[399], 100);
}

TEST(PerformanceTestSuite, ListTest) {
    int STD_allocator, Pool_allocator;
    mtl::PoolAllocator<int> poolAllocator(1e6, 24);
    //testing standard allocator
    {
        auto start = std::chrono::steady_clock::now();
        std::list<int> STD_Vector;
        for (int i = 0; i < 1e6; ++i) {
            STD_Vector.push_back(i);
        }
        auto end = std::chrono::steady_clock::now();
        STD_allocator = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    //testing pool allocator
    {
        std::list<int, mtl::PoolAllocator<int>> Pool_Vector(poolAllocator);
        auto start = std::chrono::steady_clock::now();
        for (int i = 0; i < 1e6; ++i) {
            Pool_Vector.push_back(i);
        }
        auto end = std::chrono::steady_clock::now();
        Pool_allocator = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    std::cout << "LIST TEST" << std::endl
              << "pool allocator: " << Pool_allocator << std::endl
              << "standard allocator: " << STD_allocator << std::endl;
    ASSERT_TRUE(Pool_allocator < STD_allocator);
}

TEST(PerformanceTestSuite, RandomAllocationsTest) {
    int STD_allocator, Pool_allocator;
    mtl::PoolAllocator<std::byte> poolAllocator(800'000, 200, 2, 10'000'000);
    std::allocator<std::byte> STDAllocator;
    //testing standard allocator
    {
        std::ifstream f("RandomNumbers.txt");
        std::vector<std::byte *> STD_Vector(STDAllocator);
        int input;
        auto start = std::chrono::steady_clock::now();
        while (f >> input) {
            STD_Vector.push_back(STDAllocator.allocate(input));
        }
        auto end = std::chrono::steady_clock::now();
        STD_allocator = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    //testing pool allocator
    {
        std::ifstream f("RandomNumbers.txt");
        auto Pool_Vector = std::vector<std::byte *, mtl::PoolAllocator<std::byte *>>
                (mtl::PoolAllocator<std::byte *>(poolAllocator));
        int input;
        auto start = std::chrono::steady_clock::now();
        while (f >> input) {
            Pool_Vector.push_back(poolAllocator.allocate(input));
        }
        auto end = std::chrono::steady_clock::now();
        Pool_allocator = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    }
    std::cout << "RANDOM MEMORY ALLOCATIONS TEST" << std::endl
              << "pool allocator: " << Pool_allocator << std::endl
              << "standard allocator: " << STD_allocator << std::endl;
    ASSERT_TRUE(Pool_allocator < STD_allocator);
}