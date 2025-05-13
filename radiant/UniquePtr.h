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

// radiant/UniquePtr.h
#pragma once

#include "radiant/Utility.h"                // rad::Move
#include "radiant/Algorithm.h"              // rad::Swap
#include "radiant/detail/StdTypeTraits.h"   // rad::IsConv, rad::EnIf
#include "radiant/Memory.h"                 // AllocTraits
#include <cstddef>                           // nullptr_t

namespace rad {

/// @brief Default deleter using Radiant's StdAllocator (EBO if empty).
template <typename T>
struct DefaultDelete : private StdAllocator {
    using allocator_type = StdAllocator;
    using pointer        = T*;

    // Default construct the allocator
    constexpr DefaultDelete() noexcept = default;
    // Construct from a given allocator instance
    explicit constexpr DefaultDelete(const allocator_type& a) noexcept
      : allocator_type(a)
    {}

    /// @brief Converting deleter: allow DefaultDelete<T> ← DefaultDelete<U> when U* → T*
    template <typename U,
              rad::EnIf<rad::IsConv<U*, T*>, int> = 0>
    constexpr DefaultDelete(const DefaultDelete<U>&) noexcept {}

    void operator()(pointer p) const noexcept {
        if (!p) return;
        allocator_type alloc = get_allocator();
        using Traits = AllocTraits<allocator_type>;
        Traits::Destroy(alloc, p);
        Traits::Free(  alloc, p, 1);
    }

    constexpr allocator_type&       get_allocator()       noexcept { return static_cast<allocator_type&>(*this); }
    constexpr const allocator_type& get_allocator() const noexcept { return static_cast<const allocator_type&>(*this); }
};

/// @brief A unique ownership smart pointer with customizable deleter.
/// If the deleter is empty (via EBO), sizeof(UniquePtr)==sizeof(T*).
template <typename T, typename Deleter = DefaultDelete<T>>
class UniquePtr : private Deleter {
public:
    RAD_NOT_COPYABLE(UniquePtr);

    using pointer      = T*;
    using deleter_type = Deleter;

    /// @brief Constructs null.
    constexpr UniquePtr() noexcept = default;

    /// @brief Constructs owning p, with optional custom deleter d.
    explicit constexpr UniquePtr(pointer p, Deleter d = Deleter()) noexcept
      : Deleter(rad::Move(d)), m_ptr(p)
    {}

    /// @brief Move-constructs, stealing ownership.
    constexpr UniquePtr(UniquePtr&& o) noexcept
      : Deleter(rad::Move(o.get_deleter())), m_ptr(o.release())
    {}

    /// @brief Converting move from UniquePtr<U> when U*→T*.
    template <typename U, typename E,
              rad::EnIf<rad::IsConv<U*, T*>, int> = 0>
    constexpr UniquePtr(UniquePtr<U,E>&& o) noexcept
      : Deleter(rad::Move(o.get_deleter())), m_ptr(o.release())
    {}

    /// @brief Destroys the managed object if non-null.
    ~UniquePtr() noexcept {
        if (m_ptr) static_cast<Deleter&>(*this)(m_ptr);
    }

    /// @brief nullptr-assignment (clears the pointer).
    UniquePtr& operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    /// @brief Move-assigns, destroying current and stealing ownership.
    UniquePtr& operator=(UniquePtr&& o) noexcept {
        if (this != &o) {
            reset();
            m_ptr = o.m_ptr;
            get_deleter() = rad::Move(o.get_deleter());
            o.m_ptr = nullptr;
        }
        return *this;
    }

    /// @brief Converting move-assign from UniquePtr<U> when U*→T*.
    template <typename U, typename E,
              rad::EnIf<rad::IsConv<U*, T*>, int> = 0>
    UniquePtr& operator=(UniquePtr<U,E>&& o) noexcept {
        reset(o.release());
        get_deleter() = rad::Move(o.get_deleter());
        return *this;
    }

    /// @brief Implicit bool conversion: true if non-null.
    constexpr operator bool() const noexcept { return m_ptr != nullptr; }

    /// @brief Observers
    constexpr pointer get() const noexcept       { return m_ptr; }
    constexpr T&     operator*() const noexcept  { return *m_ptr; }
    constexpr pointer operator->() const noexcept { return m_ptr; }

    /// @brief Release ownership without deletion.
    pointer release() noexcept {
        pointer tmp = m_ptr;
        m_ptr = nullptr;
        return tmp;
    }

    /// @brief Delete current and take new pointer p (default nullptr).
    void reset(pointer p = nullptr) noexcept {
        if (m_ptr) static_cast<Deleter&>(*this)(m_ptr);
        m_ptr = p;
    }

    /// @brief Swap pointers and deleters.
    void swap(UniquePtr& o) noexcept {
        using rad::Swap;
        Swap(m_ptr, o.m_ptr);
        Swap(get_deleter(), o.get_deleter());
    }

    /// @brief Access the stored deleter.
    constexpr Deleter&       get_deleter()       noexcept { return static_cast<Deleter&>(*this); }
    constexpr const Deleter& get_deleter() const noexcept { return static_cast<const Deleter&>(*this); }

private:
    pointer m_ptr = nullptr;
};

/// @brief ADL swap overload.
template <typename T, typename D>
void swap(UniquePtr<T,D>& a, UniquePtr<T,D>& b) noexcept {
    a.swap(b);
}

/// @brief Alias using default deleter.
template <typename T>
using UniquePtrDefault = UniquePtr<T>;

/// @brief Allocator-aware make_unique: allocates via AllocTraits, constructs, and returns UniquePtr.
template <typename T, typename... Args>
UniquePtr<T> make_unique(Args&&... args) {
    using D = DefaultDelete<T>;
    using A = typename D::allocator_type;
    using Traits = AllocTraits<A>;

    A alloc;
    T* raw = Traits::template Alloc<T>(alloc, 1);
    Traits::Construct(alloc, raw, rad::Forward<Args>(args)...);
    return UniquePtr<T>(raw, D(alloc));
}

} // namespace rad