#ifndef FUNC_FUNC_INL_H_
#define FUNC_FUNC_INL_H_

#include <cassert>
#include <vector>
#include <unordered_map>

namespace func {
namespace details {

class Private {
 private:
  Private() {}

  template <template <typename...> class CF, typename EF, template <typename...>
            class RF, typename PF, typename FF, func::details::FuncType TF>
  friend class func::View;

  template <template <typename...> class CF, typename EF>
  friend View<CF, EF> func::_(CF<EF>&& c);

  template <template <typename...> class C, typename E>
  friend View<C, E> func::_(const C<E>& c);

  template <template <typename...> class C, typename E>
  friend View<C, E, func::details::Ref> func::_(const C<E>* c);
};

}  // namespace details

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
View<C, E, R, P, F, t>::View(const C<E>& c, func::details::Private)
    : container_(c) {}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
View<C, E, R, P, F, t>::View(C<E>&& c, func::details::Private)
    : container_(std::move(c)) {}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
View<C, E, R, P, F, t>::View(const P& p, F f, func::details::Private)
    : container_(), parent_(p), func_(f) {}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
View<C, E, R, P, F, t>::View(const P& p, F f, E data, uint64_t metadata,
                             func::details::Private)
    : container_(), parent_(p), func_(f), data_(data), metadata_(metadata) {}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
View<C, E, R, P, F, t>::View(const View& that)
    : container_(!that.container_ ? std::move(CPtr()) : that.container_),
      parent_(that.parent_),
      func_(that.func_),
      data_(that.data_),
      metadata_(that.metadata_) {}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
View<C, E, R, P, F, t>& View<C, E, R, P, F, t>::operator=(const View& that) {
  container_.reset(new C<E>(*that.container_));
  parent_ = that.parent_;
  func_ = that.func_;
  data_ = that.data_;
  metadata_ = that.metadata_;
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G>
typename View<C, E, R, P, F, t>::template FView<G>
View<C, E, R, P, F, t>::filter(G g) {
  return View<C, E, R, View, G>(*this, g, func::details::Private());
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G>
auto View<C, E, R, P, F, t>::map(G g)
    -> typename View<C, E, R, P, F, t>::template MView<
          decltype(g(*(E*) nullptr)), G> {
  return View<C, typename std::decay<decltype(g(*(E*)nullptr))>::type, R, View,
              G, func::details::FuncType::MAP>(*this, g,
                                               func::details::Private());
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G>
auto View<C, E, R, P, F, t>::flat_map(G g)
    -> View<C, typename decltype(g(*(E*) nullptr))::value_type, R, View, G,
            func::details::FuncType::FLAT_MAP> {
  return View<C, typename decltype(g(*(E*)nullptr))::value_type, R, View, G,
              func::details::FuncType::FLAT_MAP>(*this, g,
                                                 func::details::Private());
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename T, typename G>
T View<C, E, R, P, F, t>::fold_left(T init, G g) {
  do_evaluate([&init, &g](const E& e) { init = g(init, e); });
  return std::move(init);
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G>
E View<C, E, R, P, F, t>::reduce(G g) {
  bool first = true;
  E init;
  do_evaluate([&](const E& e) {
    if (first) {
      init = e;
      first = false;
      return;
    }
    init = g(init, e);
  });
  return std::move(init);
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G>
void View<C, E, R, P, F, t>::for_each(G g) {
  do_evaluate([&](const E& e) { g(e); });
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G>
View<C, E, R, View<C, E, R, P, F, t>, G, func::details::FuncType::SKIP>
View<C, E, R, P, F, t>::skip_until(G g) {
  return View<C, E, R, View, G, func::details::FuncType::SKIP>(
      *this, g, func::details::Private());
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G>
View<C, E, R, View<C, E, R, P, F, t>, G, func::details::FuncType::KEEP>
View<C, E, R, P, F, t>::keep_while(G g) {
  return View<C, E, R, View, G, func::details::FuncType::KEEP>(
      *this, g, func::details::Private());
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
View<C, E, R, View<C, E, R, P, F, t>, std::function<bool(const E&)>,
     func::details::FuncType::SKIP>
View<C, E, R, P, F, t>::drop(size_t n) {
  auto v = View<C, E, R, View, std::function<bool(const E&)>,
                func::details::FuncType::SKIP>(
      *this, std::function<bool(const E&)>(), n, 0, func::details::Private());

  auto f = [&v](const E&) {
    if (v.data_ == 0) {
      return true;
    }

    v.data_--;
    return false;
  };

  v.func_ = f;
  return v;
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <template <typename...> class C2, typename E2, template <typename...>
          class R2, typename P2, typename F2, func::details::FuncType t2>
View<C, std::pair<E, E2>, R,
     std::pair<View<C, E, R, P, F, t>, View<C2, E2, R2, P2, F2, t2>>,
     std::function<void()>, func::details::FuncType::ZIP>
View<C, E, R, P, F, t>::zip(const View<C2, E2, R2, P2, F2, t2>& that) {
  using Pair = std::pair<View, View<C2, E2, R2, P2, F2, t2>>;
  return View<C, std::pair<E, E2>, R, Pair, std::function<void()>,
              func::details::FuncType::ZIP>(std::make_pair(*this, that), [] {},
                                            func::details::Private());
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
E View<C, E, R, P, F, t>::sum() {
  return reduce([](const E& s, const E& e) { return s + e; });
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
E View<C, E, R, P, F, t>::product() {
  return reduce([](const E& s, const E& e) { return s * e; });
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
E View<C, E, R, P, F, t>::first() {
  E first;
  bool is_first = true;
  do_evaluate([&](const E& e) {
    if (!is_first) {
      return;
    }

    first = e;
    is_first = false;
  });

  return first;
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
E View<C, E, R, P, F, t>::last() {
  E last;
  do_evaluate([&](const E& e) {
    // TODO(soheil): This would be very slow.
    last = e;
  });

  return last;
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
size_t View<C, E, R, P, F, t>::size() {
  size_t size = 0;
  do_evaluate([&](const E& e) { size++; });
  return size;
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G>
bool View<C, E, R, P, F, t>::for_all(G g) {
  return fold_left(true,
                   [&g](const bool& all, const E& e) { return all && g(e); });
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G>
typename View<C, E, R, P, F, t>::template FView<G> View<C, E, R, P, F, t>::
operator%(G g) {
  return filter(g);
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G>
auto View<C, E, R, P, F, t>::operator*(G g)
    -> typename View<C, E, R, P, F, t>::template MView<
          decltype(g(*(E*) nullptr)), G> {
  return map(g);
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G>
E View<C, E, R, P, F, t>::operator/(G g) {
  return reduce(g);
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G>
View<C, E, R, P, F, t>& View<C, E, R, P, F, t>::operator>>(G g) {
  for_each(g);
  return *this;
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G,
          typename std::enable_if<sizeof(G) && std::is_same<void*, P>::value,
                                  int>::type>
void View<C, E, R, P, F, t>::do_evaluate(G g) {
  assert(is_evaluated() && "Cannot evaluate a view without a parent.");

  for (const auto& e : *container_) {
    g(e);
  }
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G,
          typename std::enable_if<sizeof(G) && !std::is_same<void*, P>::value &&
                                      t == func::details::FuncType::FILTER,
                                  int>::type>
void View<C, E, R, P, F, t>::do_evaluate(G g) {
  using PE = typename std::decay<typename P::Element>::type;

  parent_.do_evaluate([this, &g](const PE& e) {
    if (!func_(e)) {
      return;
    }

    g(e);
  });
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G,
          typename std::enable_if<sizeof(G) && !std::is_same<void*, P>::value &&
                                      t == func::details::FuncType::ZIP,
                                  int>::type>
void View<C, E, R, P, F, t>::do_evaluate(G g) {
  auto p1 = parent_.first.evaluate();
  auto itr = p1.begin();
  auto end = p1.end();

  using P2E = typename std::decay<typename P::second_type::Element>::type;
  parent_.second.do_evaluate([&g, &itr, &end](const P2E& e) {
    if (itr == end) {
      return;
    }

    g(std::make_pair(*itr, e));
    itr++;
  });
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G,
          typename std::enable_if<sizeof(G) && !std::is_same<void*, P>::value &&
                                      t == func::details::FuncType::SKIP,
                                  int>::type>
void View<C, E, R, P, F, t>::do_evaluate(G g) {
  using PE = typename std::decay<typename P::Element>::type;

  bool passed = false;
  parent_.do_evaluate([this, &g, &passed](const PE& e) {
    if (!passed && !func_(e)) {
      return;
    }

    passed = true;
    g(e);
  });
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G,
          typename std::enable_if<sizeof(G) && !std::is_same<void*, P>::value &&
                                      t == func::details::FuncType::KEEP,
                                  int>::type>
void View<C, E, R, P, F, t>::do_evaluate(G g) {
  using PE = typename std::decay<typename P::Element>::type;

  bool keep = true;
  parent_.do_evaluate([this, &g, &keep](const PE& e) {
    if (!keep) {
      return;
    }

    if (!func_(e)) {
      keep = false;
      return;
    }

    g(e);
  });
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G,
          typename std::enable_if<sizeof(G) && !std::is_same<void*, P>::value &&
                                      t == func::details::FuncType::MAP,
                                  int>::type>
void View<C, E, R, P, F, t>::do_evaluate(G g) {
  using PE = typename std::decay<typename P::Element>::type;

  parent_.do_evaluate([this, &g](const PE& e) { g(func_(e)); });
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G,
          typename std::enable_if<sizeof(G) && !std::is_same<void*, P>::value &&
                                      t == func::details::FuncType::FLAT_MAP,
                                  int>::type>
void View<C, E, R, P, F, t>::do_evaluate(G g) {
  using PE = typename std::decay<typename P::Element>::type;

  parent_.do_evaluate([this, &g](const PE& e) {
    const auto& r = func_(e);
    for (const auto& i : r) {
      g(i);
    }
  });
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename G,
          typename std::enable_if<sizeof(G) && !std::is_same<void*, P>::value &&
                                      t == func::details::FuncType::FOLD_LEFT,
                                  int>::type>
void View<C, E, R, P, F, t>::do_evaluate(G g) {
  using PE = typename std::decay<typename P::Element>::type;

  parent_.do_evaluate([this, &g](const PE& e) { g(func_(e)); });
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
C<E> View<C, E, R, P, F, t>::evaluate() {
  if (is_evaluated()) {
    return *container_;
  }

  C<E> c;
  do_evaluate([&c](const E& e) { c.push_back(e); });
  return std::move(c);
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
std::vector<E> View<C, E, R, P, F, t>::as_vector() {
  std::vector<E> v;
  do_evaluate([&](const E& e) { v.push_back(e); });
  return std::move(v);
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
std::list<E> View<C, E, R, P, F, t>::as_list() {
  std::list<E> l;
  evaluate(&l);
  return std::move(l);
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
std::deque<E> View<C, E, R, P, F, t>::as_deque() {
  std::deque<E> d;
  evaluate(&d);
  return std::move(d);
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename K, typename V,
          typename std::enable_if<sizeof(K) && func::details::is_pair<E>::value,
                                  int>::type>
std::unordered_map<K, V> View<C, E, R, P, F, t>::as_map() {
  std::unordered_map<K, V> m;
  evaluate(&m);
  return std::move(m);
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <template <typename...> class EC>
void View<C, E, R, P, F, t>::evaluate(EC<E>* c) {
  assert(c != nullptr && "Container is nullptr.");
  do_evaluate([&](const E& e) { c->push_back(e); });
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <template <typename...> class EC>
View<C, E, R, P, F, t>& View<C, E, R, P, F, t>::operator>>(EC<E>* container) {
  evaluate(container);
  return *this;
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
template <typename K, typename V,
          typename std::enable_if<sizeof(K) && func::details::is_pair<E>::value,
                                  int>::type>
void View<C, E, R, P, F, t>::evaluate(std::unordered_map<K, V>* m) {
  static_assert(std::is_convertible<typename E::first_type, K>::value,
                "Cannot use K for key.");
  static_assert(std::is_convertible<typename E::second_type, V>::value,
                "Cannot use V for value.");
  do_evaluate([&](const E& e) { m->emplace(e.first, e.second); });
}

template <template <typename...> class C, typename E, template <typename...>
          class R, typename P, typename F, func::details::FuncType t>
View<C, E, R, P, F, t>::operator C<E>() {
  return std::move(evaluate());
}

template <template <typename...> class C, typename E>
View<C, E> _(C<E>&& c) {
  return View<C, E>(std::move(c), func::details::Private());
}

template <template <typename...> class C, typename E>
View<C, E> _(const C<E>& c) {
  return View<C, E>(c, func::details::Private());
}

template <template <typename...> class C, typename E>
View<C, E, func::details::Ref> _(const C<E>* c) {
  return View<C, E, func::details::Ref>(*c, func::details::Private());
}

template <typename E>
View<std::vector, E> _(const std::initializer_list<E>& l) {
  return _(std::vector<E>(l));
}

template <typename K, typename V>
View<std::vector, std::pair<K, V>> _(const std::unordered_map<K, V>& l) {
  return _(std::vector<std::pair<K, V>>(l.begin(), l.end()));
}

}  // namespace func

#endif  // FUNC_FUNC_INL_H_
