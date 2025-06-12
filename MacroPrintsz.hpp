#include <iostream>
#include <cxxabi.h> // demangling

// 'friend int main(int, char**);' may need to be added to: Fluid, Fluid::Particle, etc

template<typename T, typename...Rs>
void PrintTypeSize()
{
    int status; char* realname;
    realname = abi::__cxa_demangle(typeid(T).name(), NULL, NULL, &status);
    if (status) { std::cerr << std::format("demangle failed! ({}): '{}'\n", status, typeid(T).name()); return; }
    std::cout << std::format("  {}: \t{}\n", realname, sizeof(T));
    if constexpr (sizeof...(Rs)) PrintTypeSize<Rs...>();
}

template<typename...Ts>
void PrintTypeSizes()
{
    std::cout << "\n\nType-sizes: \n";
    PrintTypeSize<Ts...>();
    std::cout << "\n\n";
}

/* 
// 'using' CellMatrix/CellArray from DiffusionField
#define CellMatrix std::array<std::array<Cell*, Cell::arraySizeY>, Cell::arraySizeX>
#define CellArray std::vector<Cell>

    PrintTypeSizes<
        Fluid, Fluid::Particle,
        DiffusionField, CellMatrix, CellArray,
        LocalCells<0>, Cell, CellDelta_T, DeltaMap, DoubleCoord, CoordBase_T,
        Simulation, MainGUI, MainGUI::SimulParameters, Mouse_T, ThreadManager,
        sf::CircleShape, sf::RectangleShape, sf::RenderTexture, sf::RenderWindow
    >();

    PrintTypeSizes<
        Cell, Fluid, Fluid::Particle,
        DiffusionField, Simulation, ThreadManager,
        Mouse_T, MainGUI, MainGUI::SimulParameters,
        sf::CircleShape, sf::RectangleShape, sf::RenderTexture, sf::RenderWindow
    >();

    // sorted by length (indent of output)
    PrintTypeSizes<
        Simulation, Cell, Fluid, Mouse_T, MainGUI, 
        Fluid::Particle, DiffusionField, ThreadManager, MainGUI::SimulParameters,
        sf::CircleShape, sf::RectangleShape, sf::RenderTexture, sf::RenderWindow
    >();

 */


// ------------------------------------------------------------------------ //
// cache_size testing

//#define CACHE_TEST_ENABLED
#ifdef CACHE_TEST_ENABLED

/*
 This test uses two threads that atomically write to the data members of the given global objects.
 The first object fits in one cache line, which results in "hardware interference".
 The second object keeps its data members on separate cache lines, so possible "cache synchronization" after thread writes is avoided.
*/

#include <atomic>
#include <chrono>
#include <cstddef>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <new>
#include <thread>


/*#ifdef __cpp_lib_hardware_interference_size
    using std::hardware_constructive_interference_size;
    using std::hardware_destructive_interference_size;
#else
    // 64 bytes on x86-64 │ L1_CACHE_BYTES │ L1_CACHE_SHIFT │ __cacheline_aligned │ ...
    constexpr std::size_t hardware_constructive_interference_size = 64;
    constexpr std::size_t hardware_destructive_interference_size = 64;
#endif*/

// always 64; GCC warns about use of the 'std::' value (-Winterference-size)
constexpr std::size_t hardware_constructive_interference_size = 64;
constexpr std::size_t hardware_destructive_interference_size = 64;

std::mutex cout_mutex;

constexpr int max_write_iterations{10'000'000}; // the benchmark time tuning

struct alignas(hardware_constructive_interference_size)
OneCacheLiner // occupies one cache line
{
    std::atomic_uint64_t x{};
    std::atomic_uint64_t y{};
} oneCacheLiner;

struct TwoCacheLiner // occupies two cache lines
{
    alignas(hardware_destructive_interference_size) std::atomic_uint64_t x{};
    alignas(hardware_destructive_interference_size) std::atomic_uint64_t y{};
} twoCacheLiner;

inline auto now() noexcept { return std::chrono::high_resolution_clock::now(); }

template<bool xy>
void oneCacheLinerThread()
{
    const auto start{now()};
    for (uint64_t count{}; count != max_write_iterations; ++count)
        if constexpr (xy) oneCacheLiner.x.fetch_add(1, std::memory_order_relaxed);
        else              oneCacheLiner.y.fetch_add(1, std::memory_order_relaxed);

    const std::chrono::duration<double, std::milli> elapsed{now() - start};
    std::lock_guard lk{cout_mutex};
    std::cout << "oneCacheLinerThread() spent " << elapsed.count() << " ms\n";
    if constexpr (xy) oneCacheLiner.x = elapsed.count();
    else              oneCacheLiner.y = elapsed.count();
}

template<bool xy>
void twoCacheLinerThread()
{
    const auto start{now()};
    for (uint64_t count{}; count != max_write_iterations; ++count)
        if constexpr (xy) twoCacheLiner.x.fetch_add(1, std::memory_order_relaxed);
        else              twoCacheLiner.y.fetch_add(1, std::memory_order_relaxed);

    const std::chrono::duration<double, std::milli> elapsed{now() - start};
    std::lock_guard lk{cout_mutex};
    std::cout << "twoCacheLinerThread() spent " << elapsed.count() << " ms\n";
    if constexpr (xy) twoCacheLiner.x = elapsed.count();
    else              twoCacheLiner.y = elapsed.count();
}

void TestCaches()
{
    std::cout << "__cpp_lib_hardware_interference_size "
    #ifdef __cpp_lib_hardware_interference_size
        "= " << __cpp_lib_hardware_interference_size << '\n';
    #else
        "is not defined, use " << hardware_destructive_interference_size
                               << " as fallback\n";
    #endif

    std::cout << "hardware_destructive_interference_size == "
              << hardware_destructive_interference_size << '\n'
              << "hardware_constructive_interference_size == "
              << hardware_constructive_interference_size << "\n\n"
              << std::fixed << std::setprecision(2)
              << "sizeof( OneCacheLiner ) == " << sizeof(OneCacheLiner) << '\n'
              << "sizeof( TwoCacheLiner ) == " << sizeof(TwoCacheLiner) << "\n\n";

    constexpr int max_runs{4};
    int oneCacheLiner_average{0};
    for (auto i{0}; i != max_runs; ++i)
    {
        std::thread th1{oneCacheLinerThread<0>};
        std::thread th2{oneCacheLinerThread<1>};
        th1.join();
        th2.join();
        oneCacheLiner_average += oneCacheLiner.x + oneCacheLiner.y;
    }
    std::cout << "Average T1 time: "
              << (oneCacheLiner_average / max_runs / 2) << " ms\n\n";

    int twoCacheLiner_average{0};
    for (auto i{0}; i != max_runs; ++i)
    {
        std::thread th1{twoCacheLinerThread<0>};
        std::thread th2{twoCacheLinerThread<1>};
        th1.join();
        th2.join();
        twoCacheLiner_average += twoCacheLiner.x + twoCacheLiner.y;
    }
    std::cout << "Average T2 time: "
              << (twoCacheLiner_average / max_runs / 2) << " ms\n\n"
              << "Ratio T1/T2:~ "
              << 1.0 * oneCacheLiner_average / twoCacheLiner_average << '\n';
    
    return;
}

#endif
