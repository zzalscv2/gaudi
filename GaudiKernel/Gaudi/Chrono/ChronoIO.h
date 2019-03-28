#pragma once

#include <chrono>
#include <iostream>

#if __cplusplus <= 201703L

namespace std {
  namespace chrono {

    /**
     * Helpers to stream duration objects with proper units.
     *
     * This will become obsolete with C++20:
     * https://en.cppreference.com/w/cpp/chrono/duration/operator_ltlt
     */
    namespace io {
      /// Return unit of std::chrono::duration
      template <typename T>
      inline std::string duration_unit( T ) {
        return "";
      }

      // template specializations for units of time
      template <>
      inline std::string duration_unit( std::chrono::nanoseconds ) {
        return "ns";
      }

      template <>
      inline std::string duration_unit( std::chrono::microseconds ) {
        return "us";
      }

      template <>
      inline std::string duration_unit( std::chrono::milliseconds ) {
        return "ms";
      }

      template <>
      inline std::string duration_unit( std::chrono::seconds ) {
        return "s";
      }

      template <>
      inline std::string duration_unit( std::chrono::minutes ) {
        return "m";
      }

      template <>
      inline std::string duration_unit( std::chrono::hours ) {
        return "h";
      }
    } // namespace io

    /// stream operator for std::chrono::duration including unit
    template <class Rep, class Period>
    std::ostream& operator<<( std::ostream& os, const duration<Rep, Period>& d ) {
      os << d.count() << io::duration_unit( d );
      return os;
    }
  } // namespace chrono
} // namespace std

#endif // C++17
