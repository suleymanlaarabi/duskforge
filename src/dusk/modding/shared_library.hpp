#pragma once

#include <filesystem>
#include <string>

namespace dusk::modding {

class SharedLibrary {
public:
    SharedLibrary() = default;
    SharedLibrary(const SharedLibrary&) = delete;
    SharedLibrary& operator=(const SharedLibrary&) = delete;
    SharedLibrary(SharedLibrary&& other) noexcept;
    SharedLibrary& operator=(SharedLibrary&& other) noexcept;
    ~SharedLibrary();

    bool open(const std::filesystem::path& path, std::string* error);
    void* symbol(const char* name, std::string* error) const;
    void close();
    bool is_open() const;

private:
    void* mHandle = nullptr;
};

}  // namespace dusk::modding

