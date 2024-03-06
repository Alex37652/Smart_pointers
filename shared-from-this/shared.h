#pragma once

#include "sw_fwd.h"
// #include "weak.h" // Forward declaration

#include <cstddef>  // std::nullptr_t
#include <type_traits>
#include <memory>

// https://en.cppreference.com/w/cpp/memory/shared_ptr

class BaseSharedFromThis {};

struct ControlBlockBase {
    size_t strong_cnt = 1;
    size_t weak_cnt = 0;
    bool ptr_deleted = false;
    virtual void DeletePtr() = 0;
    virtual ~ControlBlockBase() = default;
};

template <typename T>
struct ControlBlockPointer : public ControlBlockBase {
    explicit ControlBlockPointer(T* ptr) : ptr_(ptr){};
    T* ptr_;

    void DeletePtr() override {
        if (!ptr_deleted) {
            delete ptr_;
            ptr_deleted = true;
        }
    }
    ~ControlBlockPointer() override {
        if (!ptr_deleted) {
            delete ptr_;
        }
    }
};

template <typename T>
struct ControlBlockEmplace : public ControlBlockBase {
    template <typename... Args>
    explicit ControlBlockEmplace(Args&&... args) {
        new (&storage_) T{std::forward<Args>(args)...};
    };

    void DeletePtr() override {
        if (!ptr_deleted) {
            std::destroy_at(std::launder(reinterpret_cast<T*>(&storage_)));
            ptr_deleted = true;
        }
    }

    ~ControlBlockEmplace() override {
        if (!ptr_deleted) {
            std::destroy_at(std::launder(reinterpret_cast<T*>(&storage_)));
        }
    }
    std::aligned_storage_t<sizeof(T), alignof(T)> storage_;
};

