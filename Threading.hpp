#ifndef FLUIDSIM_THREADING_INCLUDED
#define FLUIDSIM_THREADING_INCLUDED

#include <thread>
#include <future>
#include <array>
#include <tuple> // std::pair


static constexpr int THREAD_COUNT {8};

class ThreadManager
{
    const unsigned int THREAD_COUNT; // count reported by hardware
    // two problems with using this:
    // 1) not constexpr; can't be used to size arrays
    // 2) object instance is not globally defined
    
    public:
    void PrintThreadcount();
    void ContainerDivTest(); // iterates over the result of DivideContainer
    void ContainerDivTestMT();
    
    ThreadManager(): THREAD_COUNT{std::thread::hardware_concurrency()}
    {
        
    }
};


// overload for containers that don't have '+' for their iterators
// (required for DivideContainer start/end)
auto operator+(auto map_iter, auto offset) {
    while (offset > 0) { ++map_iter; --offset; }
    return map_iter;
}


template<typename T>
auto DivideContainer(T& container)
{
    using start_type = decltype(container.begin());
    using end_type   = decltype(container.end());
    
    std::array<std::pair<start_type, end_type>, THREAD_COUNT> segments;
    const auto stride = container.size() / THREAD_COUNT;
    
    for (int C{0}; C < THREAD_COUNT; ++C) {
        auto start  = container.begin() + stride*C;
        auto end    = (C == THREAD_COUNT-1)? container.end() : (start + stride);
        segments[C] = std::make_pair(start, end);
    }
    return segments;
}


template<typename T>
auto DivideContainer(const T& container) // for const containers
{
    using start_type = decltype(container.begin());
    using end_type   = decltype(container.end());
    
    std::array<std::pair<start_type, end_type>, THREAD_COUNT> segments;
    const auto stride = container.size() / THREAD_COUNT;
    
    for (int C{0}; C < THREAD_COUNT; ++C) {
        auto start  = container.begin() + stride*C;
        auto end    = (C == THREAD_COUNT-1)? container.end() : (start + stride);
        segments[C] = std::make_pair(start, end);
    }
    return segments;
}


#endif
