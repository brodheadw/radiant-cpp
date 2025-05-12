// radiant/UniquePtr.h
// Copyright 2025 The Radiant Authors.
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//     http://www.apache.org/licenses/LICENSE-2.0
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#pragma once

#include "radiant/TotallyRad.h"
#include "radiant/TypeTraits.h"
#include "radiant/Utility.h"   // for rad::swap overload
#include <utility>             // std::move
#include <cstddef>             // nullptr_t
#include <algorithm>           // std::swap

namespace rad
{

// Empty deleter type so it occupies no storage (EBO).
template <typename T>
struct DefaultDelete {
    constexpr DefaultDelete() noexcept = default;
    void operator()(T* p) const noexcept { delete p; }
};

// UniquePtr<T, Deleter> owns a T* and invokes Deleter when destroyed.
// If Deleter is empty, sizeof(UniquePtr) == sizeof(T*).
template <typename T, typename Deleter = DefaultDelete<T>>
class UniquePtr : private Deleter
{
public:
    RAD_NOT_COPYABLE(UniquePtr);

    using pointer      = T*;
    using deleter_type = Deleter;

    /// @brief Default constructs to nullptr.
    constexpr UniquePtr() noexcept
      : Deleter(),  // base subobject
        m_ptr(nullptr)
    {}

    /// @brief Constructs owning a raw pointer + optional custom deleter.
    explicit constexpr UniquePtr(pointer p,
                                 Deleter d = Deleter()) noexcept
      : Deleter(std::move(d)),
        m_ptr(p)
    {}

    /// @brief Destroys the managed object if non-null.
    ~UniquePtr() noexcept
    {
        if (m_ptr) static_cast<Deleter&>(*this)(m_ptr);
    }

    /// @brief Move-constructs, stealing ownership.
    constexpr UniquePtr(UniquePtr&& o) noexcept
      : Deleter(std::move(o.get_deleter())),
        m_ptr(o.m_ptr)
    {
        o.m_ptr = nullptr;
    }

    /// @brief Move-assigns, destroying current and stealing ownership.
    UniquePtr& operator=(UniquePtr&& o) noexcept
    {
        if (this != &o)
        {
            reset();
            m_ptr = o.m_ptr;
            get_deleter() = std::move(o.get_deleter());
            o.m_ptr = nullptr;
        }
        return *this;
    }

    // observers

    /// @return the stored pointer (may be nullptr)
    constexpr pointer get() const noexcept { return m_ptr; }

    /// @return reference to *get()
    constexpr T& operator*() const noexcept { return *m_ptr; }

    /// @return get(), for pointer‐like syntax
    constexpr pointer operator->() const noexcept { return m_ptr; }

    /// @return true if get() != nullptr
    explicit constexpr operator bool() const noexcept { return m_ptr != nullptr; }

    // modifiers

    /// @brief Release ownership without deleting; returns previous pointer.
    pointer release() noexcept
    {
        pointer tmp = m_ptr;
        m_ptr = nullptr;
        return tmp;
    }

    /// @brief Delete current and take new pointer p (default nullptr)
    void reset(pointer p = nullptr) noexcept
    {
        if (m_ptr) static_cast<Deleter&>(*this)(m_ptr);
        m_ptr = p;
    }

    /// @brief Swap pointers and deleters with another UniquePtr
    void swap(UniquePtr& o) noexcept
    {
        using std::swap;
        swap(m_ptr, o.m_ptr);
        swap(get_deleter(), o.get_deleter());
    }

    /// @return reference to the stored deleter
    constexpr Deleter&       get_deleter() noexcept       { return static_cast<Deleter&>(*this); }
    constexpr const Deleter& get_deleter() const noexcept { return static_cast<const Deleter&>(*this); }

private:
    pointer m_ptr;
};

/// @brief ADL‐enabled swap overload
template <typename T, typename D>
void swap(UniquePtr<T, D>& a, UniquePtr<T, D>& b) noexcept
{
    a.swap(b);
}

/// @brief Alias using default deleter
template <typename T>
using UniquePtrDefault = UniquePtr<T>;

} // namespace rad