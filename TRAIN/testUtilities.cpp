// testUtilities.cpp

/** @file
    Utility Classes Tester

    @author Nicolas Chaussé

    @copyright Copyright 2022 Nicolas Chaussé (nicolaschausse@protonmail.com)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, version 3 of the License only.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <https://www.gnu.org/licenses/>.

    @version 0.1

    @date 2022
*/

/*
**************
** INCLUDES **
**************
*/

#include "Utilities.hpp"

// See https://github.com/onqtam/doctest for Copyright.
//#define DOCTEST_CONFIG_DISABLE
//#define DOCTEST_CONFIG_IMPLEMENT
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#define DOCTEST_CONFIG_SUPER_FAST_ASSERTS
#include "doctest.h"

/*
****************
** PROCEDURES **
****************
*/

static decltype(auto)
Rand()
{
  static std::mt19937_64 r(
    static_cast<decltype(r)::result_type>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));

  return r();
}

/*
***********
** TESTS **
***********
*/

TEST_CASE("String()")
{
  SUBCASE("Empty")
  {
    CHECK_EQ(String(+""), std::string(""));
    CHECK_EQ(String(+""), "");
  }

  SUBCASE("Non Empty") { CHECK_EQ(String(2.3, ' ', -6, +" Hello!\n"), std::string("2.3 -6 Hello!\n")); }
}

TEST_CASE("TypeNameOf()")
{
  struct Base
  {
  } base;
  struct Derived : public Base
  {
  } derived;

  CHECK_MESSAGE(TypeNameOf(base).find("Base") != std::string::npos, TypeNameOf(base));
  CHECK_MESSAGE(TypeNameOf(derived).find("Derived") != std::string::npos, TypeNameOf(derived));
  auto const string{ std::string("Allo") };
  CHECK_MESSAGE(TypeNameOf(string).find("basic_string") != std::string::npos, TypeNameOf(string));
}

TEST_CASE("OpenInputBinaryFileNamed()")
{
  SUBCASE("Non Existing Files")
  {
    {
      auto [file, errorMessage, fileByteSize]{ OpenInputBinaryFileNamed("mpy5pvnp.4jmojvmg") };

      CHECK_UNARY_FALSE(file.good());
      CHECK_EQ(errorMessage, "Can not open for reading. ");
      CHECK_EQ(fileByteSize, 0);
    }
    {
      auto [file, errorMessage, fileByteSize]{ OpenInputBinaryFileNamed("7w20vp.dppt0t") };

      CHECK_UNARY_FALSE(file.good());
      CHECK_EQ(errorMessage, "Can not open for reading. ");
      CHECK_EQ(fileByteSize, 0);
    }

    {
      char cString[]{ "1vjnzlm1.bin" };
      auto [file, errorMessage, fileByteSize]{ OpenInputBinaryFileNamed(cString) };

      CHECK_UNARY_FALSE(file.good());
      CHECK_EQ(errorMessage, "Can not open for reading. ");
      CHECK_EQ(fileByteSize, 0);
    }
    {
      char cString[]{ "e34orj5m" };
      auto [file, errorMessage, fileByteSize]{ OpenInputBinaryFileNamed(cString) };

      CHECK_UNARY_FALSE(file.good());
      CHECK_EQ(errorMessage, "Can not open for reading. ");
      CHECK_EQ(fileByteSize, 0);
    }
  }

  SUBCASE("Existing File")
  {
    auto [file, errorMessage, fileByteSize]{ OpenInputBinaryFileNamed(String(+"8.8")) };

    CHECK_UNARY(file.good());
    CHECK_EQ(errorMessage, "");
    CHECK_EQ(fileByteSize, 8);
  }
}

