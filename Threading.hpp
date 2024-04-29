#ifndef FLUIDSIM_THREADING_INCLUDED
#define FLUIDSIM_THREADING_INCLUDED

#include <thread>
#include <array>
#include <tuple> // std::pair


static constexpr int THREAD_COUNT {4};

class ThreadManager
{
    std::array<std::jthread, THREAD_COUNT> threads;
    int nextavailable {0};
    
    public:
    const unsigned int reportedThreadcount;
    void PrintThreadcount();
    void ContainerDivTest(); // iterates over the result of DivideContainer
    void ContainerDivTestMT();
    
    template<typename FunctionT, typename... Args>
    void LaunchThread(FunctionT&& function, Args&&... args)
    {
        std::jthread& task = threads[nextavailable++];
        task = std::jthread(function, args...);
        //task.join();
        
        return;
    }
    
    ThreadManager(): reportedThreadcount{std::thread::hardware_concurrency()}
    {
        
    }
};


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
