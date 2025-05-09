// Copyright 2025 The Radiant Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "radiant/TotallyRad.h"
#include "radiant/TypeTraits.h"
#include "radiant/Utility.h" // NOLINT(misc-include-cleaner)
#include "radiant/Memory.h"

namespace rad
{
/// @brief Exclusive ownership smart pointer analog.
/// @details A move-only pointer wrapper that manages a single, heap-allocated
/// object of type T, ensuring lifetime-based destruction without relying on
/// external allocators or additional storage beyond one pointer.
/// @tparam T the type of the object being owned.

template <typename T, typename Deleter = void(*) (T*)>
class UniquePtr
{
public:
    RAD_NOT_COPYABLE(UniquePtr);

    /// @brief Default deleter: plain delete.
    static void DefaultDeleter(T* ptr) noexcept { delete ptr; }

    using pointer      = T*;
    using deleter_type = Deleter;

    /// @brief Default constructs to nullptr.
    /// @details ctor is constexpr to allow for static initialization,
    /// dtor is not constexpr to allow for custom deleters.
    constexpr UniquePtr() noexcept
      : m_ptr(nullptr), 
        m_del(Deleter(DefaultDeleter))
    {}

    /// @brief Constructs owning a raw pointer.
    /// @param p 
    /// @param d 
    explicit constexpr UniquePtr(pointer p,
                                 Deleter d = Deleter(DefaultDeleter)) noexcept
      : m_ptr(p),
        m_del(p)
    {}

    /// @brief Constructs from a raw pointer.
    /// @details This constructor is constexpr to allow for static initialization.
    /// @param p the raw pointer to take ownership of.
    /// @param d the deleter to use for destruction.
    /// @param alloc the allocator to use for allocation.
    /// @param tag the tag to use for allocation.
    /// @param size the size of the object to allocate.
    /// @param alignment the alignment of the object to allocate.
    /// @param args the arguments to pass to the constructor of the object.
    ~UniquePtr() noexcept
    {
        if (m_ptr) m_del(m_ptr);
    }

    /// @brief Move ownership of object.
    /// @details
    constexpr UniquePtr(UniquePtr&& o) noexcept
      : m_ptr(o.m_ptr),
        m_del(o.m_del)
    {
        o.m_ptr = nullptr;
    }

    UniquePtr& operator=(UniquePtr&& o) noexcept
    {
        if (this != &o)
        {
            if (m_ptr) m_del(m_ptr);
            m_ptr = o.m_ptr;
            m_del = o.m_del;
            o.m_ptr = nullptr;
        }
        return *this;
    }

    // observers
    constexpr pointer get() const noexcept          { return m_ptr; }
    constexpr T&      operator*()  const noexcept   { return *m_ptr; }
    constexpr pointer operator->() const noexcept   { return m_ptr; }
    explicit constexpr operator bool() const noexcept { return m_ptr != nullptr; }

    // modifiers
    pointer release() noexcept
    {
        pointer tmp = m_ptr;
        m_ptr = nullptr;
        return tmp;
    }

    void reset(pointer p = nullptr) noexcept
    {
        if (m_ptr) m_del(m_ptr);
        m_ptr = p;
    }

    /// @brief Swaps the contents of this unique pointer with another.
    /// @details This is a no-op if the pointers are the same.
    /// @param o
    void swap(UniquePtr& o) noexcept
    {
        rad::Swap(m_ptr, o.m_ptr);
        rad::Swap(m_del, o.m_del);
    }

    constexpr Deleter&       get_deleter() noexcept { return m_del; }
    constexpr const Deleter& get_deleter() const noexcept { return m_del; }

private:
    pointer m_ptr;
    Deleter m_del;
};

template <typename T>
using UniquePtrDefault = UniquePtr<T, void(*) (T*)>;

} // namespace rad