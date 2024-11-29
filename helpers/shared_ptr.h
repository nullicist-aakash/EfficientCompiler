#pragma once

#include <utility>
#include <exception>

namespace helpers::internal
{
    template <typename T>
    struct shared_ptr_block
    {
        T* obj;
        int count;

        constexpr explicit shared_ptr_block(T* obj) noexcept : obj(obj), count(1) {}
        constexpr shared_ptr_block(const shared_ptr_block<T>& other) noexcept = delete;
        constexpr shared_ptr_block(shared_ptr_block<T>&& other) noexcept = delete;
        constexpr shared_ptr_block<T>& operator=(const shared_ptr_block<T>& other) noexcept = delete;
        constexpr shared_ptr_block<T>& operator=(shared_ptr_block<T>&& other) noexcept = delete;

        constexpr ~shared_ptr_block()
        {
            delete obj;
        }
    };
}

namespace helpers
{
    using namespace helpers::internal;

    template <typename T>
    class shared_ptr
    {
        shared_ptr_block<T>* block;

        constexpr void decrement() noexcept
        {
            if (block->count <= 0)
                std::terminate();

            block->count--;
            if (block->count == 0)
            {
                delete block;
                block = nullptr;
            }
        }

    public:
        constexpr explicit shared_ptr(T* obj) noexcept
        {
            block = new shared_ptr_block<T>(obj);
        }

        constexpr shared_ptr(const shared_ptr<T>& other) noexcept
        {
            block = other.block;
            block->count++;
        }

        constexpr shared_ptr<T>& operator=(const shared_ptr<T>& other) noexcept
        {
            if (this == &other || block == other.block)
                return *this;

            this->decrement();

            block = other.block;
            block->count++;
            return *this;
        }

        constexpr shared_ptr(shared_ptr<T>&& other) noexcept = delete;

        constexpr shared_ptr<T>& operator=(shared_ptr<T>&& other) noexcept = delete;

        constexpr shared_ptr<T>& operator=(T* obj) noexcept
        {
            *this = shared_ptr(obj);
            return *this;
        }

        constexpr auto& operator*(this auto&& self) noexcept
        {
            return *self.block->obj;
        }

        constexpr auto& operator->(this auto&& self) noexcept
        {
            return self.block->obj;
        }

        constexpr ~shared_ptr() noexcept
        {
            this->decrement();
        }
    };

    template <typename T, typename ... Args>
    constexpr auto make_shared(Args&& ... args)
    {
        return shared_ptr(new T(std::forward<Args>(args)...));
    }
}