#pragma once

// A singleton class that manages the collection of profiling data in
// profiling Marks, which contains timestamps of start-end, name and thread ids
class Profiler
{
	using Clock = std::chrono::high_resolution_clock;
	using Stamp = uint64_t;
    constexpr static uint32_t kMaxFrames = 10;
    constexpr static uint32_t kInitFrames = 5;
public:
    class ScopedMark;
    struct FrameProfile;
    using LongFrameCallback = std::function<void(const FrameProfile& aProfile)>;

	// A profiling mark, a record of how long it took to execute a named activity
    // Also keeps track of it's parent via IDs
    struct Mark
	{
        char myName[64]; // TODO: move this out
        std::thread::id myThreadId; // TODO: look at moving it out
		Stamp myStamp;
		uint32_t myId;
		uint8_t myDepth;
	};
    static_assert(std::is_trivially_copyable_v<Mark>, "Relying on fast copies!");
    static_assert(std::is_trivially_destructible_v<Mark>, "Relying on noop destructors!");

    // A record of a frame's profiling data - how long the entire frame took,
    // what's the frame counter, and all of the marks of that frame
    struct FrameProfile
    {
        Stamp myBeginStamp;
        Stamp myEndStamp;
        size_t myFrameNum;
        std::vector<Mark> myStartMarks;
        std::vector<Mark> myEndMarks;
    };

    static Profiler& GetInstance()
    {
        static Profiler profiler;
        return profiler;
    }

    // Call to start tracking new Marks for new frame
    void NewFrame();
    void SetOnLongFrameCallback(const LongFrameCallback& aCallback) { myOnLongFrameCB = aCallback; }

    const std::array<FrameProfile, kMaxFrames>& GetBufferedFrameData() const { return myFrameProfiles;  }
    const std::array<FrameProfile, kInitFrames>& GrabInitFrame() const { return myInitFrames; };

private:
    class Storage;
    // Returns a thread-local storage for accumulating Marks for current frame
    static Storage& GetStorage(Profiler& aProfiler);

    // Initializes and automatically prepares for recording into the new frame
    Profiler();

    void AddStorage(Storage* aStorage)
    {
        // we use a spin lock because we try to avoid allocations
        // by reserving enough to support hardware_concurrency.
        // it's not guaranteed, but should be enough for a start
        tbb::spin_mutex::scoped_lock lock(myStorageMutex);
        myTLSStorages.push_back(aStorage);
    }
    std::atomic<uint32_t> myIdCounter = 0;
    std::vector<Storage*> myTLSStorages;
    tbb::concurrent_queue<Mark> myStartMarks;
    tbb::concurrent_queue<Mark> myEndMarks;
    std::array<FrameProfile, kMaxFrames> myFrameProfiles;
    std::array<FrameProfile, kInitFrames> myInitFrames;
    LongFrameCallback myOnLongFrameCB;
    size_t myFrameNum = 0;
    tbb::spin_mutex myStorageMutex;
};

class Profiler::Storage
{
public:
    Storage(Profiler& aProfiler);

    uint32_t StartScope(std::string_view aName);
    void EndScope(uint32_t anId, std::string_view aName);
private:
    uint8_t myDepth = 0;
    tbb::concurrent_queue<Mark>& myStartMarks;
    tbb::concurrent_queue<Mark>& myEndMarks;
    std::atomic<uint32_t>& myIdCounter;
};

// RAII style profiling mark - starts a mark on ctor, stops it on dtor
class Profiler::ScopedMark
{
    Profiler& myProfiler;
    std::string_view myName;
    uint32_t myId;
public:
    ScopedMark(std::string_view aName, Profiler& aProfiler = Profiler::GetInstance())
        : myProfiler(aProfiler)
        , myName(aName)
    {
        myId = GetStorage(myProfiler).StartScope(aName);
    }

    ~ScopedMark()
    {
        GetStorage(myProfiler).EndScope(myId, myName);
    }
};