// Coroutines implementation inspired by
// https://kirit.com/How%20C%2B%2B%20coroutines%20work
// © 2002-2019 Kirit & Tai Sælensminde

#pragma once

#include <exception>
#include <iostream>
#include <optional>

// HACK: Coroutines status is... questionable among the compilers.
// TODO: Define compiler versions for even more precise settings.
//

// Native CLang on Windows with MSVC driver.
#if defined(__clang__) && defined(_WIN32) && defined(_MSC_VER)

// See the included header for details.
//

#include "./vendor/coroutines-msvc-clang.hh"
#define __CORO_NS std::experimental

// CLang on MinGW.
#elif defined(__clang__) && defined(__MINGW32__)

// CLang on MinGW requries libstdc++ for coroutines, stored in
// `msys64\mingw64\include\c++\v1\experimental\coroutine`.
//

#include <experimental/coroutine>
#define __CORO_NS std::experimental

// GNU CC on MinGW.
#elif defined(__GNUC__) && defined(__MINGW32__)

// Modern GNU CC on MinGW stores the header at
// `msys64\mingw64\include\c++\10.2.0\coroutine`.
//

#include <coroutine>
#define __CORO_NS std

// Clang on MacOS.
#elif defined(__clang__) && defined(__APPLE__)

#include <experimental/coroutine>
#define __CORO_NS std::experimental

#else

#error "Coroutines are not implemented yet for this platform"

#endif

namespace Fancysoft {
namespace Phoenix {
namespace Util {
namespace Coro {

template <typename T> struct Generator {
  struct Promise {
    std::optional<T> current_value;

    Promise(){};
    ~Promise(){};

    __CORO_NS::suspend_always initial_suspend() {
      return __CORO_NS::suspend_always{};
    }

    Generator get_return_object() {
      return Generator{Handle::from_promise(*this)};
    }

    __CORO_NS::suspend_always yield_value(T value) {
      current_value = value;
      return __CORO_NS::suspend_always{};
    }

    __CORO_NS::suspend_never return_void() {
      return __CORO_NS::suspend_never{};
    }

    __CORO_NS::suspend_always final_suspend() noexcept {
      return __CORO_NS::suspend_always{};
    }

    void unhandled_exception() {
      std::cerr << "Unhandled exception in coroutine body\n";
      std::terminate();
    }
  };

  struct Iterator {
    Generator &owner;
    bool done;

    Iterator(Generator &owner, bool done) : owner(owner), done(done) {
      if (not done)
        advance();
    }

    void advance() {
      owner.coro.resume();
      done = owner.coro.done();
    }

    bool operator!=(const Iterator &i) const { return done != i.done; }

    Iterator &operator++() {
      advance();
      return *this;
    }

    T operator*() const { return owner.coro.promise().current_value.value(); }
  };

  using Handle = __CORO_NS::coroutine_handle<Promise>;

  Generator(const Generator &) = delete;
  Generator(Generator &&g) : coro(g.coro) { g.coro = nullptr; };

  ~Generator() {
    if (coro)
      coro.destroy();
  }

  T current() { return coro.promise().current_value.value(); }

  void resume() { coro.resume(); }

  T next() {
    coro.resume();
    return current();
  }

  bool done() { return coro.done(); }

  Iterator begin() { return Iterator{*this, false}; }
  Iterator end() { return Iterator{*this, true}; }

private:
  Handle coro;
  Generator(Handle h) : coro(h){};
};

template <typename T> struct CoReturn {
  struct Promise;
  friend struct Promise;

  using Handle = __CORO_NS::coroutine_handle<Promise>;

  CoReturn(const CoReturn &) = delete;
  CoReturn(CoReturn &&s) : coro(s.coro) { s.coro = nullptr; }

  ~CoReturn() {
    if (coro)
      coro.destroy();
  }

  CoReturn &operator=(const CoReturn &) = delete;

  CoReturn &operator=(CoReturn &&s) {
    coro = s.coro;
    s.coro = nullptr;
    return *this;
  }

  struct Promise {
    T value;

    friend struct CoReturn;

    Promise() {}
    ~Promise() {}

    __CORO_NS::suspend_never return_value(T v) {
      value = v;
      return __CORO_NS::suspend_never{};
    }

    __CORO_NS::suspend_always final_suspend() noexcept {
      return __CORO_NS::suspend_always{};
    }

    void unhandled_exception() {
      std::cerr << "Unhandled exception in coroutine body\n";
      std::terminate();
    }
  };

protected:
  Handle coro;
  CoReturn(Handle h) : coro(h) {}
  T get() { return coro.promise().value; }
};

template <typename T> struct Sync : public CoReturn<T> {
  using CoReturn<T>::CoReturn;
  using Handle = typename CoReturn<T>::Handle;

  struct Promise : public CoReturn<T>::Promise {
    Sync<T> get_return_object() { return Sync<T>{Handle::from_promise(*this)}; }

    __CORO_NS::suspend_never initial_suspend() {
      return __CORO_NS::suspend_never{};
    }
  };

  T get() { return CoReturn<T>::get(); }
};

template <typename T> struct Async : public CoReturn<T> {
  using CoReturn<T>::CoReturn;
  using Handle = typename CoReturn<T>::Handle;

  struct Promise : public CoReturn<T>::Promise {
    Async<T> get_return_object() {
      return Async<T>{Handle::from_promise(*this)};
    }

    __CORO_NS::suspend_always initial_suspend() {
      return __CORO_NS::suspend_always{};
    }
  };

  struct Awaitable {
    Handle coro;

    // Return a boolean to describe whether or not a suspend is
    // needed, `true` for "don't suspend" and `false` for "suspend".
    bool await_ready() { return this->coro.done(); }

    // Called when a suspend is needed because the value isn't ready
    // yet.
    __CORO_NS::coroutine_handle<>
    await_suspend(__CORO_NS::coroutine_handle<> awaiting) {
      this->coro.resume();
      return awaiting;
    }

    // Return the value that is being awaited on.
    // This becomes the result of the `co_await` expression.
    T await_resume() { return this->coro.promise().value; }
  };

  T get() {
    if (!this->coro.done())
      this->coro.resume();

    return CoReturn<T>::get();
  }

  Awaitable operator co_await() { return Awaitable{this->coro}; }
};

} // namespace Coro
} // namespace Util
} // namespace Phoenix
} // namespace Fancysoft

#undef __CORO_NS
