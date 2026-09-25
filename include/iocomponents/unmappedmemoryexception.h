#ifndef UNMAPPEDMEMORYEXCEPTION_H
#define UNMAPPEDMEMORYEXCEPTION_H

#include <exception>
#include <string>
#include "../globals.h"

class UnmappedMemoryException : public std::exception
{
public:
  explicit UnmappedMemoryException(word address);
  const char* what() const noexcept override;
  word Address() const noexcept;
private:
  word m_address;
  std::string m_message;
};

#endif /* UNMAPPEDMEMORYEXCEPTION_H */
