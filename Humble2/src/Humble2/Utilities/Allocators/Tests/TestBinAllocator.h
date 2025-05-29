#pragma once

// BinAllocatorTests.cpp

#include "Utilities/Allocators/BinAllocator.h"
#include <vector>
#include <random>
#include <chrono>
#include <iostream>

using namespace HBL2;

// Test framework macros
#define TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            std::cout << "FAILED: " << #condition << " at line " << __LINE__ << std::endl; \
            return false; \
        } \
    } while(0)

#define RUN_TEST(test_func) \
    do { \
        std::cout << "Running " << #test_func << "... "; \
        if (test_func()) { \
            std::cout << "PASSED" << std::endl; \
        } else { \
            std::cout << "FAILED" << std::endl; \
            return 1; \
        } \
    } while(0)

// Test basic allocation and deallocation
bool test_basic_allocation()
{
    BinAllocator allocator(1024, 100);

    // Test basic allocation
    int* ptr1 = allocator.Allocate<int>();
    TEST_ASSERT(ptr1 != nullptr);

    float* ptr2 = allocator.Allocate<float>(sizeof(float) * 10);
    TEST_ASSERT(ptr2 != nullptr);

    // Test that pointers are different
    TEST_ASSERT(ptr1 != reinterpret_cast<int*>(ptr2));

    // Test deallocation
    allocator.Deallocate(ptr1);
    allocator.Deallocate(ptr2);

    return true;
}

// Test memory alignment
bool test_memory_alignment()
{
    BinAllocator allocator(1024, 100);

    struct AlignedStruct
    {
        alignas(16) char data[32];
    };

    AlignedStruct* ptr = allocator.Allocate<AlignedStruct>();
    TEST_ASSERT(ptr != nullptr);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(ptr) % 16 == 0);

    allocator.Deallocate(ptr);

    return true;
}

// Test coalescing behavior
bool test_coalescing()
{
    BinAllocator allocator(1024, 100);

    // Allocate three adjacent blocks
    char* ptr1 = allocator.Allocate<char>(100);
    char* ptr2 = allocator.Allocate<char>(100);
    char* ptr3 = allocator.Allocate<char>(100);

    TEST_ASSERT(ptr1 != nullptr);
    TEST_ASSERT(ptr2 != nullptr);
    TEST_ASSERT(ptr3 != nullptr);

    auto report1 = allocator.GetStorageReport();

    // Free middle block
    allocator.Deallocate(ptr2);

    auto report2 = allocator.GetStorageReport();
    TEST_ASSERT(report2.totalFreeSpace > report1.totalFreeSpace);

    // Free first block - should coalesce with middle
    allocator.Deallocate(ptr1);

    auto report3 = allocator.GetStorageReport();
    TEST_ASSERT(report3.totalFreeSpace > report2.totalFreeSpace);
    TEST_ASSERT(report3.largestFreeRegion >= 200); // At least size of two blocks

    // Free third block - should coalesce all three
    allocator.Deallocate(ptr3);

    auto report4 = allocator.GetStorageReport();
    TEST_ASSERT(report4.largestFreeRegion >= 300); // At least size of three blocks

    return true;
}

// Test out of memory conditions
bool test_out_of_memory()
{
    BinAllocator allocator(100, 10); // Small allocator

    std::vector<void*> ptrs;

    // Allocate until out of memory
    for (int i = 0; i < 20; ++i)
    {
        void* ptr = allocator.Allocate<char>(10);
        if (ptr == nullptr)
            break;
        ptrs.push_back(ptr);
    }

    // Should have allocated some memory
    TEST_ASSERT(!ptrs.empty());

    // Try to allocate more - should fail
    void* ptr = allocator.Allocate<char>(50);
    TEST_ASSERT(ptr == nullptr);

    // Clean up
    for (void* p : ptrs)
    {
        allocator.Deallocate(static_cast<char*>(p));
    }

    return true;
}

// Test size class distribution
bool test_size_classes()
{
    BinAllocator allocator(4096, 1000);

    std::vector<std::pair<void*, size_t>> allocations;

    // Test various power-of-2 sizes
    std::vector<size_t> sizes = { 1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024 };

    for (size_t size : sizes)
    {
        void* ptr = allocator.Allocate<char>(size);
        TEST_ASSERT(ptr != nullptr);
        allocations.push_back({ ptr, size });
    }

    // Clean up
    for (auto& alloc : allocations)
    {
        allocator.Deallocate(static_cast<char*>(alloc.first));
    }

    return true;
}

