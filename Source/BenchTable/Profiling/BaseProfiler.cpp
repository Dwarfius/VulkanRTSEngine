#include "Precomp.h"
#include <thread>
#include <mutex>
#include <chrono>
#include <cstring>
#include <atomic>

class ProfileManager
{
    // TODO: try QPC wrapper
    using Clock = std::chrono::high_resolution_clock;
    using Stamp = Clock::time_point;
    friend class ScopedMark;
public:
    // TLS storage type for a single profiling event
    struct Mark
    {
        Stamp myBeginStamp;
        Stamp myEndStamp;
        char myName[64];
        int myId;
        int myParentId;
        std::thread::id myThreadId;
    };

    class Storage
    {
    public:
        Storage(std::atomic<int>& aGlobalCounter)
            : myIdCounter(aGlobalCounter)
        {
            myMarks.reserve(256);
            NewFrame();
        }

        void BeginMark(const std::string_view& aName);
        void EndMark();
        void NewFrame()
        {
            myActiveMarkInd = -1;
            myMarks.clear();
        }
        void AppendMarks(std::vector<Mark>& aBuffer) const;
    private:
        std::vector<Mark> myMarks;
        std::atomic<int>& myIdCounter;
        int myActiveMarkInd;
    };

    static ProfileManager& GetInstance()
    {
        static ProfileManager manager;
        return manager;
    }

    class ScopedMark
    {
    public:
        ScopedMark(const std::string_view& aName)
        {
            GetStorage().BeginMark(aName);
        }

        ~ScopedMark()
        {
            GetStorage().EndMark();
        }
    };

    // need to quick-flush the TLS - reset all counters
    void BeginFrame();
    // need to accumulate all TLS into a single buffer
    void EndFrame();

private:
    static Storage& GetStorage()
    {
        static thread_local Storage threadStorage = [] {
            ProfileManager& manager = GetInstance();
            Storage storage(manager.myIdCounter);
            manager.AddStorage(&threadStorage);
            return storage;
        }();
        return threadStorage;
    }

    void AddStorage(Storage* aStorage)
    {
        std::lock_guard lock(myStorageMutex);
        myTLSStorages.push_back(aStorage);
    }
    std::atomic<int> myIdCounter = 0;
    std::vector<Storage*> myTLSStorages;
    std::mutex myStorageMutex;
};

void ProfileManager::Storage::BeginMark(const std::string_view& aName)
{
    Mark newMark;
    newMark.myBeginStamp = Clock::now();
    std::memcpy(newMark.myName, aName.data(), aName.size());
    newMark.myName[aName.size()] = 0;
    newMark.myId = myIdCounter++;
    // temporarily we're going to store local index of the parent
    // instead of the global ID, it will be replaced in EndMark
    // This is done to simplify parent lookup for Begin-/EndMark
    newMark.myParentId = myActiveMarkInd;
    newMark.myThreadId = std::this_thread::get_id();
    myMarks.push_back(std::move(newMark));
    myActiveMarkInd = myMarks.size() - 1;
}

void ProfileManager::Storage::EndMark()
{
    Mark& lastMark = myMarks[myActiveMarkInd];
    // close up the mark
    lastMark.myEndStamp = Clock::now();
    lastMark.myParentId = (myActiveMarkInd != -1) ? myMarks[myActiveMarkInd].myId : -1;
    // point at parent for active mark scope
    myActiveMarkInd = lastMark.myParentId;
}

void ProfileManager::Storage::AppendMarks(std::vector<Mark>& aBuffer) const
{
    aBuffer.insert(aBuffer.end(), myMarks.begin(), myMarks.end());
}

// need to quick-flush the TLS - reset all counters
void ProfileManager::BeginFrame()
{
    myIdCounter = 0;
    for (Storage* storage : myTLSStorages)
    {
        storage->NewFrame();
    }
}
// need to accumulate all TLS into a single buffer
void ProfileManager::EndFrame()
{
    std::vector<Mark> allMarks;
    for (Storage* storage : myTLSStorages)
    {
        storage->AppendMarks(allMarks);
    }

    const Mark& lastMark = allMarks.back();
    std::cout << "End Frame, last capture:" << std::endl;
    std::cout << "\tName: " << lastMark.myName << std::endl;
    std::cout << "\tId: " << lastMark.myId << std::endl;
    std::cout << "\tParentId: " << lastMark.myParentId << std::endl;
    std::cout << "\tDuration: " << (lastMark.myEndStamp - lastMark.myBeginStamp).count() << std::endl;
}

void Mark(benchmark::State& state)
{
    ProfileManager::GetInstance().BeginFrame();
    for (auto _ : state)
    {
        ProfileManager::ScopedMark mark(__func__);
    }
    ProfileManager::GetInstance().EndFrame();
}

void Add(benchmark::State& state)
{
    ProfileManager::GetInstance().BeginFrame();
    int i = 0;
    for (auto _ : state)
    {
        ProfileManager::ScopedMark mark(__func__);
        benchmark::DoNotOptimize(i++);
    }
    ProfileManager::GetInstance().EndFrame();
}

void Strings(benchmark::State& state)
{
    ProfileManager::GetInstance().BeginFrame();
    for (auto _ : state)
    {
        ProfileManager::ScopedMark mark(__func__);
        std::string var("");
        benchmark::DoNotOptimize(var.data());
        var = "123456789";
        benchmark::ClobberMemory();
    }
    ProfileManager::GetInstance().EndFrame();
}

void ForLoop(benchmark::State& state)
{
    ProfileManager::GetInstance().BeginFrame();
    for (auto _ : state)
    {
        ProfileManager::ScopedMark mark(__func__);
        for (int i = 0; i < 100; i++)
        {
            std::string var("");
            benchmark::DoNotOptimize(var.data());
            var = "123456789";
            benchmark::ClobberMemory();
        }
    }
    ProfileManager::GetInstance().EndFrame();
}

BENCHMARK(Mark);
BENCHMARK(Add);
BENCHMARK(Strings);
BENCHMARK(ForLoop);