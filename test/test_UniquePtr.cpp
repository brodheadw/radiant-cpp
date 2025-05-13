// test/test_UniquePtr.cpp
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

#include "gtest/gtest.h"
#include "radiant/UniquePtr.h"

// simple type to track live instances
struct Foo {
  static int live;
  int value;
  Foo(int v) : value(v) { ++live; }
  ~Foo()         { --live; }
};
int Foo::live = 0;

// custom deleter functor for testing
struct D {
  static int count;
  void operator()(int* p) const noexcept {
    ++count;
    delete p;
  }
};
int D::count = 0;

// custom deleter with state for get_deleter test
struct D2 {
  int state;
  static int count;
  D2(int s = 0) : state(s) {}

  template<typename U>
  void operator()(U* p) const noexcept {
    ++count;
    delete p;
  }
};
int D2::count = 0;

// Ensure empty deleters take no extra space
static_assert(sizeof(rad::UniquePtrDefault<Foo>) == sizeof(Foo*),
              "UniquePtrDefault<Foo> must be one pointer");
static_assert(sizeof(rad::UniquePtr<int, D>) == sizeof(int*),
              "UniquePtr with empty deleter must be one pointer");

// Non-empty deleter size check
struct NonEmpty { int x; void operator()(int*) noexcept {} };
static_assert(sizeof(rad::UniquePtr<int, NonEmpty>) > sizeof(int*),
              "UniquePtr with non-empty deleter must be larger than a pointer");

TEST(UniquePtr, BasicLifetimeAndSize) {
  EXPECT_EQ(Foo::live, 0);
  {
    rad::UniquePtrDefault<Foo> p(new Foo{42});
    EXPECT_TRUE(p);
    EXPECT_EQ(p->value, 42);
    EXPECT_EQ(Foo::live, 1);
  }
  EXPECT_EQ(Foo::live, 0);
}

TEST(UniquePtr, NullptrAssignment) {
  rad::UniquePtrDefault<int> p(new int{3});
  EXPECT_TRUE(p);
  p = nullptr;
  EXPECT_FALSE(p);
}

TEST(UniquePtr, ReleaseAndReset) {
  Foo::live = 0;
  Foo* raw = new Foo{7};
  rad::UniquePtrDefault<Foo> p(raw);
  EXPECT_TRUE(p);
  EXPECT_EQ(Foo::live, 1);

  Foo* r = p.release();
  EXPECT_EQ(r, raw);
  EXPECT_FALSE(p);
  delete r;
  EXPECT_EQ(Foo::live, 0);

  p.reset(new Foo{5});
  EXPECT_TRUE(p);
  EXPECT_EQ(p->value, 5);
  EXPECT_EQ(Foo::live, 1);

  p.reset();
  EXPECT_FALSE(p);
  EXPECT_EQ(Foo::live, 0);
}

TEST(UniquePtr, MoveSemantics) {
  Foo::live = 0;
  rad::UniquePtrDefault<Foo> a(new Foo{1});
  EXPECT_TRUE(a);

  rad::UniquePtrDefault<Foo> b(std::move(a));
  EXPECT_FALSE(a);
  EXPECT_TRUE(b);
  EXPECT_EQ(b->value, 1);

  Foo::live = 1;
  rad::UniquePtrDefault<Foo> c;
  c = std::move(b);
  EXPECT_FALSE(b);
  EXPECT_TRUE(c);
  EXPECT_EQ(c->value, 1);

  c.reset();
  EXPECT_EQ(Foo::live, 0);
}

struct Base { virtual ~Base() = default; };
struct Derived : Base { int v = 9; };

TEST(UniquePtr, ConvertingMove) {
  rad::UniquePtr<Derived> d(new Derived{});
  rad::UniquePtr<Base>    b(std::move(d));
  EXPECT_TRUE(b);
  EXPECT_FALSE(d);

  rad::UniquePtr<Derived> d2(new Derived{});
  b = std::move(d2);
  EXPECT_TRUE(b);
  EXPECT_FALSE(d2);
}