TEST_CASE("Allocator")
{
  static size_t const Size{ 100'000'000 + (static_cast<size_t>(Rand()) % 10'000'000) };

  SUBCASE("Default")
  {
    Timer t;
    Array<int> a(Size);
    t.lap();
    CHECK_GT(t.elapsedMilliseconds(), 25);
  }

  SUBCASE("No Construct")
  {
    Timer t;
    Array<int, NoConstructAllocator<int>> a(Size);
    t.lap();
    CHECK_LT(t.elapsedMicroseconds(), 50);
  }

  SUBCASE("Default")
  {
    Timer t;
    std::vector<long int> a(Size);
    t.lap();
    CHECK_GT(t.elapsedMilliseconds(), 25);
  }

  SUBCASE("No Construct")
  {
    Timer t;
    std::vector<long int, NoConstructAllocator<long int>> a(Size);
    t.lap();
    CHECK_LT(t.elapsedMicroseconds(), 50);
  }
}

TEST_CASE("Array")
{
  SUBCASE("Can Copy-Construct")
  {
    Array<int> a(3);
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 3);
    a[0] = -9;
    a[1] = -8;
    a[2] = -7;
    CHECK_EQ(a[0], -9);
    CHECK_EQ(a[1], -8);
    CHECK_EQ(a[2], -7);

    decltype(auto) b(a);
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 3);
    CHECK_EQ(a[0], -9);
    CHECK_EQ(a[1], -8);
    CHECK_EQ(a[2], -7);

    a[0] = 22;
    b[1] = 111;

    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 3);
    CHECK_EQ(a[0], 22);
    CHECK_EQ(a[1], -8);
    CHECK_EQ(a[2], -7);

    CHECK_UNARY_FALSE(b.empty());
    CHECK_EQ(b.size(), 3);
    CHECK_EQ(b[0], -9);
    CHECK_EQ(b[1], 111);
    CHECK_EQ(b[2], -7);
  }

  SUBCASE("Can Copy-Assign to Same Size")
  {
    Array<std::string> a(4);
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 4);
    a[0] = "One";
    a[1] = "Two";
    a[2] = "Three";
    a[3] = "Four";
    CHECK_EQ(a[0], "One");
    CHECK_EQ(a[1], "Two");
    CHECK_EQ(a[2], "Three");
    CHECK_EQ(a[3], "Four");

    Array<std::string> b(4);
    CHECK_UNARY_FALSE(b.empty());
    CHECK_EQ(b.size(), 4);
    b[0] = "Un";
    b[1] = "Deux";
    b[2] = "Trois";
    b[3] = "Quatre";
    CHECK_EQ(b[0], "Un");
    CHECK_EQ(b[1], "Deux");
    CHECK_EQ(b[2], "Trois");
    CHECK_EQ(b[3], "Quatre");

    a = b;
    a[1] = "2";
    b[3] = "4";

    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 4);
    CHECK_EQ(a[0], "Un");
    CHECK_EQ(a[1], "2");
    CHECK_EQ(a[2], "Trois");
    CHECK_EQ(a[3], "Quatre");

    CHECK_UNARY_FALSE(b.empty());
    CHECK_EQ(b.size(), 4);
    CHECK_EQ(b[0], "Un");
    CHECK_EQ(b[1], "Deux");
    CHECK_EQ(b[2], "Trois");
    CHECK_EQ(b[3], "4");
  }

  SUBCASE("Can Copy-Assign to Empty")
  {
    Array<std::string> a(4);
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 4);
    a[0] = "Um";
    a[1] = "Dois";
    a[2] = "Três";
    a[3] = "Quatro";
    CHECK_EQ(a[0], "Um");
    CHECK_EQ(a[1], "Dois");
    CHECK_EQ(a[2], "Três");
    CHECK_EQ(a[3], "Quatro");

    Array<std::string> b;
    CHECK_UNARY(b.empty());
    CHECK_EQ(b.size(), 0);

    b = a;
    b[0] = "1";
    a[3] = "4";

    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 4);
    CHECK_EQ(a[0], "Um");
    CHECK_EQ(a[1], "Dois");
    CHECK_EQ(a[2], "Três");
    CHECK_EQ(a[3], "4");

    CHECK_UNARY_FALSE(b.empty());
    CHECK_EQ(b.size(), 4);
    CHECK_EQ(b[0], "1");
    CHECK_EQ(b[1], "Dois");
    CHECK_EQ(b[2], "Três");
    CHECK_EQ(b[3], "Quatro");
  }

  SUBCASE("Can Not Copy-Assign to Different Size")
  {
    Array<std::string> a(2);
    a[0] = "Pejig";
    a[1] = "Nij";
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 2);
    CHECK_EQ(a[0], "Pejig");
    CHECK_EQ(a[1], "Nij");

    Array<std::string> b(1);
    CHECK_UNARY_FALSE(b.empty());
    CHECK_EQ(b.size(), 1);
    CHECK_THROWS(b = a);
    CHECK_NE(b[0], a[0]);

    Array<std::string> c(1000);
    CHECK_UNARY_FALSE(c.empty());
    CHECK_EQ(c.size(), 1000);
    CHECK_THROWS(c = a);
    CHECK_NE(c[0], a[0]);

    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 2);
    CHECK_EQ(a[0], "Pejig");
    CHECK_EQ(a[1], "Nij");
  }

  SUBCASE("Can Be Sized")
  {
    Array<long int> a;
    CHECK_UNARY(a.empty());
    CHECK_EQ(a.size(), 0);

    a.setSize(3);
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 3);

    a[0] = 123;
    a[1] = 234;
    a[2] = 345;
    CHECK_EQ(a[0], 123);
    CHECK_EQ(a[1], 234);
    CHECK_EQ(a[2], 345);
  }

  SUBCASE("Can Resize with Same Size")
  {
    Array<short> a(4);
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 4);

    a[0] = 123;
    a[1] = 234;
    a[2] = 345;
    a[3] = 456;
    CHECK_EQ(a[0], 123);
    CHECK_EQ(a[1], 234);
    CHECK_EQ(a[2], 345);
    CHECK_EQ(a[3], 456);

    a.setSize(4);
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 4);
    CHECK_EQ(a[0], 123);
    CHECK_EQ(a[1], 234);
    CHECK_EQ(a[2], 345);
    CHECK_EQ(a[3], 456);
  }

  SUBCASE("Can Not Resize with Different Size")
  {
    Array<double> a(2);
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 2);

    a[0] = -3.2;
    a[1] = -5.4;
    CHECK_EQ(a[0], -3.2);
    CHECK_EQ(a[1], -5.4);

    CHECK_THROWS(a.setSize(4));
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 2);
    CHECK_EQ(a[0], -3.2);
    CHECK_EQ(a[1], -5.4);

    CHECK_THROWS(a.setSize(1));
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 2);
    CHECK_EQ(a[0], -3.2);
    CHECK_EQ(a[1], -5.4);
  }

  SUBCASE("Can Move-Construct")
  {
    Array<int> a(3);
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 3);
    a[0] = -9;
    a[1] = -8;
    a[2] = -7;
    CHECK_EQ(a[0], -9);
    CHECK_EQ(a[1], -8);
    CHECK_EQ(a[2], -7);

    decltype(auto) b(std::move(a));
    CHECK_UNARY_FALSE(b.empty());
    CHECK_EQ(b.size(), 3);
    CHECK_EQ(b[0], -9);
    CHECK_EQ(b[1], -8);
    CHECK_EQ(b[2], -7);
  }

  SUBCASE("Can Move-Assign to Same Size")
  {
    Array<std::string> a(4);
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 4);
    a[0] = "One";
    a[1] = "Two";
    a[2] = "Three";
    a[3] = "Four";
    CHECK_EQ(a[0], "One");
    CHECK_EQ(a[1], "Two");
    CHECK_EQ(a[2], "Three");
    CHECK_EQ(a[3], "Four");

    Array<std::string> b(4);
    CHECK_UNARY_FALSE(b.empty());
    CHECK_EQ(b.size(), 4);
    b[0] = "Un";
    b[1] = "Deux";
    b[2] = "Trois";
    b[3] = "Quatre";
    CHECK_EQ(b[0], "Un");
    CHECK_EQ(b[1], "Deux");
    CHECK_EQ(b[2], "Trois");
    CHECK_EQ(b[3], "Quatre");

    a = std::move(b);
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 4);
    CHECK_EQ(a[0], "Un");
    CHECK_EQ(a[1], "Deux");
    CHECK_EQ(a[2], "Trois");
    CHECK_EQ(a[3], "Quatre");
  }

  SUBCASE("Can Move-Assign to Empty")
  {
    Array<std::string> a(4);
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 4);
    a[0] = "Um";
    a[1] = "Dois";
    a[2] = "Três";
    a[3] = "Quatro";
    CHECK_EQ(a[0], "Um");
    CHECK_EQ(a[1], "Dois");
    CHECK_EQ(a[2], "Três");
    CHECK_EQ(a[3], "Quatro");

    Array<std::string> b;
    CHECK_UNARY(b.empty());
    CHECK_EQ(b.size(), 0);

    b = std::move(a);
    b[0] = "1";

    CHECK_UNARY_FALSE(b.empty());
    CHECK_EQ(b.size(), 4);
    CHECK_EQ(b[0], "1");
    CHECK_EQ(b[1], "Dois");
    CHECK_EQ(b[2], "Três");
    CHECK_EQ(b[3], "Quatro");
  }

  SUBCASE("Can Not Move-Assign to Different Size")
  {
    Array<std::string> a(2);
    a[0] = "Pejig";
    a[1] = "Nij";
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 2);
    CHECK_EQ(a[0], "Pejig");
    CHECK_EQ(a[1], "Nij");

    Array<std::string> b(1);
    CHECK_UNARY_FALSE(b.empty());
    CHECK_EQ(b.size(), 1);
    CHECK_THROWS(b = std::move(a));
    CHECK_NE(b[0], "Pejig");

    Array<std::string> c(100);
    CHECK_UNARY_FALSE(c.empty());
    CHECK_EQ(c.size(), 100);
    CHECK_THROWS(c = std::move(a));
    CHECK_NE(c[0], "Pejig");

    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 2);
    CHECK_EQ(a[0], "Pejig");
    CHECK_EQ(a[1], "Nij");
  }

  SUBCASE("operator=()")
  {
    Array<decltype(Rand()), NoConstructAllocator<decltype(Rand())>>
      a(static_cast<size_t>(100'000 + (Rand() % 100'000)));
    for (auto&& e : a)
      e = Rand();

    decltype(auto) b(a);
    CHECK_EQ(a.size(), b.size());
    CHECK_EQ(a, b);

    auto const index{ static_cast<size_t>(Rand() % 100'000) };
    if (a[index] > 0)
      --a[index];
    else
      a[index] = 1;
    CHECK_EQ(a.size(), b.size());
    CHECK_NE(a, b);
  }

  SUBCASE("Fill")
  {
    Array<char, NoConstructAllocator<char>> a(999);
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 999);

    a.fill('z');
    CHECK_UNARY_FALSE(a.empty());
    CHECK_EQ(a.size(), 999);

    size_t i{ 0 };
    for (; i != a.size(); ++i) {
      if (a[i] != 'z')
        break;
    }
    CHECK_EQ(i, a.size());
  }

  SUBCASE("Swap")
  {
    Array<decltype(Rand()), NoConstructAllocator<decltype(Rand())>> a(12345), b(12345);
    for (auto&& e : a) {
      e = Rand();
    }
    for (auto&& e : b) {
      e = Rand();
    }
    decltype(auto) aa(a);
    decltype(auto) bb(b);
    CHECK_EQ(a.size(), 12345);
    CHECK_EQ(aa.size(), 12345);
    CHECK_EQ(b.size(), 12345);
    CHECK_EQ(bb.size(), 12345);

    aa.swap(bb);
    CHECK_EQ(a, bb);
    CHECK_EQ(b, aa);

    swap(bb, aa);
    CHECK_EQ(aa, a);
    CHECK_EQ(bb, b);
  }

  SUBCASE("Swap Different Sizes")
  {
    {
      Array<decltype(Rand()), NoConstructAllocator<decltype(Rand())>> a(2345), b(2344);
      for (auto&& e : a) {
        e = Rand();
      }
      for (auto&& e : b) {
        e = Rand();
      }
      decltype(auto) aa(a);
      decltype(auto) bb(b);
      CHECK_EQ(a.size(), 2345);
      CHECK_EQ(aa.size(), 2345);
      CHECK_EQ(b.size(), 2344);
      CHECK_EQ(bb.size(), 2344);

      CHECK_THROWS(aa.swap(bb));
      CHECK_EQ(a, aa);
      CHECK_EQ(b, bb);

      CHECK_THROWS(swap(bb, aa));
      CHECK_EQ(aa, a);
      CHECK_EQ(bb, b);
    }

    {
      Array<double> a(2), b(3);
      CHECK_THROWS(a.swap(b));
      CHECK_THROWS(swap(b, a));
    }

    {
      Array<double> a(2), b;
      CHECK_THROWS(a.swap(b));
      CHECK_THROWS(swap(b, a));
    }

    {
      Array<double> a, b;
      a.swap(b);
      swap(b, a);
    }
  }
}

