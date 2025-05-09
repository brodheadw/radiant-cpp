// test/test_UniquePtr.cpp
// Copyright 2025 The Radiant Authors.
// Licensed under the Apache License, Version 2.0 (the "License"); …

#include "gtest/gtest.h"
#include "radiant/UniquePtr.h"

namespace rad {
  // convenience alias for tests
  template <typename T>
  using UniquePtrDefault = UniquePtr<T, void(*)(T*)>;
}

struct Foo {
  static int live;
  int value;
  Foo(int v) : value(v) { ++live; }
  ~Foo() { --live; }
};
int Foo::live = 0;

TEST(UniquePtr, BasicLifetimeAndSize) {
  static_assert(sizeof(rad::UniquePtrDefault<Foo>) == sizeof(Foo*),
                "UniquePtrDefault<Foo> must be one pointer in size");
  
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

  // move‐assign
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

  // clean up
  x.reset();
  y.reset();
  EXPECT_EQ(Foo::live, 0);
}

TEST(UniquePtr, CustomDeleter) {
  struct D {
    static int count;
    static void del(int* p) noexcept {
      ++count;
      ::delete p;
    }
  };
  int* raw = new int(99);
  static_assert(!std::is_same_v<decltype(D::del), void(*)(int*)> ||
                sizeof(rad::UniquePtr<int, decltype(&D::del)>) == sizeof(int*),
                "still one-pointer");

  rad::UniquePtr<int, decltype(&D::del)> p(raw, &D::del);
  EXPECT_EQ(*p, 99);
  EXPECT_EQ(D::count, 0);

  p.reset();
  EXPECT_EQ(D::count, 1);
  EXPECT_FALSE(p);
}
int D::count = 0;
