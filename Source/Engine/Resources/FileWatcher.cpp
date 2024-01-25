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
        std::string myPath;
        OVERLAPPED myResult;
        std::vector<FileModif> myTrackedSet;
        std::thread::id myThreadId = std::this_thread::get_id();
        HandlePtr myDirHandle{ INVALID_HANDLE_VALUE };
        HandlePtr myDirCompPort{ INVALID_HANDLE_VALUE };
    };

    void PrintWinErrror()
    {
        const DWORD errorCode = GetLastError();
        LPSTR errString = nullptr;
        const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM;
        FormatMessageA(flags, nullptr, errorCode, LANG_SYSTEM_DEFAULT, (LPSTR)&errString, 0, nullptr);
        std::println("Error({}): {}", errorCode, errString);
        LocalFree(errString);
    }

    std::unique_ptr<State> InitState(std::string_view aPath)
    {
        std::unique_ptr<State> statePtr(new State());
        State& state = *statePtr.get();
        state.myPath = std::string(aPath.data(), aPath.size());
        const DWORD accessFlags = GENERIC_READ;
        const DWORD fileShareFlags = FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE;
        const DWORD extraModes = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
        state.myDirHandle.reset(CreateFileA(aPath.data(), accessFlags, fileShareFlags, nullptr, OPEN_EXISTING, extraModes, nullptr));
        if (state.myDirHandle.get() == INVALID_HANDLE_VALUE)
        {
            PrintWinErrror();
            const std::filesystem::path p(aPath);
            std::println("Failed to open handle to {}", p.string());
            return {};
        }

        state.myDirCompPort.reset(CreateIoCompletionPort(state.myDirHandle.get(), nullptr, 0, 0));
        if (state.myDirCompPort.get() == INVALID_HANDLE_VALUE)
        {
            PrintWinErrror();
            std::println("Failed to open IO completion port!");
            return {};
        }

        const DWORD changeTypes = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME;
        if (!ReadDirectoryChangesExW(state.myDirHandle.get(), 
            state.myBuffer, sizeof(state.myBuffer), 
            TRUE, changeTypes, nullptr,
            &state.myResult, nullptr, ReadDirectoryNotifyExtendedInformation))
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
            for (std::byte* ptr = aState.myBuffer;;)
            {
                // Non-const as we have to in-place convert separators dwon the line
                FILE_NOTIFY_EXTENDED_INFORMATION* info = 
                    reinterpret_cast<FILE_NOTIFY_EXTENDED_INFORMATION*>(ptr);
                // we track renames only to see if destination file changed, 
                // so can ignore the source of rename
                if (info->Action != FILE_ACTION_RENAMED_NEW_NAME
                    && info->Action != FILE_ACTION_MODIFIED)
                {
                    if (!info->NextEntryOffset)
                    {
                        break;
                    }
                    ptr += info->NextEntryOffset;
                    continue;
                }
                wchar_t* beginPtr = info->FileName;
                wchar_t* endPtr = info->FileName + info->FileNameLength / 2;
                std::replace(beginPtr, endPtr, L'\\', L'/');
                const std::wstring_view pathSlice(beginPtr, endPtr);
                std::filesystem::path modifPath(aState.myPath);
                modifPath.append(pathSlice);
                // we only care about files
                if (std::filesystem::is_directory(modifPath))
                {
                    if (!info->NextEntryOffset)
                    {
                        break;
                    }
                    ptr += info->NextEntryOffset;
                    continue;
                }

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
            const DWORD changeTypes = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME;
            if (!ReadDirectoryChangesExW(aState.myDirHandle.get(),
                aState.myBuffer, sizeof(aState.myBuffer),
                TRUE, changeTypes, nullptr,
                &aState.myResult, nullptr, ReadDirectoryNotifyExtendedInformation))
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

FileWatcher::FileWatcher(std::string_view aPath)
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