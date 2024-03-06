#pragma once

#include "sw_fwd.h"
#include "shared.h"  // Forward declaration

// https://en.cppreference.com/w/cpp/memory/weak_ptr
template <typename T>
class WeakPtr {
public:
    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Constructors

    WeakPtr(){};

    WeakPtr(const WeakPtr& other) {
        block_ = other.block_;
        if (block_) {
            block_->weak_cnt++;
        }
        ptr_ = other.ptr_;
    };
    WeakPtr(WeakPtr&& other) {
        block_ = other.block_;
        ptr_ = other.ptr_;
        other.block_ = nullptr;
        other.ptr_ = nullptr;
    };

    // Demote `SharedPtr`
    // #2 from https://en.cppreference.com/w/cpp/memory/weak_ptr/weak_ptr
    WeakPtr(const SharedPtr<T>& other) {
        block_ = other.GetBlock();
        if (block_) {
            block_->weak_cnt++;
        }
        ptr_ = other.Get();
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // `operator=`-s

    WeakPtr& operator=(const WeakPtr& other) {
        if (this != &other) {
            if (block_) {
                --block_->weak_cnt;
                if (block_->strong_cnt == 0 && block_->weak_cnt == 0) {
                    delete block_;
                } else if (block_->strong_cnt == 0) {
                    block_->DeletePtr();
                }
            }
            ptr_ = other.ptr_;
            block_ = other.block_;
            if (block_) {
                block_->weak_cnt++;
            }
        }
        return *this;
    };

    WeakPtr& operator=(WeakPtr&& other) {
        if (this != &other) {
            if (block_) {
                --block_->weak_cnt;
                if (block_->strong_cnt == 0 && block_->weak_cnt == 0) {
                    delete block_;
                } else if (block_->strong_cnt == 0) {
                    block_->DeletePtr();
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

    ~WeakPtr() {
        if (block_) {
            --block_->weak_cnt;
            if (block_->strong_cnt == 0 && block_->weak_cnt == 0) {
                delete block_;
            } else if (block_->strong_cnt == 0) {
                block_->DeletePtr();
            }
        }
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Modifiers

    void Reset() {
        if (block_) {
            --block_->weak_cnt;
            if (block_->strong_cnt == 0 && block_->weak_cnt == 0) {
                delete block_;
            } else if (block_->strong_cnt == 0) {
                block_->DeletePtr();
            }
        }
        block_ = nullptr;
        ptr_ = nullptr;
    };
    void Swap(WeakPtr& other) {
        std::swap(block_, other.block_);
        std::swap(ptr_, other.ptr_);
    };

    ////////////////////////////////////////////////////////////////////////////////////////////////
    // Observers

    size_t UseCount() const {
        if (block_) {
            return block_->strong_cnt;
        }
        return 0;
    };
    bool Expired() const {
        if (block_) {
            if (block_->strong_cnt > 0) {
                return false;
            }
        }
        return true;
    };
    SharedPtr<T> Lock() const {
        if (block_) {
            if (block_->strong_cnt > 0) {
                ++block_->strong_cnt;
                return SharedPtr<T>(block_, ptr_);
            } else {
                ++block_->strong_cnt;
                return SharedPtr<T>(block_, nullptr);
            }
        }
        return SharedPtr<T>(block_, ptr_);
    };

private:
    ControlBlockBase* block_ = nullptr;
    T* ptr_ = nullptr;

    template <typename Y>
    friend class SharedPtr;
};