TEST(UniquePtr, Swap) {
  Foo::live = 0;
  rad::UniquePtrDefault<Foo> x(new Foo{10});
  rad::UniquePtrDefault<Foo> y(new Foo{20});
  EXPECT_EQ(x->value, 10);
  EXPECT_EQ(y->value, 20);

  x.swap(y);
  EXPECT_EQ(x->value, 20);
  EXPECT_EQ(y->value, 10);

  using std::swap;
  swap(x, y);
  EXPECT_EQ(x->value, 10);
  EXPECT_EQ(y->value, 20);

  x.reset();
  y.reset();
  EXPECT_EQ(Foo::live, 0);
}

TEST(UniquePtr, CustomDeleter) {
  D::count = 0;
  int* raw = new int(99);

  rad::UniquePtr<int, D> p(raw, D());
  EXPECT_TRUE(p);
  EXPECT_EQ(*p, 99);
  EXPECT_EQ(D::count, 0);

  p.reset();
  EXPECT_FALSE(p);
  EXPECT_EQ(D::count, 1);
}

// Tests for allocator-based make_unique
TEST(UniquePtr, MakeUniqueBasic) {
  Foo::live = 0;
  auto p = rad::make_unique<Foo>(55);
  EXPECT_TRUE(p);
  EXPECT_EQ(p->value, 55);
  EXPECT_EQ(Foo::live, 1);
  p.reset();
  EXPECT_FALSE(p);
  EXPECT_EQ(Foo::live, 0);
}

TEST(UniquePtr, MakeUniqueInt) {
  auto pi = rad::make_unique<int>(7);
  EXPECT_TRUE(pi);
  EXPECT_EQ(*pi, 7);
}

// Test destructor invocation for custom deleter
TEST(UniquePtr, CustomDeleterDestructor) {
  D::count = 0;
  {
    int* raw = new int(111);
    rad::UniquePtr<int, D> p(raw, D());
    EXPECT_EQ(D::count, 0);
  }
  EXPECT_EQ(D::count, 1);
}

// Test get_deleter state for custom deleter with state
TEST(UniquePtr, CustomDeleterState) {
  D2::count = 0;
  D2 d2{123};
  rad::UniquePtr<int, D2> p(new int{8}, d2);
  EXPECT_EQ(p.get_deleter().state, 123);
  p.reset();
  EXPECT_EQ(D2::count, 1);
}

// Test get() method
TEST(UniquePtr, GetPointer) {
  int* raw = new int{321};
  rad::UniquePtr<int, D> p(raw, D());
  EXPECT_EQ(p.get(), raw);
  p.release();
  delete raw;
}

// Self-move assignment safety
TEST(UniquePtr, SelfMoveAssignment) {
  Foo::live = 0;
  auto u = rad::make_unique<Foo>(1);
  EXPECT_TRUE(u);
  EXPECT_EQ(Foo::live, 1);
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-move"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wself-move"
#endif
  u = std::move(u);
#if defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
  EXPECT_TRUE(u);
  EXPECT_EQ(Foo::live, 1);
}

// Converting move with custom deleter
TEST(UniquePtr, ConvertingMoveCustomDeleter) {
  D2::count = 0;
  using UpDerived = rad::UniquePtr<Derived, D2>;
  using UpBase    = rad::UniquePtr<Base, D2>;
  UpDerived dptr(new Derived(), D2(42));
  UpBase    bptr(std::move(dptr));
  EXPECT_FALSE(dptr);
  EXPECT_TRUE(bptr);
  EXPECT_EQ(bptr.get_deleter().state, 42);
  bptr.reset();
  EXPECT_EQ(D2::count, 1);
}

// Dereference operator* returns reference
TEST(UniquePtr, DereferenceReturnsReference) {
  Foo::live = 0;
  auto p = rad::make_unique<Foo>(99);
  Foo& f = *p;
  EXPECT_EQ(f.value, 99);
  p.reset();
  EXPECT_EQ(Foo::live, 0);
}

// Double reset is no-op
TEST(UniquePtr, DoubleResetNoOp) {
  Foo::live = 0;
  auto p = rad::make_unique<Foo>(2);
  EXPECT_EQ(Foo::live, 1);
  p.reset();
  EXPECT_FALSE(p);
  EXPECT_EQ(Foo::live, 0);
  p.reset();
  EXPECT_EQ(Foo::live, 0);
}
