#ifndef LAB4_RESOURCE_MANAGER_HPP
#define LAB4_RESOURCE_MANAGER_HPP

#include "file_handle.hpp"
#include <memory>
#include <string>
#include <unordered_map>

namespace lab4::resource
{

class ResourceManager
{
  public:
    ResourceManager() = default;

    std::shared_ptr<FileHandle> get_resource(const std::string& filename, const std::string& mode = "r");
    void release(const std::string& filename);
    void cleanup();
    size_t cache_size() const noexcept
    {
        return cache_.size();
    }

  private:
    std::unordered_map<std::string, std::weak_ptr<FileHandle>> cache_;
};

} // namespace lab4::resource
#endif // LAB4_RESOURCE_MANAGER_HPP