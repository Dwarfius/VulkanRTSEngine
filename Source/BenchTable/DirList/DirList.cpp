#include "Precomp.h"

#include <filesystem>
#include <thread>
#include <atomic>
#include <Windows.h>

//constexpr static std::string_view kPath = "D:/Projects"; //51179
constexpr static std::string_view kPath = "D:"; // 515747

void StdScan(benchmark::State& aState)
{
	namespace fs = std::filesystem;
	std::atomic<uint32_t> counter;
	std::string path = kPath.data();
	if (!path.ends_with('/'))
	{
		path += '/';
	}
	for (auto _ : aState)
	{
		counter = 0;

		std::error_code errCode;
		auto options = fs::directory_options::skip_permission_denied;
		auto dirIter = fs::recursive_directory_iterator(path, options, errCode);
		for (const auto& dirEntry : dirIter)
		{
			counter++;
		}
	}
}

bool IsValidDir(const WIN32_FIND_DATA& aFindData)
{
	const bool isDir = aFindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;
	const bool isSystem = aFindData.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM;
	return isDir && !isSystem;
}

void Win32DequeScan(benchmark::State& aState)
{
	std::atomic<uint32_t> counter;
	using Path = std::array<char, MAX_PATH - 2>;
	for (auto _ : aState)
	{
		counter = 0;
		std::queue<Path> pathsToProcess;
		Path root;
		std::copy(kPath.begin(), kPath.end(), root.begin());
		root[kPath.size()] = 0;
		pathsToProcess.push(root);
		while (!pathsToProcess.empty())
		{
			Path currDir = pathsToProcess.front();
			pathsToProcess.pop();

			Path currDirWithWC = currDir;
			std::strcat(currDirWithWC.data(), "/*");

			WIN32_FIND_DATA findData;
			HANDLE findHandle = FindFirstFile(currDirWithWC.data(), &findData);
			do
			{
				if (findData.cFileName[0] == '.' && findData.cFileName[1] == 0
					|| findData.cFileName[0] == '.' && findData.cFileName[1] == '.')
				{
					continue;
				}

				if (IsValidDir(findData))
				{
					Path child;
					std::strcpy(child.data(), currDir.data());
					std::strcat(child.data(), "/");
					std::strcat(child.data(), findData.cFileName);
					ASSERT(strlen(child.data()) < MAX_PATH - 2);
					pathsToProcess.push(child);
				}
				counter++;
			} while (FindNextFile(findHandle, &findData) != 0);
			FindClose(findHandle);
		}
	}
}

void Win32VectorScan(benchmark::State& aState)
{
	std::atomic<uint32_t> counter;
	using Path = std::array<char, MAX_PATH - 2>;
	for (auto _ : aState)
	{
		counter = 0;
		std::vector<Path> pathsToProcess;
		pathsToProcess.reserve(4000);
		Path root;
		std::copy(kPath.begin(), kPath.end(), root.begin());
		root[kPath.size()] = 0;
		pathsToProcess.push_back(root);
		while (!pathsToProcess.empty())
		{
			Path currDir = pathsToProcess.back();
			pathsToProcess.pop_back();

			Path currDirWithWC = currDir;
			std::strcat(currDirWithWC.data(), "/*");

			WIN32_FIND_DATA findData;
			HANDLE findHandle = FindFirstFile(currDirWithWC.data(), &findData);
			do
			{
				if (findData.cFileName[0] == '.' && findData.cFileName[1] == 0
					|| findData.cFileName[0] == '.' && findData.cFileName[1] == '.')
				{
					continue;
				}

				if (IsValidDir(findData))
				{
					Path child;
					std::strcpy(child.data(), currDir.data());
					std::strcat(child.data(), "/");
					std::strcat(child.data(), findData.cFileName);
					ASSERT(strlen(child.data()) < MAX_PATH - 2);
					pathsToProcess.push_back(child);
				}
				counter++;
			} while (FindNextFile(findHandle, &findData) != 0);
			FindClose(findHandle);
		}
	}
}

BENCHMARK(StdScan);
BENCHMARK(Win32DequeScan);
BENCHMARK(Win32VectorScan);

void Win32MTQueueScan(benchmark::State& aState)
{
	std::atomic<uint32_t> counter;
	using Path = std::array<char, MAX_PATH - 2>;

	auto ProcessDir = [](tbb::concurrent_queue<Path>& aPathsToProcess, 
			std::atomic<uint32_t>& aCounter) 
	{
		Path currDir;
		if (!aPathsToProcess.try_pop(currDir)) 
		{ 
			return false; 
		}

		Path currDirWithWC = currDir;
		std::strcat(currDirWithWC.data(), "/*");

		WIN32_FIND_DATA findData;
		HANDLE findHandle = FindFirstFile(currDirWithWC.data(), &findData);
		do
		{
			if (findData.cFileName[0] == '.' && findData.cFileName[1] == 0
				|| findData.cFileName[0] == '.' && findData.cFileName[1] == '.')
			{
				continue;
			}

			if (IsValidDir(findData))
			{
				Path child;
				std::strcpy(child.data(), currDir.data());
				std::strcat(child.data(), "/");
				std::strcat(child.data(), findData.cFileName);
				aPathsToProcess.push(child);
			}
			aCounter++;
		} while (FindNextFile(findHandle, &findData) != 0);
		FindClose(findHandle);
		return true;
	};

	const uint32_t threadCount = std::thread::hardware_concurrency() / 2;
	for (auto _ : aState)
	{
		counter = 0;
		tbb::concurrent_queue<Path> pathsToProcess;

		aState.PauseTiming();
		std::vector<std::atomic<bool>> perThreadDone(threadCount);
		std::vector<std::jthread> threadPool;
		for (uint32_t i = 0; i < threadCount; i++)
		{
			threadPool.push_back(std::jthread{
				[ProcessDir, &pathsToProcess, &counter, &isDone = perThreadDone[i]]
				(std::stop_token aToken) {
					isDone = false;
					while (!aToken.stop_requested())
					{
						isDone = !ProcessDir(pathsToProcess, counter);
					}
				}
			});
		}
		aState.ResumeTiming();

		// load up data
		Path root;
		std::copy(kPath.begin(), kPath.end(), root.begin());
		root[kPath.size()] = 0;
		pathsToProcess.push(root);

		//wait for completion
		while (true)
		{
			bool isDone = !ProcessDir(pathsToProcess, counter);
			for (std::atomic<bool>& flag : perThreadDone)
			{
				isDone &= flag;
			}
			if (isDone)
			{
				break;
			}
		}

		// stopping to exlude waiting for threads to finish
		aState.PauseTiming();
		threadPool.clear();
		aState.ResumeTiming();
	}
}

BENCHMARK(Win32MTQueueScan);