#include "file_handle.hpp"
#include "resource_error.hpp"
#include <cstring>

namespace lab4::resource
{

FileHandle::FileHandle() noexcept : file_(nullptr) {}

FileHandle::FileHandle(const std::string& filename, const std::string& mode) : file_(nullptr), filename_(filename)
{
    open(filename, mode);
}

FileHandle::FileHandle(FileHandle&& other) noexcept : file_(other.file_), filename_(std::move(other.filename_))
{
    other.file_ = nullptr;
}

FileHandle& FileHandle::operator=(FileHandle&& other) noexcept
{
    if (this != &other)
    {
        close();
        file_ = other.file_;
        filename_ = std::move(other.filename_);
        other.file_ = nullptr;
    }
    return *this;
}

FileHandle::~FileHandle()
{
    close();
}

void FileHandle::open(const std::string& filename, const std::string& mode)
{
    if (is_open())
    {
        close();
    }
    file_ = std::fopen(filename.c_str(), mode.c_str());
    if (!file_)
    {
        throw ResourceError("Failed to open file: " + filename);
    }
    filename_ = filename;
}

void FileHandle::close()
{
    if (file_)
    {
        std::fclose(file_);
        file_ = nullptr;
    }
}

bool FileHandle::is_open() const noexcept
{
    return file_ != nullptr;
}

FILE* FileHandle::get() const noexcept
{
    return file_;
}

std::string FileHandle::read_line()
{
    if (!is_open())
    {
        throw ResourceError("Cannot read from closed file: " + filename_);
    }
    char buffer[1024];
    if (std::fgets(buffer, sizeof(buffer), file_))
    {
        buffer[std::strcspn(buffer, "\n")] = '\0';
        return std::string(buffer);
    }
    if (std::feof(file_))
    {
        return "";
    }
    throw ResourceError("Error reading from file: " + filename_);
}

} // namespace lab4::resource