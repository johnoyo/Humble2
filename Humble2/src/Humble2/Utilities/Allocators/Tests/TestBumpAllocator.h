#pragma once

// BumpAllocatorTests.cpp

#include "Utilities/Allocators/BumpAllocator.h"

#include <vector>
#include <chrono>
#include <iostream>
#include <cstring>
#include <random>

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
        } \
    } while(0)

// Test basic allocation functionality
bool test_basic_allocation()
{
    BumpAllocator allocator(1024);

    // Test basic allocation
    int* ptr1 = allocator.Allocate<int>();
    TEST_ASSERT(ptr1 != nullptr);

    float* ptr2 = allocator.Allocate<float>();
    TEST_ASSERT(ptr2 != nullptr);

    // Test that pointers are different and sequential
    TEST_ASSERT(ptr1 != reinterpret_cast<int*>(ptr2));
    TEST_ASSERT(reinterpret_cast<char*>(ptr2) >= reinterpret_cast<char*>(ptr1) + sizeof(int));

    // Test custom size allocation
    char* ptr3 = allocator.Allocate<char>(100);
    TEST_ASSERT(ptr3 != nullptr);
    TEST_ASSERT(ptr3 >= reinterpret_cast<char*>(ptr2) + sizeof(float));

    return true;
}

// Test memory alignment
bool test_memory_alignment()
{
    BumpAllocator allocator(1024);

    // Test various alignment requirements
    struct Aligned1 { alignas(1) char data; };
    struct Aligned4 { alignas(4) int data; };
    struct Aligned8 { alignas(8) double data; };
    struct Aligned16 { alignas(16) char data[32]; };

    Aligned1* ptr1 = allocator.Allocate<Aligned1>();
    TEST_ASSERT(ptr1 != nullptr);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(ptr1) % 1 == 0);

    Aligned4* ptr4 = allocator.Allocate<Aligned4>();
    TEST_ASSERT(ptr4 != nullptr);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(ptr4) % 4 == 0);

    Aligned8* ptr8 = allocator.Allocate<Aligned8>();
    TEST_ASSERT(ptr8 != nullptr);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(ptr8) % 8 == 0);

    Aligned16* ptr16 = allocator.Allocate<Aligned16>();
    TEST_ASSERT(ptr16 != nullptr);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(ptr16) % 16 == 0);

    return true;
}

// Test sequential allocation behavior
bool test_sequential_allocation()
{
    BumpAllocator allocator(1024);

    std::vector<void*> ptrs;

    // Allocate multiple blocks and verify they are sequential
    for (int i = 0; i < 10; ++i)
    {
        int* ptr = allocator.Allocate<int>();
        TEST_ASSERT(ptr != nullptr);

        if (!ptrs.empty())
        {
            char* prevPtr = static_cast<char*>(ptrs.back());
            char* currPtr = reinterpret_cast<char*>(ptr);

            // Current pointer should be at least sizeof(int) ahead with alignment
            TEST_ASSERT(currPtr >= prevPtr + sizeof(int));
        }

        ptrs.push_back(ptr);
    }

    return true;
}

// Test out of memory conditions
bool test_out_of_memory()
{
    BumpAllocator allocator(100); // Small allocator

    std::vector<void*> ptrs;

    // Allocate until out of memory
    for (int i = 0; i < 50; ++i)
    {
        void* ptr = allocator.Allocate<char>(10);
        if (ptr == nullptr)
            break;
        ptrs.push_back(ptr);
    }

    // Should have allocated some memory
    TEST_ASSERT(!ptrs.empty());
    TEST_ASSERT(ptrs.size() <= 10); // Should fit at most 10 blocks of size 10

    // Try to allocate more - should fail
    void* ptr = allocator.Allocate<char>(50);
    TEST_ASSERT(ptr == nullptr);

    return true;
}

// Test large allocations
bool test_large_allocations()
{
    BumpAllocator allocator(1024 * 1024); // 1MB

    // Test large single allocation
    char* largePtr = allocator.Allocate<char>(512 * 1024); // 512KB
    TEST_ASSERT(largePtr != nullptr);

    // Should still have space for smaller allocation
    int* smallPtr = allocator.Allocate<int>();
    TEST_ASSERT(smallPtr != nullptr);

    // Test allocation that's too large
    char* tooLargePtr = allocator.Allocate<char>(600 * 1024); // 600KB (too large for remaining space)
    TEST_ASSERT(tooLargePtr == nullptr);

    return true;
}

