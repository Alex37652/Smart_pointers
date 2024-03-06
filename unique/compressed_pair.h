#pragma once

#pragma once

#include <type_traits>
#include <iostream>
#include <memory>
#include <cstddef>

// Me think, why waste time write lot code, when few code do trick.

template <typename T, size_t I, bool = std::is_empty_v<T> && !std::is_final_v<T>>
struct CommpressedPairElement {

    T value_;

    CommpressedPairElement() : value_(){};
    CommpressedPairElement(const T& v) : value_(v){};
    CommpressedPairElement(T&& v) : value_(std::move(v)){};

    const T& GetElement() const {
        return value_;
    }

    T& GetElement() {
        return value_;
    }
};

template <typename T, size_t I>
struct CommpressedPairElement<T, I, true> : public T {

    CommpressedPairElement() {
    }

    CommpressedPairElement(const T& val) {
    }

    T& GetElement() {
        return *this;
    }
    const T& GetElement() const {
        return *this;
    }
};

template <typename F, typename S>
class CompressedPair : private CommpressedPairElement<F, 0>, private CommpressedPairElement<S, 1> {
public:
    CompressedPair() : First(), Second() {
    }

    CompressedPair(F& first, S& second) : First(first), Second(second) {
    }

    CompressedPair(F&& first, S&& second) : First(std::move(first)), Second(std::move(second)) {
    }

    CompressedPair(F& first, S&& second) : First(first), Second(std::move(second)) {
    }

    CompressedPair(F&& first, S& second) : First(std::move(first)), Second(second) {
    }

    F& GetFirst() {
        return First::GetElement();
    }

    S& GetSecond() {
        return Second::GetElement();
    };

    const F& GetFirst() const {
        return First::GetElement();
    }

    const S& GetSecond() const {
        return Second ::GetElement();
    };

private:
    using First = CommpressedPairElement<F, 0>;
    using Second = CommpressedPairElement<S, 1>;
};