// Test multiple types with different alignments
bool test_mixed_types()
{
    BinAllocator allocator(2048, 100);

    struct TestStruct1 { char data[7]; };
    struct TestStruct2 { alignas(8) int64_t data; };
    struct TestStruct3 { alignas(16) double data[2]; };

    TestStruct1* ptr1 = allocator.Allocate<TestStruct1>();
    TestStruct2* ptr2 = allocator.Allocate<TestStruct2>();
    TestStruct3* ptr3 = allocator.Allocate<TestStruct3>();

    TEST_ASSERT(ptr1 != nullptr);
    TEST_ASSERT(ptr2 != nullptr);
    TEST_ASSERT(ptr3 != nullptr);

    // Check alignments
    TEST_ASSERT(reinterpret_cast<uintptr_t>(ptr2) % 8 == 0);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(ptr3) % 16 == 0);

    allocator.Deallocate(ptr1);
    allocator.Deallocate(ptr2);
    allocator.Deallocate(ptr3);

    return true;
}

// Test invalidate functionality
bool test_invalidate()
{
    BinAllocator allocator(1024, 100);

    // Allocate some memory
    std::vector<void*> ptrs;
    for (int i = 0; i < 10; ++i)
    {
        void* ptr = allocator.Allocate<char>(50);
        TEST_ASSERT(ptr != nullptr);
        ptrs.push_back(ptr);
    }

    auto report1 = allocator.GetStorageReport();
    TEST_ASSERT(report1.totalFreeSpace < 1024);

    // Invalidate
    allocator.Invalidate();

    auto report2 = allocator.GetStorageReport();
    TEST_ASSERT(report2.totalFreeSpace == 1024); // Should be back to full capacity

    // Should be able to allocate again
    void* newPtr = allocator.Allocate<char>(100);
    TEST_ASSERT(newPtr != nullptr);

    allocator.Deallocate(static_cast<char*>(newPtr));

    return true;
}

// Test edge cases
bool test_edge_cases()
{
    BinAllocator allocator(1024, 100);

    // Test zero-size allocation
    void* ptr1 = allocator.Allocate<char>(0);
    TEST_ASSERT(ptr1 != nullptr);

    // Test very small allocation
    void* ptr2 = allocator.Allocate<char>(1);
    TEST_ASSERT(ptr2 != nullptr);

    // Test null pointer deallocation (should not crash)
    allocator.Deallocate(static_cast<char*>(nullptr));

    allocator.Deallocate(static_cast<char*>(ptr1));
    allocator.Deallocate(static_cast<char*>(ptr2));

    return true;
}

// Test fragmentation patterns
bool test_fragmentation()
{
    BinAllocator allocator(2048, 200);

    std::vector<void*> ptrs;

    // Allocate many small blocks
    for (int i = 0; i < 50; ++i)
    {
        void* ptr = allocator.Allocate<char>(32);
        TEST_ASSERT(ptr != nullptr);
        ptrs.push_back(ptr);
    }

    // Free every other block to create fragmentation
    for (size_t i = 1; i < ptrs.size(); i += 2)
    {
        allocator.Deallocate(static_cast<char*>(ptrs[i]));
        ptrs[i] = nullptr;
    }

    auto report1 = allocator.GetStorageReport();

    // Try to allocate a larger block
    void* largePtr = allocator.Allocate<char>(64);
    TEST_ASSERT(largePtr != nullptr);

    // Clean up
    allocator.Deallocate(static_cast<char*>(largePtr));
    for (void* ptr : ptrs)
    {
        if (ptr != nullptr)
        {
            allocator.Deallocate(static_cast<char*>(ptr));
        }
    }

    return true;
}

// Performance test
bool test_performance()
{
    BinAllocator allocator(10 * 1024 * 1024, 10000); // 10MB

    const int numAllocations = 1000;
    std::vector<void*> ptrs(numAllocations);

    auto start = std::chrono::high_resolution_clock::now();

    // Allocation phase
    for (int i = 0; i < numAllocations; ++i)
    {
        size_t size = 16 + (i % 1024); // Variable sizes
        ptrs[i] = allocator.Allocate<char>(size);
        TEST_ASSERT(ptrs[i] != nullptr);
    }

    auto mid = std::chrono::high_resolution_clock::now();

    // Deallocation phase
    for (int i = 0; i < numAllocations; ++i)
    {
        allocator.Deallocate(static_cast<char*>(ptrs[i]));
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto allocTime = std::chrono::duration_cast<std::chrono::microseconds>(mid - start);
    auto deallocTime = std::chrono::duration_cast<std::chrono::microseconds>(end - mid);

    std::cout << "\n  Allocation time: " << allocTime.count() << " μs";
    std::cout << "\n  Deallocation time: " << deallocTime.count() << " μs";

    return true;
}

int TestBinAllocator()
{
    std::cout << "Running BinAllocator Tests\n" << std::endl;

    RUN_TEST(test_basic_allocation);
    RUN_TEST(test_memory_alignment);
    RUN_TEST(test_coalescing);
    RUN_TEST(test_out_of_memory);
    RUN_TEST(test_size_classes);
    RUN_TEST(test_mixed_types);
    RUN_TEST(test_invalidate);
    RUN_TEST(test_edge_cases);
    RUN_TEST(test_fragmentation);
    RUN_TEST(test_performance);

    std::cout << "\nAll tests passed!" << std::endl;

    return 0;
}