TEST_CASE("GoferThreadsPool" * doctest::timeout(5))
{
  // For debugging only.
  auto const displayWaitForErrandsToComplete{ []() { /* MESSAGE("Waiting for errands to complete..."); */ } };
  auto const displayErrandsCompleted{ []() { /* MESSAGE("Errands completed."); */ } };
  auto const displayWaitForThreadsToDie{ []() { /* MESSAGE("Waiting for threads to die..."); */ } };
  auto const displayThreadsDied{ []() { /* MESSAGE("Threads died."); */ } };

  SUBCASE("Empty")
  {
    {
      GoferThreadsPool p1(9);
      CHECK_EQ(p1.goferThreadsCount(), 9);
      CHECK_EQ(p1.errandsLeftCount(), 0);
      displayWaitForErrandsToComplete();
      p1.waitForAllErrandsToComplete();
      displayErrandsCompleted();
      CHECK_EQ(p1.errandsLeftCount(), 0);
      CHECK_EQ(p1.goferThreadsCount(), 9);

      GoferThreadsPool p2;
      CHECK_GT(p2.goferThreadsCount(), 0);
      CHECK_EQ(p2.errandsLeftCount(), 0);
      displayWaitForErrandsToComplete();
      p2.waitForAllErrandsToComplete();
      displayErrandsCompleted();
      CHECK_GT(p2.goferThreadsCount(), 0);
      CHECK_EQ(p2.errandsLeftCount(), 0);

      auto const r{ static_cast<decltype(std::thread::hardware_concurrency())>(100 + (Rand() % 100)) };
      GoferThreadsPool p3(r);
      CHECK_EQ(p3.goferThreadsCount(), r);
      CHECK_EQ(p3.errandsLeftCount(), 0);
      displayWaitForErrandsToComplete();
      p3.waitForAllErrandsToComplete();
      displayErrandsCompleted();
      CHECK_EQ(p3.errandsLeftCount(), 0);
      CHECK_EQ(p3.goferThreadsCount(), r);

      displayWaitForThreadsToDie();
    }
    displayThreadsDied();
  }

  SUBCASE("enQueueErrand()")
  {
    {
      GoferThreadsPool p(4);
      CHECK_EQ(p.goferThreadsCount(), 4);
      CHECK_EQ(p.errandsLeftCount(), 0);

      decltype(Rand()) r;
      constexpr static const decltype(r) x{ 10 };

      std::atomic<int> a(123);
      CHECK_EQ(a, 123);

      r = x + (Rand() % x);
      displayWaitForErrandsToComplete();
      CHECK_UNARY(p.enQueueErrand([&a, r]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(r));
        a = 234;
      }));
      CHECK_EQ(p.errandsLeftCount(), 1);
      p.waitForAllErrandsToComplete();
      CHECK_EQ(a, 234);
      displayErrandsCompleted();
      CHECK_EQ(p.errandsLeftCount(), 0);
      CHECK_EQ(p.goferThreadsCount(), 4);

      r = x + (Rand() % x);
      displayWaitForErrandsToComplete();
      CHECK_UNARY(p.enQueueErrand(GoferThreadsPool::ErrandProcedure([&a, r]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(r));
        a = 345;
      })));
      CHECK_EQ(p.errandsLeftCount(), 1);
      p.waitForAllErrandsToComplete();
      CHECK_EQ(a, 345);
      displayErrandsCompleted();
      CHECK_EQ(p.errandsLeftCount(), 0);
      CHECK_EQ(p.goferThreadsCount(), 4);

      r = x + (Rand() % x);
      auto e{ [&a, r]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(r));
        a = 456;
      } };
      displayWaitForErrandsToComplete();
      CHECK_UNARY(p.enQueueErrand(e));
      CHECK_EQ(p.errandsLeftCount(), 1);
      p.waitForAllErrandsToComplete();
      CHECK_EQ(a, 456);
      displayErrandsCompleted();
      CHECK_EQ(p.errandsLeftCount(), 0);
      CHECK_EQ(p.goferThreadsCount(), 4);

      r = x + (Rand() % x);
      auto const f{ GoferThreadsPool::ErrandProcedure([&a, r]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(r));
        a = 567;
      }) };
      displayWaitForErrandsToComplete();
      CHECK_UNARY(p.enQueueErrand(f));
      CHECK_EQ(p.errandsLeftCount(), 1);
      p.waitForAllErrandsToComplete();
      CHECK_EQ(a, 567);
      displayErrandsCompleted();
      CHECK_EQ(p.errandsLeftCount(), 0);
      CHECK_EQ(p.goferThreadsCount(), 4);

      displayWaitForErrandsToComplete();
      CHECK_UNARY(p.enQueueErrand(+[] { std::chrono::milliseconds(23); }));
      CHECK_EQ(p.errandsLeftCount(), 1);
      p.waitForAllErrandsToComplete();
      CHECK_EQ(a, 567);
      displayErrandsCompleted();
      CHECK_EQ(p.errandsLeftCount(), 0);
      CHECK_EQ(p.goferThreadsCount(), 4);

      displayWaitForThreadsToDie();
    }
    displayThreadsDied();
  }

  SUBCASE("enQueueErrand[s]() none")
  {
    {
      GoferThreadsPool p(19);
      CHECK_EQ(p.goferThreadsCount(), 19);
      CHECK_EQ(p.errandsLeftCount(), 0);

      CHECK_UNARY_FALSE(p.enQueueErrand(nullptr));
      CHECK_EQ(p.errandsLeftCount(), 0);
      CHECK_UNARY_FALSE(p.enQueueErrand(GoferThreadsPool::ErrandProcedure()));
      CHECK_EQ(p.errandsLeftCount(), 0);

      std::vector<std::function<void()>> errands;
      CHECK_EQ(errands.size(), 0);
      CHECK_EQ(p.enQueueErrands(errands, false), 0);
      CHECK_EQ(p.errandsLeftCount(), 0);

      errands.emplace_back(GoferThreadsPool::ErrandProcedure());
      errands.emplace_back();
      CHECK_EQ(errands.size(), 2);
      CHECK_EQ(p.enQueueErrands(errands), 0);
      CHECK_EQ(p.errandsLeftCount(), 0);

      displayWaitForErrandsToComplete();
      p.waitForAllErrandsToComplete();
      displayErrandsCompleted();
      CHECK_EQ(p.errandsLeftCount(), 0);
      CHECK_EQ(p.goferThreadsCount(), 19);

      displayWaitForThreadsToDie();
    }
    displayThreadsDied();
  }

  SUBCASE("enQueueErrands() one")
  {
    {
      GoferThreadsPool p(2);
      CHECK_EQ(p.goferThreadsCount(), 2);
      CHECK_EQ(p.errandsLeftCount(), 0);

      std::atomic<int> a{ 0 };
      CHECK_EQ(a, 0);
      decltype(Rand()) const r{ Rand() % 100 };
      int const b{ static_cast<int>(Rand()) % (std::numeric_limits<int>::max() - 1) };
      std::vector<std::function<void()>> errands;
      errands.emplace_back([&a, r, b]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(r));
        a = b;
      });
      CHECK_EQ(errands.size(), 1);
      displayWaitForErrandsToComplete();
      CHECK_EQ(p.enQueueErrands(errands, false), 1);
      p.waitForAllErrandsToComplete();
      displayErrandsCompleted();
      CHECK_EQ(a, b);
      CHECK_EQ(p.errandsLeftCount(), 0);
      CHECK_EQ(p.goferThreadsCount(), 2);

      displayWaitForThreadsToDie();
    }
    displayThreadsDied();
  }

  SUBCASE("enQueueErrands() many")
  {
    using Random = std::mt19937_64;
    using RandomValue = decltype(Random()());
    auto const VectorSizes{ static_cast<decltype(std::thread::hardware_concurrency())>(11 + (Rand() % 11)) };

    // Create vectors.
    std::vector<std::vector<RandomValue, NoConstructAllocator<RandomValue>>> vectors;
    std::vector<std::vector<RandomValue, NoConstructAllocator<RandomValue>>> vectorCopies;
    std::vector<Random> randoms;
    std::vector<std::function<void()>> errands;
    vectors.reserve(VectorSizes);
    vectorCopies.reserve(VectorSizes);
    randoms.reserve(VectorSizes);
    errands.reserve(VectorSizes);

    // Initialize vectors.
    for (std::decay_t<decltype(VectorSizes)> index{ 0 }; index != VectorSizes; ++index) {
      auto const size{ static_cast<size_t>(1 + (Rand() % 100'000)) };
      vectors.emplace_back(size);
      vectorCopies.emplace_back(size);
      randoms.emplace_back(Rand());
      errands.emplace_back([&, index]() {
        for (auto&& element : vectors[index])
          element = (randoms[index])();
      });

      // Initialize vector vectors.
      for (auto&& element : vectors.back())
        element = (randoms.back())();
      vectorCopies.back() = vectors.back();
      CHECK_EQ(vectors.back(), vectorCopies.back());
    }
    CHECK_EQ(errands.size(), VectorSizes);

    // Test three different pool sizes.
    auto const TestGoferThreadsPool{ [&](decltype(VectorSizes) const threadsCount)
    {
      {
        GoferThreadsPool p(threadsCount);
        if (threadsCount > 0)
          CHECK_EQ(p.goferThreadsCount(), threadsCount);
        CHECK_EQ(p.errandsLeftCount(), 0);

        displayWaitForErrandsToComplete();
        CHECK_EQ(p.enQueueErrands(errands, true), VectorSizes);
        p.waitForAllErrandsToComplete();
        displayErrandsCompleted();
        CHECK_EQ(p.errandsLeftCount(), 0);

        displayWaitForThreadsToDie();
      }
      displayThreadsDied();
      for (std::decay_t<decltype(VectorSizes)> index{ 0 }; index != VectorSizes; ++index) {
        CHECK_NE(vectors[index], vectorCopies[index]);
        vectors[index] = vectorCopies[index];
        CHECK_EQ(vectors[index], vectorCopies[index]);
      }
    } };
    TestGoferThreadsPool(0);
    TestGoferThreadsPool(1);
    TestGoferThreadsPool(2);
    TestGoferThreadsPool(3);
    TestGoferThreadsPool(VectorSizes / 2);
    TestGoferThreadsPool(VectorSizes * 2);
  }

  SUBCASE("Destroy Wait")
  {
    std::vector<std::function<void()>> errands;
    decltype(Rand()) r;
    std::atomic<int> a(0);
    for (int64_t i{ 0 }; i != 5; ++i) {
      r = 100 + (Rand() % 50);
      errands.emplace_back([&a, r]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(r));
        ++a;
      });
    }

    {
      GoferThreadsPool p(5);

      displayWaitForThreadsToDie();
      CHECK_EQ(p.enQueueErrands(errands, false), 5);
      // Wait for the 5 ++a errands to start executing.
      std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    displayThreadsDied();

    CHECK_EQ(a, 5);
  }

  SUBCASE("Destroy Wait Not")
  {
    constexpr static size_t const s{ 4 };

    std::vector<std::function<void()>> errands;
    errands.reserve(s);
    decltype(Rand()) r;
    for (size_t i{ 0 }; i != s; ++i) {
      r = 100 + (Rand() % 50);
      errands.emplace_back([r]() { std::this_thread::sleep_for(std::chrono::milliseconds(r)); });
    }

    {
      GoferThreadsPool p(2);

      displayWaitForThreadsToDie();
      CHECK_EQ(p.enQueueErrands(errands, false), s);

      /* Most often than not, no errand will run unless yield() below is un-commented.
         It would seem that the current thread has time to unlock the mutex (out of enQueueErrands() above)
         and re-lock it back right away (calling GoferThreadsPool's destructor getting out of the context below).
      */
      // std::this_thread::yield();
    }
    displayThreadsDied();
  }
}

TEST_CASE("RandomBoolean")
{
  static uint32_t const RandomBooleanIterations{ 50'000'000 + (static_cast<uint32_t>(Rand()) % 50'000'000) };
  static decltype(RandomBooleanIterations) const RandomBooleanHalf{ RandomBooleanIterations / 2 };
  static decltype(RandomBooleanIterations) const RandomBooleanDelta{ RandomBooleanIterations / 5'000 };
  static decltype(RandomBooleanIterations) const RandomBooleanMinimumHalf{ RandomBooleanHalf - RandomBooleanDelta };
  static decltype(RandomBooleanIterations) const RandomBooleanMaximumHalf{ RandomBooleanHalf + RandomBooleanDelta };

  SUBCASE("std::mt19937_64")
  {
    RandomBoolean b(std::make_shared<std::mt19937_64>(
      static_cast<std::mt19937_64::result_type>(std::chrono::high_resolution_clock::now().time_since_epoch().count())));
    std::decay_t<decltype(RandomBooleanIterations)> trues{ 0 };

    for (std::decay_t<decltype(RandomBooleanIterations)> i{ 0 }; i != RandomBooleanIterations; ++i)
      if (b())
        ++trues;

    CHECK_GT(trues, RandomBooleanMinimumHalf);
    CHECK_LT(trues, RandomBooleanMaximumHalf);
    CHECK_NE(trues, RandomBooleanHalf);
  }

  SUBCASE("std::mt19937")
  {
    auto r{ std::make_unique<std::mt19937>(
      static_cast<std::mt19937::result_type>(std::chrono::high_resolution_clock::now().time_since_epoch().count())) };
    RandomBoolean b(std::move(r));
    std::decay_t<decltype(RandomBooleanIterations)> trues{ 0 };

    for (std::decay_t<decltype(RandomBooleanIterations)> i{ 0 }; i != RandomBooleanIterations; ++i)
      if (b())
        ++trues;

    CHECK_GT(trues, RandomBooleanMinimumHalf);
    CHECK_LT(trues, RandomBooleanMaximumHalf);
    CHECK_NE(trues, RandomBooleanHalf);
  }

  SUBCASE("std::ranlux24_base")
  {
    RandomBoolean b(std::make_shared<std::ranlux24_base>(static_cast<std::ranlux24_base::result_type>(
      std::chrono::high_resolution_clock::now().time_since_epoch().count())));
    std::decay_t<decltype(RandomBooleanIterations)> trues{ 0 };

    for (std::decay_t<decltype(RandomBooleanIterations)> i{ 0 }; i != RandomBooleanIterations; ++i)
      if (b())
        ++trues;

    CHECK_GT(trues, RandomBooleanMinimumHalf);
    CHECK_LT(trues, RandomBooleanMaximumHalf);
    CHECK_NE(trues, RandomBooleanHalf);
  }

  SUBCASE("std::ranlux48_base")
  {
    auto r{ std::make_shared<std::ranlux48_base>(static_cast<std::ranlux48_base::result_type>(
      std::chrono::high_resolution_clock::now().time_since_epoch().count())) };
    RandomBoolean b(r);
    std::decay_t<decltype(RandomBooleanIterations)> trues{ 0 };

    for (std::decay_t<decltype(RandomBooleanIterations)> i{ 0 }; i != RandomBooleanIterations; ++i)
      if (b())
        ++trues;

    CHECK_GT(trues, RandomBooleanMinimumHalf);
    CHECK_LT(trues, RandomBooleanMaximumHalf);
    CHECK_NE(trues, RandomBooleanHalf);
  }
}
