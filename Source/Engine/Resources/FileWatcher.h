#pragma once

class FileWatcher
{
public:
    struct OSState { virtual ~OSState() = default; };

    FileWatcher(std::string_view aPath);

    void CheckFiles();

    std::span<const std::string> GetModifs() const { return myModifs; }

private:
    std::vector<std::string> myModifs;
    std::unique_ptr<OSState> myState;
};