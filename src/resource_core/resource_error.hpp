#ifndef LAB4_RESOURCE_ERROR_HPP
#define LAB4_RESOURCE_ERROR_HPP

#include <stdexcept>
#include <string>

namespace lab4::resource
{

class ResourceError : public std::runtime_error
{
  public:
    explicit ResourceError(const std::string& message);
    explicit ResourceError(const char* message);
};

} // namespace lab4::resource
#endif // LAB4_RESOURCE_ERROR_HPP