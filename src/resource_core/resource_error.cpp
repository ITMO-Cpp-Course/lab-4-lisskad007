#include "resource_error.hpp"

namespace lab4::resource
{

ResourceError::ResourceError(const std::string& message) : std::runtime_error(message) {}

ResourceError::ResourceError(const char* message) : std::runtime_error(message) {}

} // namespace lab4::resource