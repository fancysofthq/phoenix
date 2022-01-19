#include "fancysoft/phoenix/util/pool.hh"

namespace Fancysoft::Phoenix::Util {

template <class T> std::unique_ptr<T> Pool<T>::acquire() {
  std::unique_lock lock(_mutex);

  while (true) {
    if (!_queue.empty()) {
      auto obj = move(_queue.front());
      _queue.pop_front();
      return obj;
    } else {
      if (_current_size < _max_size) {
        _current_size++;
        auto obj = std::make_unique<T>(_factory());
        return obj;
      } else {
        _condvar.wait(lock);
        continue;
      }
    }
  }

  return nullptr;
}

template <class T>
std::unique_ptr<T> Pool<T>::acquire(std::chrono::duration<float> timeout) {
  std::unique_lock lock(_mutex);

  while (true) {
    if (!_queue.empty()) {
      auto obj = move(_queue.front());
      _queue.pop_front();
      return obj;
    } else {
      if (_current_size < _max_size) {
        _current_size++;
        auto obj = std::make_unique<T>(_factory());
        return obj;
      } else {
        if (_condvar.wait_for(lock, timeout) == std::cv_status::no_timeout) {
          continue;
        } else {
          return nullptr;
        }
      }
    }
  }

  return nullptr;
}

template <class T> void Pool<T>::release(std::unique_ptr<T> obj) {
  std::unique_lock lock(_mutex);
  _queue.push_back(move(obj));
  _condvar.notify_one();
}

} // namespace Fancysoft::Phoenix::Util