template <typename T>
class SharedPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    SharedPtr(){};
    SharedPtr(std::nullptr_t) {
    }
    explicit SharedPtr(T* ptr) {
        block_ = new ControlBlockPointer(ptr);
        ptr_ = ptr;
        if constexpr (std::is_convertible_v<T*, BaseSharedFromThis*>) {
            ptr_->block_ = block_;
            ptr_->ptr_ = ptr;
        }
    };

    template <typename Y>
    explicit SharedPtr(Y* ptr) {
        block_ = new ControlBlockPointer(ptr);
        ptr_ = ptr;
        if constexpr (std::is_convertible_v<Y*, BaseSharedFromThis*>) {
            ptr_->block_ = block_;
            ptr_->ptr_ = ptr_;
        }
    };

    SharedPtr(const SharedPtr& other) {
        block_ = other.block_;
        if (block_) {
            block_->strong_cnt++;
        }
        ptr_ = other.ptr_;
    };

    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other) {
        block_ = other.GetBlock();
        if (block_) {
            block_->strong_cnt++;
        }
        ptr_ = other.Get();
    };

    SharedPtr(SharedPtr&& other) {
        block_ = other.block_;
        ptr_ = other.ptr_;
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    };

    template <typename Y>
    SharedPtr(SharedPtr<Y>&& other) {
        block_ = other.GetBlock();
        ptr_ = other.Get();
        block_->strong_cnt++;
        other.Reset();
    };

    SharedPtr(ControlBlockEmplace<T>* block) {
        block_ = block;
        ptr_ = std::launder(reinterpret_cast<T*>(&block->storage_));
        if constexpr (std::is_convertible_v<T*, BaseSharedFromThis*>) {
            ptr_->block_ = block_;
            ptr_->ptr_ = ptr_;
        }
    }

    SharedPtr(ControlBlockBase* block, T* ptr) {
        block_ = block;
        ptr_ = ptr;
        if (block_) {
            block_->strong_cnt++;
        }
    }

    //    template <typename Y>
    //    SharedPtr(ControlBlockPointer<Y>* block, Y* ptr) {
    //        block_ = block;
    //        ptr_ = ptr;
    //    }

    // Aliasing constructor
    // #8 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    template <typename Y>
    SharedPtr(const SharedPtr<Y>& other, T* ptr) {
        ptr_ = ptr;
        if (other.GetBlock()) {
            block_ = other.GetBlock();
            block_->strong_cnt++;
        }
    };

    // Promote `WeakPtr`
    // #11 from https://en.cppreference.com/w/cpp/memory/shared_ptr/shared_ptr
    explicit SharedPtr(const WeakPtr<T>& other) {
        if (other.UseCount()) {
            block_ = other.block_;
            ptr_ = other.ptr_;
            if (block_) {
                ++block_->strong_cnt;
            }
        } else {
            throw BadWeakPtr();
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    SharedPtr& operator=(const SharedPtr& other) {
        if (this != &other) {
            if (block_) {
                --block_->strong_cnt;
                if (block_->strong_cnt == 0 && block_->weak_cnt == 0) {
                    delete block_;
                } else if (block_->strong_cnt == 0) {
                    block_->DeletePtr();
                    ptr_ = nullptr;
                }
            }
            ptr_ = other.ptr_;
            block_ = other.block_;
            if (block_) {
                block_->strong_cnt++;
            }
        }
        return *this;
    };

    SharedPtr& operator=(SharedPtr&& other) {
        if (this != &other) {
            if (block_) {
                --block_->strong_cnt;
                if (block_->strong_cnt == 0 && block_->weak_cnt == 0) {
                    delete block_;
                } else if (block_->strong_cnt == 0) {
                    block_->DeletePtr();
                    ptr_ = nullptr;
                }
            }
            block_ = other.block_;
            other.block_ = nullptr;
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Destructor

    ~SharedPtr() {
        if (block_) {
            --block_->strong_cnt;
            if (block_->strong_cnt == 0 && block_->weak_cnt == 0) {
                delete block_;
            } else if (block_->strong_cnt == 0) {
                block_->DeletePtr();
                ptr_ = nullptr;
            }
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_) {
            --block_->strong_cnt;
            if (block_->strong_cnt == 0 && block_->weak_cnt == 0) {
                delete block_;
            } else if (block_->strong_cnt == 0) {
                block_->DeletePtr();
                ptr_ = nullptr;
            }
        }
        block_ = nullptr;
        ptr_ = nullptr;
    };

    void Reset(T* ptr) {
        if (block_) {
            --block_->strong_cnt;
            if (block_->strong_cnt == 0 && block_->weak_cnt == 0) {
                delete block_;
            } else if (block_->strong_cnt == 0) {
                block_->DeletePtr();
                ptr_ = nullptr;
            }
        }
        block_ = new ControlBlockPointer<T>(ptr);
        ptr_ = ptr;
    };

    template <typename Y>
    void Reset(Y* ptr) {
        if (block_) {
            --block_->strong_cnt;
            if (block_->strong_cnt == 0 && block_->weak_cnt == 0) {
                delete block_;
            } else if (block_->strong_cnt == 0) {
                block_->DeletePtr();
                ptr_ = nullptr;
            }
        }
        block_ = new ControlBlockPointer<Y>(ptr);
        ptr_ = ptr;
    };

    void Swap(SharedPtr& other) {
        std::swap(block_, other.block_);
        std::swap(ptr_, other.ptr_);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    ControlBlockBase* GetBlock() {
        if (block_) {
            return block_;
        }
        return nullptr;
    };

    ControlBlockBase* GetBlock() const {
        if (block_) {
            return block_;
        }
        return nullptr;
    };

    T* Get() const {
        return ptr_;
    };

    T* Get() {
        return ptr_;
    };

    T& operator*() const {
        return *ptr_;
    };

    T* operator->() const {
        return ptr_;
    };

    size_t UseCount() const {
        if (block_) {
            return block_->strong_cnt;
        }
        return 0;
    };

    explicit operator bool() const {
        return block_;
    };

private:
    ControlBlockBase* block_ = nullptr;
    T* ptr_ = nullptr;
};

template <typename T, typename U>
inline bool operator==(const SharedPtr<T>& left, const SharedPtr<U>& right) {
    if (left.GetBlock() && right.GetBlock()) {
        return left.Get() == right.Get();
    }
};

// Allocate memory only once
template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
    auto block = new ControlBlockEmplace<T>(std::forward<Args>(args)...);
    return SharedPtr<T>(block);
};

// Look for usage examples in tests and seminar
template <typename T>
class EnableSharedFromThis : public BaseSharedFromThis {
public:
    SharedPtr<T> SharedFromThis() {
        return SharedPtr<T>(block_, ptr_);
    };
    SharedPtr<const T> SharedFromThis() const {
        return SharedPtr<T>(block_, ptr_);
    };

    WeakPtr<T> WeakFromThis() noexcept {
        return WeakPtr<T>(block_, ptr_);
    };
    WeakPtr<const T> WeakFromThis() const noexcept {
        return WeakPtr<const T>(block_, ptr_);
    };

private:
    ControlBlockBase* block_ = nullptr;
    T* ptr_ = nullptr;
    template <typename Y>
    friend class SharedPtr;
};
