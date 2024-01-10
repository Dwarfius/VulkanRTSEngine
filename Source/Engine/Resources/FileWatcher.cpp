#include "Precomp.h"
#include "FileWatcher.h"

// TODO: move to Precomp
#include <filesystem>

#ifdef WIN32
namespace Win32
{
    struct FileModif
    {
        std::string myPath;
        std::chrono::sys_time<std::chrono::milliseconds> myTimestamp;

        bool operator==(const FileModif& aOther) const
        {
            return myPath == aOther.myPath;
        }
    };

    struct State : FileWatcher::OSState
    {
        using HandleDeleter = decltype([](HANDLE aHandle)
        {
            if (aHandle != INVALID_HANDLE_VALUE)
            {
                CloseHandle(aHandle);
            }
        });
        using HandlePtr = std::unique_ptr<std::remove_pointer_t<HANDLE>, HandleDeleter>;

        // 64kb max, ReadDirectoryChangesW requires DWORD alignment
        alignas(DWORD) std::byte myBuffer[64 * 1024]{}; 
        HandlePtr myDirHandle{ INVALID_HANDLE_VALUE };
        HandlePtr myDirCompPort{ INVALID_HANDLE_VALUE };
        OVERLAPPED myResult;
        std::vector<FileModif> myTrackedSet;
        std::thread::id myThreadId = std::this_thread::get_id();
    };

    void PrintWinErrror()
    {
        DWORD errorCode = GetLastError();
        LPSTR errString = nullptr;
        FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, NULL, errorCode, LANG_SYSTEM_DEFAULT, (LPSTR)&errString, 0, NULL);
        std::println("Error({}): {}", errorCode, errString);
        LocalFree(errString);
    }

    std::unique_ptr<State> InitState(std::wstring_view aPath)
    {
        std::unique_ptr<State> statePtr(new State());
        State& state = *statePtr.get();
        state.myDirHandle.reset(CreateFileW(aPath.data(), FILE_LIST_DIRECTORY, FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL));
        if (state.myDirHandle.get() == INVALID_HANDLE_VALUE)
        {
            PrintWinErrror();
            std::filesystem::path p(aPath);
            std::println("Failed to open handle to {}", p.string());
            return {};
        }

        state.myDirCompPort.reset(CreateIoCompletionPort(state.myDirHandle.get(), NULL, NULL, 0));
        if (state.myDirCompPort.get() == INVALID_HANDLE_VALUE)
        {
            PrintWinErrror();
            std::println("Failed to open IO completion port!");
            return {};
        }

        if (!ReadDirectoryChangesExW(state.myDirHandle.get(), 
            state.myBuffer, sizeof(state.myBuffer), 
            TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE, NULL, 
            &state.myResult, NULL, ReadDirectoryNotifyExtendedInformation))
        {
            PrintWinErrror();
            std::println("Failed to queue up directory changes call!");
            return {};
        }

        return statePtr;
    }

    void CheckFiles(State& aState, std::vector<std::string>& aDetected)
    {
        using namespace std::chrono_literals;
        constexpr std::chrono::duration kDelay = 100ms;

        ASSERT_STR(aState.myThreadId == std::this_thread::get_id(),
            "FileWatcher is running on a different thread that it was constructed, "
            "this is not supported by ReadDirectoryChangesExW!"
        );

        DWORD bytesCount;
        ULONG_PTR completionKey;
        LPOVERLAPPED resultPtr = nullptr;

        aDetected.clear();
        while (GetQueuedCompletionStatus(aState.myDirCompPort.get(), &bytesCount, &completionKey, &resultPtr, 0))
        {
            for (const std::byte* ptr = aState.myBuffer;;)
            {
                const FILE_NOTIFY_EXTENDED_INFORMATION* info = reinterpret_cast<const FILE_NOTIFY_EXTENDED_INFORMATION*>(ptr);
                const std::wstring_view pathSlice(info->FileName, info->FileName + info->FileNameLength / 2);
                const std::filesystem::path modifPath(pathSlice);

                // https://learn.microsoft.com/en-us/windows/win32/api/winternl/nf-winternl-ntquerysystemtime
                // ...This is a 64-bit value representing the number of 100-nanosecond intervals since January 1, 1601 (UTC).
                const long long unixStamp = std::bit_cast<long long>(info->LastModificationTime.u);
                using HundrenthNS = std::ratio<1, 10000000>;
                namespace chr = std::chrono;
                const chr::duration<long long, HundrenthNS> modifTime(unixStamp);
                const chr::milliseconds modifTimeMs = chr::duration_cast<chr::milliseconds>(modifTime);
                const chr::file_time<chr::milliseconds> timePoint(modifTimeMs);

                FileModif modif{ modifPath.string(), chr::clock_cast<chr::system_clock>(timePoint) };
                // Note - can produce "duplicate" notifications, so need to filter those out!
                auto iter = std::find(aState.myTrackedSet.begin(), aState.myTrackedSet.end(), modif);
                if (iter == aState.myTrackedSet.end())
                {
                    aDetected.emplace_back(modif.myPath);
                    aState.myTrackedSet.emplace_back(std::move(modif));
                }

                if (!info->NextEntryOffset)
                {
                    break;
                }
                ptr += info->NextEntryOffset;
            }

            // reque our notification waiter
            if (!ReadDirectoryChangesExW(aState.myDirHandle.get(),
                aState.myBuffer, sizeof(aState.myBuffer),
                TRUE, FILE_NOTIFY_CHANGE_LAST_WRITE, NULL,
                &aState.myResult, NULL, ReadDirectoryNotifyExtendedInformation))
            {
                PrintWinErrror();
            }
        }

        std::erase_if(aState.myTrackedSet, [](const FileModif& aFile) {
            auto diff = std::chrono::system_clock::now() - aFile.myTimestamp;
            return diff > kDelay;
        });
    }
}
#endif // WIN32

FileWatcher::FileWatcher(std::wstring_view aPath)
{
#ifdef WIN32
    namespace Platform = Win32;
#endif
    myState = Platform::InitState(aPath);
}

void FileWatcher::CheckFiles()
{
#ifdef WIN32
    namespace Platform = Win32;
#endif
    using State = Platform::State;
    Platform::CheckFiles(*static_cast<State*>(myState.get()), myModifs);
}