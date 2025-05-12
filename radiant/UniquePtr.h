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
template <typename T, typename Deleter = DefaultDelete<T>>

class UniquePtr : private Deleter
{
public:
    RAD_NOT_COPYABLE(UniquePtr);

    using pointer      = T*;

    struct DefaultDelete
    {
        constexpr DefaultDelete() noexcept = default;
        void operator()(T* p) const noexcept { delete p; }
    };

    constexpr UniquePtr() noexcept
      : Deleter(), // empty base
        m_ptr(nullptr)
    {}

    explicit constexpr UniquePtr(pointer p, Deleter d = Deleter()) noexcept
      : Deleter(std::move(d)),
        m_ptr(p)
    {}

    ~UniquePtr() noexcept
    {
        if (m_ptr) static_cast<Deleter&>(*this)(m_ptr);
    }

    constexpr UniquePtr(UniquePtr&& o) noexcept
      : Deleter(std::move(o.get_deleter())),
        m_ptr(o.m_ptr)
    {
        o.m_ptr = nullptr;
    }

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
        if (m_ptr) static_cast<Deleter&>(*this)(m_ptr);
        m_ptr = p;
    }

    /// @brief Swaps the contents of this unique pointer with another.
    /// @details This is a no-op if the pointers are the same.
    /// @param o
    void swap(UniquePtr& o) noexcept
    {
        rad::Swap(m_ptr, o.m_ptr);
        rad::Swap(get_deleter(), o.get_deleter());
    }

    constexpr Deleter&       get_deleter() noexcept { return m_del; }
    constexpr const Deleter& get_deleter() const noexcept { return m_del; }

private:
    pointer m_ptr;
};

template <typename T>
using UniquePtrDefault = UniquePtr<T>;

} // namespace rad