// Test memory initialization
bool test_memory_initialization()
{
    BumpAllocator allocator(1024);

    // Allocate some memory
    char* ptr1 = allocator.Allocate<char>(100);
    TEST_ASSERT(ptr1 != nullptr);

    // Memory should be initialized to zero
    for (int i = 0; i < 100; ++i)
    {
        TEST_ASSERT(ptr1[i] == 0);
    }

    // Write some data
    for (int i = 0; i < 100; ++i)
    {
        ptr1[i] = static_cast<char>(i % 256);
    }

    // Verify data was written
    for (int i = 0; i < 100; ++i)
    {
        TEST_ASSERT(ptr1[i] == static_cast<char>(i % 256));
    }

    return true;
}

// Test invalidate functionality
bool test_invalidate()
{
    BumpAllocator allocator(1024);

    // Allocate some memory
    std::vector<void*> ptrs;
    for (int i = 0; i < 10; ++i)
    {
        void* ptr = allocator.Allocate<char>(50);
        TEST_ASSERT(ptr != nullptr);
        ptrs.push_back(ptr);
    }

    float percentageBefore = allocator.GetFullPercentage();
    TEST_ASSERT(percentageBefore > 0.0f);
    TEST_ASSERT(percentageBefore < 100.0f);

    // Write some data to verify it gets cleared
    char* testPtr = static_cast<char*>(ptrs[0]);
    testPtr[0] = 42;

    // Invalidate
    allocator.Invalidate();

    float percentageAfter = allocator.GetFullPercentage();
    TEST_ASSERT(percentageAfter == 0.0f);

    // Memory should be zeroed
    TEST_ASSERT(testPtr[0] == 0);

    // Should be able to allocate from the beginning again
    void* newPtr = allocator.Allocate<char>(100);
    TEST_ASSERT(newPtr != nullptr);
    TEST_ASSERT(newPtr == ptrs[0]); // Should be same as first allocation

    return true;
}

// Test percentage tracking
bool test_percentage_tracking()
{
    BumpAllocator allocator(1000);

    // Initially should be 0%
    TEST_ASSERT(allocator.GetFullPercentage() == 0.0f);

    // Allocate 25% of capacity
    allocator.Allocate<char>(250);
    float percentage1 = allocator.GetFullPercentage();
    TEST_ASSERT(percentage1 >= 24.0f && percentage1 <= 26.0f); // Allow for alignment

    // Allocate another 25%
    allocator.Allocate<char>(250);
    float percentage2 = allocator.GetFullPercentage();
    TEST_ASSERT(percentage2 >= 49.0f && percentage2 <= 51.0f);

    // Allocate remaining capacity
    allocator.Allocate<char>(500);
    float percentage3 = allocator.GetFullPercentage();
    TEST_ASSERT(percentage3 >= 99.0f && percentage3 <= 100.0f);

    return true;
}

// Test edge cases
bool test_edge_cases()
{
    BumpAllocator allocator(1024);

    // Test zero-size allocation
    void* ptr1 = allocator.Allocate<char>(0);
    TEST_ASSERT(ptr1 != nullptr);

    // Test very small allocation
    void* ptr2 = allocator.Allocate<char>(1);
    TEST_ASSERT(ptr2 != nullptr);

    // Test deallocation (should be no-op)
    allocator.Deallocate(static_cast<char*>(ptr1));
    allocator.Deallocate(static_cast<char*>(ptr2));

    // Should still be able to allocate
    void* ptr3 = allocator.Allocate<char>(10);
    TEST_ASSERT(ptr3 != nullptr);

    return true;
}

