#pragma once

#include "domain.hpp"
#include "errc.hpp"

namespace outcome::core {

namespace detail {

template <class StatusCode, class Allocator = std::allocator<StatusCode>>
class indirecting_domain : public status_code_domain {
public:
  using inner_type = StatusCode;
  using allocator_type = Allocator;

  struct payload_type {
    StatusCode sc;
    Allocator alloc;

    payload_type(StatusCode &&s, Allocator a)
        : sc(static_cast<StatusCode &&>(s)), alloc(a) {}
    payload_type(const StatusCode &s, Allocator a) : sc(s), alloc(a) {}
  };

  using value_type = payload_type *;

  constexpr indirecting_domain() noexcept
      : status_code_domain(0xc44f7bdeb2cc50e9ULL ^
                           StatusCode::domain_type::get().id()) {}

  static const indirecting_domain &get() noexcept {
    static const indirecting_domain singleton{};
    return singleton;
  }

  [[nodiscard]] string_ref name() const noexcept override {
    return string_ref("indirecting domain");
  }

  [[nodiscard]] payload_info_t payload_info() const noexcept override {
    return {sizeof(value_type),
            sizeof(status_code_domain *) + sizeof(value_type),
            alignof(value_type)};
  }

protected:
  [[nodiscard]] bool
  _do_failure(const status_code<void> &code) const noexcept override {
    auto *p = _get_payload(code);
    return p ? p->sc.failure() : false;
  }

  [[nodiscard]] bool
  _do_equivalent(const status_code<void> &code1,
                 const status_code<void> &code2) const noexcept override {
    auto *p = _get_payload(code1);
    if (!p)
      return false;
    const auto &inner_base = reinterpret_cast<const status_code<void> &>(p->sc);
    const status_code_domain &dom = p->sc.domain();
    return dom._do_equivalent(inner_base, code2);
  }

  [[nodiscard]] generic_code
  _generic_code(const status_code<void> &code) const noexcept override {
    auto *p = _get_payload(code);
    if (!p)
      return generic_code(errc::unknown);
    const auto &inner_base = reinterpret_cast<const status_code<void> &>(p->sc);
    const status_code_domain &dom = p->sc.domain();
    return dom._generic_code(inner_base);
  }

  [[nodiscard]] string_ref
  _do_message(const status_code<void> &code) const noexcept override {
    auto *p = _get_payload(code);
    if (!p)
      return string_ref("(empty nested code)");
    return p->sc.message();
  }

#ifdef __cpp_exceptions
  [[noreturn]] void
  _do_throw_exception(const status_code<void> &code) const override {
    auto *p = _get_payload(code);
    if (p) {
      p->sc.throw_exception();
    }
    throw status_error<_generic_code_domain>(generic_code(errc::unknown));
  }
#endif

  void _do_erased_copy(status_code<void> &dst, const status_code<void> &src,
                       payload_info_t /*info*/) const override {
    auto *p = _get_payload(src);
    if (!p)
      return;

    using rebound_alloc = typename std::allocator_traits<
        Allocator>::template rebind_alloc<payload_type>;
    using alloc_traits = std::allocator_traits<rebound_alloc>;

    rebound_alloc alloc(p->alloc);
    auto *np = alloc_traits::allocate(alloc, 1);
    alloc_traits::construct(alloc, np, p->sc.clone(), p->alloc);

    // Write domain pointer and payload pointer directly into dst's memory
    auto *raw = reinterpret_cast<char *>(&dst);
    auto *dom_ptr = this;
    std::memcpy(raw, &dom_ptr, sizeof(dom_ptr));
    auto np_val = reinterpret_cast<intptr_t>(np);
    std::memcpy(raw + sizeof(dom_ptr), &np_val, sizeof(np_val));
  }

  void _do_erased_destroy(status_code<void> &code,
                          payload_info_t /*info*/) const noexcept override {
    auto *p = _get_payload(code);
    if (!p)
      return;

    using rebound_alloc = typename std::allocator_traits<
        Allocator>::template rebind_alloc<payload_type>;
    using alloc_traits = std::allocator_traits<rebound_alloc>;

    rebound_alloc alloc(p->alloc);
    alloc_traits::destroy(alloc, p);
    alloc_traits::deallocate(alloc, p, 1);
  }

private:
  static payload_type *_get_payload(const status_code<void> &code) noexcept {
    // The erased status code stores [domain_ptr | intptr_t_value].
    // The intptr_t value is a pointer to our payload.
    auto *raw = reinterpret_cast<const char *>(&code);
    intptr_t val;
    std::memcpy(&val, raw + sizeof(const status_code_domain *), sizeof(val));
    return reinterpret_cast<payload_type *>(val);
  }
};

} // namespace detail

template <class T, class Alloc = std::allocator<std::decay_t<T>>>
inline status_code<detail::erased<intptr_t>>
make_nested_status_code(T &&v, Alloc alloc = {}) {
  using SC = std::decay_t<T>;
  using domain_type = detail::indirecting_domain<SC, Alloc>;
  using payload_type = typename domain_type::payload_type;

  using rebound_alloc = typename std::allocator_traits<
      Alloc>::template rebind_alloc<payload_type>;
  using alloc_traits = std::allocator_traits<rebound_alloc>;

  rebound_alloc a(alloc);
  auto *p = alloc_traits::allocate(a, 1);
  alloc_traits::construct(a, p, static_cast<T &&>(v), alloc);

  // Build the erased status code by writing raw memory
  status_code<detail::erased<intptr_t>> ret;
  auto *raw = reinterpret_cast<char *>(&ret);
  auto *dom_ptr = &domain_type::get();
  std::memcpy(raw, &dom_ptr, sizeof(dom_ptr));
  auto val = reinterpret_cast<intptr_t>(p);
  std::memcpy(raw + sizeof(dom_ptr), &val, sizeof(val));
  return ret;
}

template <class StatusCode, class U>
StatusCode *get_if(status_code<detail::erased<U>> *v) noexcept {
  if (!v || v->empty())
    return nullptr;
  using domain_type =
      detail::indirecting_domain<StatusCode, std::allocator<StatusCode>>;
  if (v->domain().id() != domain_type::get().id())
    return nullptr;
  auto *raw = reinterpret_cast<const char *>(v);
  intptr_t val;
  std::memcpy(&val, raw + sizeof(const status_code_domain *), sizeof(val));
  auto *p = reinterpret_cast<typename domain_type::payload_type *>(val);
  return &p->sc;
}

template <class StatusCode, class U>
const StatusCode *get_if(const status_code<detail::erased<U>> *v) noexcept {
  return get_if<StatusCode>(const_cast<status_code<detail::erased<U>> *>(v));
}

} // namespace outcome::core
