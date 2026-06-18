#include "dusk/modding/shared_library.hpp"

#include "dusk/io.hpp"

#if defined(_WIN32)
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace dusk::modding {

SharedLibrary::SharedLibrary(SharedLibrary&& other) noexcept : mHandle(other.mHandle) {
    other.mHandle = nullptr;
}

SharedLibrary& SharedLibrary::operator=(SharedLibrary&& other) noexcept {
    if (this != &other) {
        close();
        mHandle = other.mHandle;
        other.mHandle = nullptr;
    }
    return *this;
}

SharedLibrary::~SharedLibrary() {
    close();
}

bool SharedLibrary::open(const std::filesystem::path& path, std::string* error) {
    close();

#if defined(_WIN32)
    mHandle = static_cast<void*>(LoadLibraryW(path.c_str()));
    if (!mHandle) {
        if (error) {
            *error = "LoadLibraryW failed";
        }
        return false;
    }
#else
    const auto pathString = io::fs_path_to_string(path);
    mHandle = dlopen(pathString.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!mHandle) {
        if (error) {
            const char* msg = dlerror();
            *error = msg ? msg : "dlopen failed";
        }
        return false;
    }
#endif

    return true;
}

void* SharedLibrary::symbol(const char* name, std::string* error) const {
    if (!mHandle) {
        if (error) {
            *error = "library is not open";
        }
        return nullptr;
    }

#if defined(_WIN32)
    void* ptr = reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(mHandle), name));
    if (!ptr && error) {
        *error = "GetProcAddress failed";
    }
    return ptr;
#else
    dlerror();
    void* ptr = dlsym(mHandle, name);
    const char* msg = dlerror();
    if (msg != nullptr) {
        if (error) {
            *error = msg;
        }
        return nullptr;
    }
    return ptr;
#endif
}

void SharedLibrary::close() {
    if (!mHandle) {
        return;
    }

#if defined(_WIN32)
    FreeLibrary(static_cast<HMODULE>(mHandle));
#else
    dlclose(mHandle);
#endif
    mHandle = nullptr;
}

bool SharedLibrary::is_open() const {
    return mHandle != nullptr;
}

}  // namespace dusk::modding

