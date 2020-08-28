#pragma once

namespace memory {

/// Force a read from memory location @addr even if the result is not used.
template <typename T>
inline T force_read(volatile T *addr) {
  return *addr;
}

}  // namespace memory
