#pragma once
#include <euphrosyne/pch.hpp>

namespace euphrosyne
{

class SdlException : public exception
{
    string message_;
public:
    SdlException();
    explicit SdlException(string_view message);

    const char *what() const noexcept override;
};

} // namespace euphrosyne

