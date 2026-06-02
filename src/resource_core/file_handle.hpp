#ifndef LAB4_FILE_HANDLE_HPP
#define LAB4_FILE_HANDLE_HPP

#include <cstdio>
#include <string>

namespace lab4::resource
{

class FileHandle
{
  public:
    FileHandle() noexcept;
    explicit FileHandle(const std::string& filename, const std::string& mode = "r");

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    FileHandle(FileHandle&& other) noexcept;
    FileHandle& operator=(FileHandle&& other) noexcept;

    ~FileHandle();

    void open(const std::string& filename, const std::string& mode = "r");
    void close();
    bool is_open() const noexcept;
    FILE* get() const noexcept;
    std::string read_line();
    std::string get_filename() const noexcept
    {
        return filename_;
    }

  private:
    FILE* file_;
    std::string filename_;
};

} // namespace lab4::resource
#endif // LAB4_FILE_HANDLE_HPP