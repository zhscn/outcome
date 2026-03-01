#pragma once

#include <atomic>
#include <cstddef>
#include <cstring>
#include <string>

namespace outcome::core {

// A reference-counted or thunk-managed string reference.
// This is fundamental to the domain ABI — domain messages return string_ref
// which manages its own lifecycle via a thunk function pointer.
class string_ref {
public:
  using size_type = std::size_t;

  // Thunk operations
  enum class _thunk_op {
    copy,
    move,
    destruct,
  };

  // The thunk function type: manages lifecycle of the string data.
  // When called with copy/move, dest is the target string_ref.
  // When called with destruct, dest is the string_ref being destroyed.
  using _thunk_type = void (*)(string_ref *dest, const string_ref *src,
                               _thunk_op op) noexcept;

private:
  const char *_begin{nullptr};
  size_type _length{0};
  void *_state[3]{nullptr, nullptr, nullptr}; // opaque state for thunk
  _thunk_type _thunk{nullptr};

  // Default thunk for non-owning references: does nothing
  static void _null_thunk(string_ref * /*dest*/, const string_ref * /*src*/,
                          _thunk_op /*op*/) noexcept {}

public:
  // Default construct: empty string
  constexpr string_ref() noexcept = default;

  // Construct from string literal or static string (non-owning)
  constexpr string_ref(const char *str, size_type len) noexcept
      : _begin(str), _length(len), _state{}, _thunk(_null_thunk) {}

  // Construct from C string (non-owning)
  string_ref(const char *str) noexcept
      : _begin(str), _length(str ? std::strlen(str) : 0), _state{},
        _thunk(_null_thunk) {}

  // Construct with thunk (owning)
  string_ref(const char *str, size_type len, void *state0, void *state1,
             void *state2, _thunk_type thunk) noexcept
      : _begin(str), _length(len), _state{state0, state1, state2},
        _thunk(thunk) {}

  // Copy
  string_ref(const string_ref &o) noexcept
      : _begin(o._begin), _length(o._length), _state{}, _thunk(o._thunk) {
    std::memcpy(_state, o._state, sizeof(_state));
    if (_thunk)
      _thunk(this, &o, _thunk_op::copy);
  }

  // Move
  string_ref(string_ref &&o) noexcept
      : _begin(o._begin), _length(o._length), _state{}, _thunk(o._thunk) {
    std::memcpy(_state, o._state, sizeof(_state));
    if (_thunk)
      _thunk(this, &o, _thunk_op::move);
  }

  string_ref &operator=(const string_ref &o) noexcept {
    if (this != &o) {
      this->~string_ref();
      new (this) string_ref(o);
    }
    return *this;
  }

  string_ref &operator=(string_ref &&o) noexcept {
    if (this != &o) {
      this->~string_ref();
      new (this) string_ref(static_cast<string_ref &&>(o));
    }
    return *this;
  }

  ~string_ref() noexcept {
    if (_thunk)
      _thunk(this, nullptr, _thunk_op::destruct);
  }

  // Accessors
  [[nodiscard]] const char *data() const noexcept { return _begin; }
  [[nodiscard]] size_type size() const noexcept { return _length; }
  [[nodiscard]] size_type length() const noexcept { return _length; }
  [[nodiscard]] bool empty() const noexcept { return _length == 0; }
  [[nodiscard]] const char *begin() const noexcept { return _begin; }
  [[nodiscard]] const char *end() const noexcept { return _begin + _length; }

  // Convert to std::string
  [[nodiscard]] std::string str() const { return {_begin, _length}; }

  // Access internal state (for thunk implementations)
  void *const *_state_ptr() const noexcept { return _state; }
  void **_state_ptr() noexcept { return _state; }
};

// A string_ref implementation that uses atomic reference counting.
class atomic_refcounted_string_ref {
  struct _allocated {
    std::atomic<unsigned> count;
    size_t length;
    char data[];
  };

public:
  static string_ref create(const char *str, size_t len) {
    auto *alloc =
        static_cast<_allocated *>(::operator new(sizeof(_allocated) + len + 1));
    new (&alloc->count) std::atomic<unsigned>(1);
    alloc->length = len;
    std::memcpy(alloc->data, str, len);
    alloc->data[len] = '\0';

    return string_ref(alloc->data, len, alloc, nullptr, nullptr, _thunk);
  }

  static string_ref create(const std::string &str) {
    return create(str.data(), str.size());
  }

  static string_ref create(const char *str) {
    return create(str, std::strlen(str));
  }

private:
  static void _thunk(string_ref *dest, const string_ref *src,
                     string_ref::_thunk_op op) noexcept {
    switch (op) {
    case string_ref::_thunk_op::copy: {
      auto *alloc =
          static_cast<_allocated *>(const_cast<void *>(src->_state_ptr()[0]));
      alloc->count.fetch_add(1, std::memory_order_relaxed);
      break;
    }
    case string_ref::_thunk_op::move: {
      // Source's thunk state is already copied; null out source
      const_cast<string_ref *>(src)->_state_ptr()[0] = nullptr;
      break;
    }
    case string_ref::_thunk_op::destruct: {
      auto *alloc = static_cast<_allocated *>(dest->_state_ptr()[0]);
      if (alloc && alloc->count.fetch_sub(1, std::memory_order_acq_rel) == 1) {
        alloc->count.~atomic();
        ::operator delete(alloc);
      }
      break;
    }
    }
  }
};

} // namespace outcome::core
