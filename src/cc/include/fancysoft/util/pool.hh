#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>

namespace Fancysoft {
namespace Util {

template <class T> class Pool {
  std::deque<std::unique_ptr<T>> _queue;

  /// The maximum allowed amount of created objects.
  size_t _max_size;

  /// Tracks the total amount of created objects.
  size_t _current_size = 0;

  std::function<T()> _factory;

  std::mutex _mutex;
  std::condition_variable _condvar;

public:
  Pool(std::function<T()> factory, size_t size) :
      _factory(factory), _max_size(size) {}

  std::unique_ptr<T> acquire();
  std::unique_ptr<T> acquire(std::chrono::duration<float> timeout);

  /// Put an object back to the pool.
  ///
  /// NOTE: It doesn't check if the object was actually created in this
  /// pool before!
  void release(std::unique_ptr<T>);
};

} // namespace Util
} // namespace Fancysoft
