#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace minitub {

enum class ExceptionType {
  Invalid,         // invalid argument or state
  OutOfRange,      // index or value out of range
  NotImplemented,  // feature not built yet
  Config,          // bad EngineConfig input
  Io,              // file or disk I/O failure
};

constexpr auto ExceptionTypeToString(ExceptionType type) -> std::string_view {
  switch (type) {
    case ExceptionType::Invalid:
      return "Invalid";
    case ExceptionType::OutOfRange:
      return "OutOfRange";
    case ExceptionType::NotImplemented:
      return "NotImplemented";
    case ExceptionType::Config:
      return "Config";
    case ExceptionType::Io:
      return "Io";
  }
  return "Unknown";
}

/** Base exception for MiniTub. what() reads "<Type>: <message>". */
class Exception : public std::runtime_error {
 public:
  explicit Exception(const std::string &message) : Exception(ExceptionType::Invalid, message) {}
  Exception(ExceptionType type, const std::string &message)
      : std::runtime_error(std::string(ExceptionTypeToString(type)) + ": " + message), type_(type) {}

  auto GetType() const noexcept -> ExceptionType { return type_; }

 private:
  ExceptionType type_;
};

}  // namespace minitub
