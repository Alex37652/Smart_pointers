#pragma once

#include "compressed_pair.h"

#include <cstddef>  // std::nullptr_t

struct Slug {};

template <typename T>
class DefaultDeleter {
public:
    DefaultDeleter() = default;

    template <typename S>
    DefaultDeleter(DefaultDeleter<S>&&) {
        DefaultDeleter<S>();
    };

    void operator()(T* ptr) const noexcept {
        delete ptr;
    }
};

template <typename T>
class DefaultDeleter<T[]> {
public:
    void operator()(T* ptr) const noexcept {
        delete[] ptr;
    }
};

// Primary template
template <typename T, typename Deleter = DefaultDeleter<T>>
class UniquePtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors
    explicit UniquePtr(T* ptr = nullptr) : pair_(ptr, Deleter()){};

    UniquePtr(T* ptr, Deleter deleter) : pair_(ptr, std::move(deleter)){};

    UniquePtr(UniquePtr&& other) noexcept
        : pair_(other.Release(), std::move(other.pair_.GetSecond())){};

    template <typename S, typename SDeleter>
    UniquePtr(UniquePtr<S, SDeleter>&& other) noexcept {
        pair_.GetFirst() = other.Release();
        pair_.GetSecond() = std::move(other.GetDeleter());
    };

    UniquePtr(UniquePtr& other) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    UniquePtr& operator=(UniquePtr&& other) noexcept {
        if (this != &other) {
            pair_.GetSecond()(pair_.GetFirst());
            pair_.GetFirst() = other.pair_.GetFirst();
            other.pair_.GetFirst() = nullptr;
            GetDeleter() = std::move(other.GetDeleter());
        }
        return *this;
    };

    UniquePtr& operator=(std::nullptr_t) {
        pair_.GetSecond()(pair_.GetFirst());
        pair_.GetFirst() = std::nullptr_t();
        return *this;
    };

    UniquePtr& operator=(UniquePtr& other) = delete;

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~UniquePtr() {
        pair_.GetSecond()(pair_.GetFirst());
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    T* Release() {
        auto tmp = pair_.GetFirst();
        pair_.GetFirst() = nullptr;
        return tmp;
    };

    void Reset(T* ptr = nullptr) {
        if (pair_.GetFirst()) {
            auto tmp = pair_.GetFirst();
            pair_.GetFirst() = ptr;
            pair_.GetSecond()(tmp);
        }
    };

    void Swap(UniquePtr& other) {
        std::swap(pair_.GetFirst(), other.pair_.GetFirst());
        std::swap(pair_.GetSecond(), other.pair_.GetSecond());
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    T* Get() const {
        return pair_.GetFirst();
    };
    Deleter& GetDeleter() {
        return (pair_.GetSecond());
    };
    const Deleter& GetDeleter() const {
        return (pair_.GetSecond());
    };
    explicit operator bool() const {
        return pair_.GetFirst();
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Single-object dereference operators

    std::add_lvalue_reference_t<T> operator*() const {
        return *pair_.GetFirst();
    };

    T* operator->() const {
        return pair_.GetFirst();
    };

private:
    CompressedPair<T*, Deleter> pair_;
};

// Specialization for arrays
template <typename T, typename Deleter>
class UniquePtr<T[], Deleter> {
public:
    explicit UniquePtr(T* ptr = nullptr) : pair_(ptr, Deleter()){};

    void Reset(T* ptr = nullptr) {
        if (pair_.GetFirst()) {
            auto tmp = pair_.GetFirst();
            pair_.GetFirst() = ptr;
            pair_.GetSecond()(tmp);
        }
    };

    ~UniquePtr() {
        pair_.GetSecond()(pair_.GetFirst());
    };

    T& operator[](size_t i) {
        return pair_.GetFirst()[i];
    }

    T& operator[](size_t i) const {
        return pair_.GetFirst()[i];
    }

private:
    CompressedPair<T*, Deleter> pair_;
};