// Test different data types
bool test_mixed_types()
{
    BumpAllocator allocator(2048);

    // Test various types
    bool* boolPtr = allocator.Allocate<bool>();
    char* charPtr = allocator.Allocate<char>();
    short* shortPtr = allocator.Allocate<short>();
    int* intPtr = allocator.Allocate<int>();
    long* longPtr = allocator.Allocate<long>();
    float* floatPtr = allocator.Allocate<float>();
    double* doublePtr = allocator.Allocate<double>();

    TEST_ASSERT(boolPtr != nullptr);
    TEST_ASSERT(charPtr != nullptr);
    TEST_ASSERT(shortPtr != nullptr);
    TEST_ASSERT(intPtr != nullptr);
    TEST_ASSERT(longPtr != nullptr);
    TEST_ASSERT(floatPtr != nullptr);
    TEST_ASSERT(doublePtr != nullptr);

    // Test alignment for each type
    TEST_ASSERT(reinterpret_cast<uintptr_t>(boolPtr) % alignof(bool) == 0);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(charPtr) % alignof(char) == 0);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(shortPtr) % alignof(short) == 0);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(intPtr) % alignof(int) == 0);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(longPtr) % alignof(long) == 0);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(floatPtr) % alignof(float) == 0);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(doublePtr) % alignof(double) == 0);

    // Test writing and reading data
    *boolPtr = true;
    *charPtr = 'A';
    *shortPtr = 12345;
    *intPtr = 987654321;
    *longPtr = 1234567890LL;
    *floatPtr = 3.14159f;
    *doublePtr = 2.71828;

    TEST_ASSERT(*boolPtr == true);
    TEST_ASSERT(*charPtr == 'A');
    TEST_ASSERT(*shortPtr == 12345);
    TEST_ASSERT(*intPtr == 987654321);
    TEST_ASSERT(*longPtr == 1234567890LL);
    TEST_ASSERT(*floatPtr == 3.14159f);
    TEST_ASSERT(*doublePtr == 2.71828);

    return true;
}

bool test_array_allocations()
{
    BumpAllocator allocator(8192); // Increased size for better alignment

    // Test various array sizes
    int* intArray = allocator.Allocate<int>(100 * sizeof(int));
    TEST_ASSERT(intArray != nullptr);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(intArray) % alignof(int) == 0);

    double* doubleArray = allocator.Allocate<double>(50 * sizeof(double));
    TEST_ASSERT(doubleArray != nullptr);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(doubleArray) % alignof(double) == 0);

    struct TestStruct
    {
        int a;
        double b;
        char c[16];
    };

    TestStruct* structArray = allocator.Allocate<TestStruct>(10 * sizeof(TestStruct));
    TEST_ASSERT(structArray != nullptr);
    TEST_ASSERT(reinterpret_cast<uintptr_t>(structArray) % alignof(TestStruct) == 0);

    // Test writing to arrays with validation
    for (int i = 0; i < 100; ++i)
    {
        intArray[i] = i * i;
    }

    for (int i = 0; i < 50; ++i)
    {
        doubleArray[i] = i * 0.5;
    }

    for (int i = 0; i < 10; ++i)
    {
        structArray[i].a = i;
        structArray[i].b = i * 1.5;
        std::snprintf(structArray[i].c, 16, "item_%d", i);
    }

    // Verify data immediately after writing
    for (int i = 0; i < 100; ++i)
    {
        if (intArray[i] != i * i)
        {
            std::cout << "Array corruption at index " << i << ": expected " << (i * i) << ", got " << intArray[i] << std::endl;
            TEST_ASSERT(false);
        }
    }

    for (int i = 0; i < 50; ++i)
    {
        TEST_ASSERT(doubleArray[i] == i * 0.5);
    }

    for (int i = 0; i < 10; ++i)
    {
        TEST_ASSERT(structArray[i].a == i);
        TEST_ASSERT(structArray[i].b == i * 1.5);
    }

    return true;
}

// Test initialization with different sizes
bool test_initialization()
{
    // Test various initialization sizes
    std::vector<size_t> sizes = { 64, 256, 1024, 4096, 16384, 65536 };

    for (size_t size : sizes)
    {
        BumpAllocator allocator(size);

        // Should be able to allocate at least one byte
        void* ptr = allocator.Allocate<char>(1);
        TEST_ASSERT(ptr != nullptr);

        TEST_ASSERT(allocator.GetFullPercentage() > 0.0f);
        TEST_ASSERT(allocator.GetFullPercentage() < 100.0f);
    }

    return true;
}

