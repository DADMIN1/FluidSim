#include "Threading.hpp"

#include <iostream>
#include <future>
#include <vector>


void ThreadManager::PrintThreadcount() {
    std::cout << "\nreported threadcount: " << this->THREAD_COUNT << '\n';
    std::cout << "using threadcount: " << ::THREAD_COUNT << '\n' << '\n';
}


// iterates over the result of DivideContainer()
void ThreadManager::ContainerDivTest()
{
    std::cout << "\nTesting DivideContainer()\n";
    constexpr int arrsize = 123;
    std::array<int, arrsize> testcontainer;
    for (int x{0}; x < arrsize; ++x) { testcontainer[x] = x; }
    
    auto dividetest = DivideContainer(testcontainer);
    for (auto p: dividetest) {
        for (auto iter{p.first}; iter != p.second; ++iter) {
            if (*iter < 10)  {std::cout << " ";}
            if (*iter < 100) {std::cout << " ";}
            std::cout << *iter << ", ";
        }
        std::cout << '\n';
    }
}


void ThreadManager::ContainerDivTestMT()
{
    std::cout << "\nTesting DivideContainer() (multithreading)\n";
    constexpr int arrsize = 123;
    std::array<int, arrsize> testcontainer;
    for (int x{0}; x < arrsize; ++x) { testcontainer[x] = x; }
    
    auto dividetest = DivideContainer(testcontainer);
    std::array<std::future<void>, ::THREAD_COUNT> deferredcalls;
    //std::shared_future<void> endf;
    auto nextslot = deferredcalls.rbegin();
    
    for (auto& p: dividetest) {
        auto lambdaprint = [&p]()
        {
            for (auto iter{p.first}; iter != p.second; ++iter) {
                if (*iter < 10)  {std::cout << " ";}
                if (*iter < 100) {std::cout << " ";}
                std::cout << *iter << ", ";
            }
            std::cout << '\n';
            return;
        };
        
        // *nextslot++ = std::async(std::launch::deferred, lambdaprint);
        *nextslot++ = std::async(std::launch::async, lambdaprint);
    }
    //for (const auto& result: deferredcalls) { result.wait(); }
}
