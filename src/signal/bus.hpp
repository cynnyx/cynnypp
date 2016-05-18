#pragma once


#include <memory>
#include <utility>
#include <type_traits>
#include "signal.hpp"


namespace cynny {
namespace cynnypp {
namespace signal {


namespace details {

template<class E>
struct event_tag { using type = E; };


template<int N, int M>
struct choice: public choice<N+1, M> { };

template<int N>
struct choice<N, N> { };


template<typename...>
using void_t = void;

template<typename, typename, typename = void_t<>>
struct has_receive_member: std::false_type { };

template<typename C, typename E>
struct has_receive_member<C, E, void_t<decltype(std::declval<C>().receive(std::declval<E>()))>>: std::true_type { };

} // namespace details


template<class D>
struct Receiver: public std::enable_shared_from_this<D>
{ };


template<int S, class... T>
class BusBase;

template<int S, class E, class... O>
class BusBase<S, E, O...>: public BusBase<S, O...> {
    using Base = BusBase<S, O...>;

protected:
    using Base::get;
    using Base::reg;
    using Base::unreg;

    Signal<E>& get(details::event_tag<E>) {
        return signal;
    }

    template<class C>
    std::enable_if_t<details::has_receive_member<C, E>::value>
    reg(details::choice<S-(sizeof...(O)+1), S>, std::weak_ptr<C> ptr) {
        signal.template add<C, &C::receive>(ptr);
        Base::reg(details::choice<S-sizeof...(O), S>{}, ptr);
    }

    template<class C>
    std::enable_if_t<details::has_receive_member<C, E>::value>
    unreg(details::choice<S-(sizeof...(O)+1), S>, std::weak_ptr<C> ptr) {
        signal.template remove<C, &C::receive>(ptr);
        Base::unreg(details::choice<S-sizeof...(O), S>{}, ptr);
    }

    std::size_t size() const noexcept {
        return signal.size() + Base::size();
    }

private:
    Signal<E> signal;
};

template<int S>
class BusBase<S> {
protected:
    virtual ~BusBase() { }
    void get();
    void reg(details::choice<S, S>, std::weak_ptr<void>) { }
    void unreg(details::choice<S, S>, std::weak_ptr<void>) { }
    std::size_t size() const noexcept { return 0; }
};

template<class... T>
class Bus: public BusBase<sizeof...(T), T...> {
    using Base = BusBase<sizeof...(T), T...>;

public:
    using Base::size;

    template<class E, void(*F)(const E &)>
    void add() {
        Signal<E> &signal = Base::get(details::event_tag<E>{});
        signal.template add<F>();
    }

    template<class C>
    void reg(std::shared_ptr<C> &ptr) {
        Base::reg(details::choice<0, sizeof...(T)>{}, ptr);
    }

    template<class C>
    void unreg(std::shared_ptr<C> &ptr) {
        Base::unreg(details::choice<0, sizeof...(T)>{}, ptr);
    }

    template<class E, class C, void(C::*M)(const E &) = &C::receive>
    void add(std::shared_ptr<C> &ptr) {
        Signal<E> &signal = Base::get(details::event_tag<E>{});
        signal.template add<C, M>(ptr);
    }

    template<class E, void(*F)(const E &)>
    void remove() {
        Signal<E> &signal = Base::get(details::event_tag<E>{});
        signal.template remove<F>();
    }

    template<class E, class C, void(C::*M)(const E &) = &C::receive>
    void remove(std::shared_ptr<C> &ptr) {
        Signal<E> &signal = Base::get(details::event_tag<E>{});
        signal.template remove<C, M>(ptr);
    }

    template<class E, class... A>
    void publish(A&&... args) {
        Signal<E> &signal = Base::get(details::event_tag<E>{});
        E e(std::forward<A>(args)...);
        signal.publish(e);
    }
};


}}}