bool test_performance()
{
    BumpAllocator allocator(10 * 1024 * 1024); // 10MB

    const int numAllocations = 100000;
    std::vector<void*> ptrs(numAllocations);

    auto start = std::chrono::high_resolution_clock::now();

    // Allocation phase
    for (int i = 0; i < numAllocations; ++i)
    {
        size_t size = 8 + (i % 64); // Variable sizes
        ptrs[i] = allocator.Allocate<char>(size);
        TEST_ASSERT(ptrs[i] != nullptr);
    }

    auto mid = std::chrono::high_resolution_clock::now();

    // "Deallocation" phase (no-op for bump allocator)
    for (int i = 0; i < numAllocations; ++i)
    {
        allocator.Deallocate(static_cast<char*>(ptrs[i]));
    }

    auto end = std::chrono::high_resolution_clock::now();

    auto allocTime = std::chrono::duration_cast<std::chrono::microseconds>(mid - start);
    auto deallocTime = std::chrono::duration_cast<std::chrono::microseconds>(end - mid);

    std::cout << "\n  Allocation time: " << allocTime.count() << " μs";
    std::cout << "\n  Deallocation time: " << deallocTime.count() << " μs";
    std::cout << "\n  Average allocation time: " << (double)allocTime.count() / numAllocations << " μs";

    // Bump allocator should be very fast
    TEST_ASSERT(allocTime.count() < 50000); // Should complete in less than 50ms

    return true;
}

// Test memory usage patterns
bool test_memory_patterns()
{
    BumpAllocator allocator(2048);

    // Pattern 1: Many small allocations
    std::vector<void*> smallPtrs;
    for (int i = 0; i < 100; ++i)
    {
        void* ptr = allocator.Allocate<char>(8);
        TEST_ASSERT(ptr != nullptr);
        smallPtrs.push_back(ptr);
    }

    float percentage1 = allocator.GetFullPercentage();

    // Pattern 2: Few large allocations
    allocator.Invalidate();

    std::vector<void*> largePtrs;
    for (int i = 0; i < 4; ++i)
    {
        void* ptr = allocator.Allocate<char>(200);
        TEST_ASSERT(ptr != nullptr);
        largePtrs.push_back(ptr);
    }

    float percentage2 = allocator.GetFullPercentage();

    // Both patterns should use similar amounts of memory
    TEST_ASSERT(std::abs(percentage1 - percentage2) < 10.0f);

    return true;
}

bool stress_test_random_allocations()
{
    std::cout << "Running stress test with random allocations... ";

    BumpAllocator allocator(50 * 1024 * 1024); // 50MB

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> sizeDist(1, 4096);

    std::vector<void*> ptrs;

    // Allocate random sizes until out of memory
    while (true)
    {
        size_t size = sizeDist(gen);
        void* ptr = allocator.Allocate<char>(size);

        if (ptr == nullptr)
            break;

        ptrs.push_back(ptr);

        // Write some data to verify memory is accessible
        std::memset(ptr, 0xAA, size);
    }

    std::cout << "Allocated " << ptrs.size() << " blocks. ";

    // Verify all memory is still accessible
    for (size_t i = 0; i < ptrs.size(); ++i)
    {
        char* cptr = static_cast<char*>(ptrs[i]);
        if (cptr[0] != static_cast<char>(0xAA))
        {
            std::cout << "FAILED - Memory corruption detected!" << std::endl;
            return false;
        }
    }

    std::cout << "PASSED" << std::endl;
    return true;
}

int TestBumpAllocator()
{
    std::cout << "Running BumpAllocator Tests\n" << std::endl;

    RUN_TEST(test_basic_allocation);
    RUN_TEST(test_memory_alignment);
    RUN_TEST(test_sequential_allocation);
    RUN_TEST(test_out_of_memory);
    RUN_TEST(test_large_allocations);
    RUN_TEST(test_memory_initialization);
    RUN_TEST(test_invalidate);
    RUN_TEST(test_percentage_tracking);
    RUN_TEST(test_edge_cases);
    RUN_TEST(test_mixed_types);
    RUN_TEST(test_array_allocations);
    RUN_TEST(test_initialization);
    RUN_TEST(test_performance);
    RUN_TEST(test_memory_patterns);
    RUN_TEST(stress_test_random_allocations);

    std::cout << "\nAll tests passed!" << std::endl;

    return 0;
}