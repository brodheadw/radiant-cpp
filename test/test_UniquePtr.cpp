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

TEST(UniquePtr, BasicLifetimeAndSize) {
  // the default deleter is empty, so this must be one pointer in size
  static_assert(sizeof(rad::UniquePtrDefault<Foo>) == sizeof(Foo*),
                "UniquePtrDefault<Foo> must be one pointer");

  EXPECT_EQ(Foo::live, 0);
  {
    rad::UniquePtrDefault<Foo> p(new Foo{42});
    EXPECT_TRUE(p);
    EXPECT_EQ(p->value, 42);
    EXPECT_EQ(Foo::live, 1);
  }
  // destructor should have run
  EXPECT_EQ(Foo::live, 0);
}

TEST (UniquePtr, NullptrAssignment) {
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
  // user must delete now:
  delete r;
  EXPECT_EQ(Foo::live, 0);

  // reset to new object
  p.reset(new Foo{5});
  EXPECT_TRUE(p);
  EXPECT_EQ(p->value, 5);
  EXPECT_EQ(Foo::live, 1);

  // reset to nullptr
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

  // move-assign
  Foo::live = 1;
  rad::UniquePtrDefault<Foo> c;
  c = std::move(b);
  EXPECT_FALSE(b);
  EXPECT_TRUE(c);
  EXPECT_EQ(c->value, 1);

  // cleanup
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

  // ADL swap
  using std::swap;
  swap(x, y);
  EXPECT_EQ(x->value, 10);
  EXPECT_EQ(y->value, 20);

  // cleanup
  x.reset();
  y.reset();
  EXPECT_EQ(Foo::live, 0);
}

TEST(UniquePtr, CustomDeleter) {
  D::count = 0;
  int* raw = new int(99);

  // UniquePtr<int, D> uses our functor D
  rad::UniquePtr<int, D> p(raw, D());
  EXPECT_TRUE(p);
  EXPECT_EQ(*p, 99);
  EXPECT_EQ(D::count, 0);

  p.reset();
  EXPECT_FALSE(p);
  EXPECT_EQ(D::count, 1);
}