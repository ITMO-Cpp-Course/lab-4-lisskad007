#include "resource_manager.hpp"
#include "resource_error.hpp"

namespace lab4::resource
{

std::shared_ptr<FileHandle> ResourceManager::get_resource(const std::string& filename, const std::string& mode)
{
    auto it = cache_.find(filename);
    if (it != cache_.end())
    {
        if (auto locked = it->second.lock())
        {
            return locked;
        }
        else
        {
            cache_.erase(it);
        }
    }

    auto handle = std::make_shared<FileHandle>(filename, mode);
    cache_[filename] = handle;
    return handle;
}

void ResourceManager::release(const std::string& filename)
{
    auto it = cache_.find(filename);
    if (it != cache_.end())
    {
        if (it->second.expired())
        {
            cache_.erase(it);
        }
        else
        {
            throw ResourceError("Cannot release resource '" + filename + "': still in use");
        }
    }
}

void ResourceManager::cleanup()
{
    for (auto it = cache_.begin(); it != cache_.end();)
    {
        if (it->second.expired())
        {
            it = cache_.erase(it);
        }
        else
        {
            ++it;
        }
    }
}

} // namespace lab4::resource