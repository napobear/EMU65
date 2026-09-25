#include "../../include/iocomponents/unmappedmemoryexception.h"
#include <sstream>
#include <iomanip>

UnmappedMemoryException::UnmappedMemoryException(word address) : m_address(address)
{
    std::ostringstream oss;
    oss << "The virtual memory address 0x" << std::hex << std::uppercase << std::setw(4)
        << std::setfill('0') << static_cast<unsigned int>(address)
        << " cannot be accessed because it is unmapped.";
    m_message = oss.str();
}

const char* UnmappedMemoryException::what() const noexcept
{
    return m_message.c_str();
}

word UnmappedMemoryException::Address() const noexcept
{
    return m_address;
}
