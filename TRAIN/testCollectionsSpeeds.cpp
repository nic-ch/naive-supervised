// testCollectionsSpeeds.cpp

/** @file
    Test Array speed compared to other collection types.

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

    @date 2022
*/

/*
**************
** INCLUDES **
**************
*/

#include "Utilities.hpp"

/*
*****************
** DEFINITIONS **
*****************
*/

using ValueType = int;
using RandomEngineType = std::mt19937_64;
// using RandomEngineType = std::mt19937;

/*
*************
** CLASSES **
*************
*/

template<typename CollectionType>
class Container
{
private:
  CollectionType myValues;

public:
  explicit Container(size_t size)
    : myValues(size)
  {
  }
  auto& values() { return myValues; }
  auto const& constValues() const { return myValues; }

  decltype(auto) size() noexcept(noexcept(myValues.size())) { return myValues.size(); }
  decltype(auto) operator[](size_t const index) noexcept(noexcept(myValues.operator[](index)))
  {
    return myValues[index];
  }
  decltype(auto) operator[](size_t const index) const noexcept(noexcept(myValues.operator[](index)))
  {
    return myValues[index];
  }
};

/*
****************
** PROCEDURES **
****************
*/

static void
PrintTimer(char const* const message, Timer& timer, long int const sum)
{
  std::cout << "  ";
  if (sum)
    std::cout << "Sum is " << sum << ". ";
  std::cout << message << " took " << timer << ".\n" << std::flush;
}

static void
TestSpeeds(size_t const totalIterations, size_t collectionSize)
{
  std::cout << std::boolalpha << std::fixed << std::setprecision(2) << "▒▒ TestSpeeds(" << totalIterations << ", "
            << collectionSize << ") START\n"
            << std::flush;
  {
    constexpr ValueType const maximum{ 1000 };
    constexpr ValueType const range{ (2 * maximum) + 1 };
    size_t const loopsCount{ totalIterations / collectionSize };
    int64_t sum{ 0 };

    RandomEngineType randomInteger(
      static_cast<decltype(randomInteger())>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
    Timer timer;

    std::cout << "Creating collections of size " << collectionSize << ":\n" << std::flush;

    timer.restart();
    auto a{ new ValueType[collectionSize] };
    auto b{ new ValueType[collectionSize] };
    auto c{ new ValueType[collectionSize] };
    timer.lap();
    PrintTimer("C-array: a, b, c", timer, sum);

    timer.restart();
    std::vector<ValueType, NoConstructAllocator<ValueType>> va(collectionSize);
    std::vector<ValueType, NoConstructAllocator<ValueType>> vb(collectionSize);
    std::vector<ValueType, NoConstructAllocator<ValueType>> vc(collectionSize);
    timer.lap();
    PrintTimer("std::vector: va, vb, vc", timer, sum);

    timer.restart();
    Array<ValueType, NoConstructAllocator<ValueType>> aa(collectionSize);
    Array<ValueType, NoConstructAllocator<ValueType>> ab(collectionSize);
    Array<ValueType, NoConstructAllocator<ValueType>> ac(collectionSize);
    timer.lap();
    PrintTimer("Array: aa, ab, ac", timer, sum);

    timer.restart();
    auto ua{ std::make_unique<Array<ValueType, NoConstructAllocator<ValueType>>>(collectionSize) };
    auto ub{ std::make_unique<Array<ValueType, NoConstructAllocator<ValueType>>>(collectionSize) };
    auto uc{ std::make_unique<Array<ValueType, NoConstructAllocator<ValueType>>>(collectionSize) };
    timer.lap();
    PrintTimer("unique_ptr<Array>: ua, ub, uc", timer, sum);

    timer.restart();
    auto sa{ std::make_shared<Array<ValueType, NoConstructAllocator<ValueType>>>(collectionSize) };
    auto sb{ std::make_shared<Array<ValueType, NoConstructAllocator<ValueType>>>(collectionSize) };
    auto sc{ std::make_shared<Array<ValueType, NoConstructAllocator<ValueType>>>(collectionSize) };
    timer.lap();
    PrintTimer("shared_ptr<Array>: sa, sb, sc", timer, sum);

    timer.restart();
    auto caa{ std::make_shared<Container<Array<ValueType, NoConstructAllocator<ValueType>>>>(collectionSize) };
    auto cab{ std::make_shared<Container<Array<ValueType, NoConstructAllocator<ValueType>>>>(collectionSize) };
    auto cac{ std::make_shared<Container<Array<ValueType, NoConstructAllocator<ValueType>>>>(collectionSize) };
    timer.lap();
    PrintTimer("shared_ptr<Container<Array>>: caa, cab cac", timer, sum);

    timer.restart();
    auto cva{ std::make_shared<Container<std::vector<ValueType, NoConstructAllocator<ValueType>>>>(collectionSize) };
    auto cvb{ std::make_shared<Container<std::vector<ValueType, NoConstructAllocator<ValueType>>>>(collectionSize) };
    auto cvc{ std::make_shared<Container<std::vector<ValueType, NoConstructAllocator<ValueType>>>>(collectionSize) };
    timer.lap();
    PrintTimer("shared_ptr<Container<std::vector>>: cva, cvb cvc", timer, sum);

    std::cout << "Random-initializing:\n" << std::flush;

    timer.restart();
    for (size_t i{ 0 }; i != collectionSize; ++i)
      a[i] = (randomInteger() % range) - maximum;
    for (size_t i{ 0 }; i != collectionSize; ++i)
      b[i] = (randomInteger() % range) - maximum;
    timer.lap();
    PrintTimer("a, b", timer, sum);

    timer.restart();
    for (auto&& e : va)
      e = (randomInteger() % range) - maximum;
    for (auto&& e : vb)
      e = (randomInteger() % range) - maximum;
    timer.lap();
    PrintTimer("va, vb", timer, sum);

    timer.restart();
    for (auto&& e : aa)
      e = (randomInteger() % range) - maximum;
    for (auto&& e : ab)
      e = (randomInteger() % range) - maximum;
    timer.lap();
    PrintTimer("aa, ab", timer, sum);

    timer.restart();
    for (auto&& e : *sa)
      e = (randomInteger() % range) - maximum;
    for (auto&& e : *sb)
      e = (randomInteger() % range) - maximum;
    timer.lap();
    PrintTimer("sa, sb", timer, sum);

    timer.restart();
    for (auto&& e : *ua)
      e = (randomInteger() % range) - maximum;
    for (auto&& e : *ub)
      e = (randomInteger() % range) - maximum;
    timer.lap();
    PrintTimer("ua, ub", timer, sum);

    timer.restart();
    for (auto&& e : caa->values())
      e = (randomInteger() % range) - maximum;
    for (auto&& e : cab->values())
      e = (randomInteger() % range) - maximum;
    timer.lap();
    PrintTimer("caa, cab", timer, sum);

    timer.restart();
    for (auto&& e : cva->values())
      e = (randomInteger() % range) - maximum;
    for (auto&& e : cvb->values())
      e = (randomInteger() % range) - maximum;
    timer.lap();
    PrintTimer("cva, cvb", timer, sum);

    std::cout << "Looping " << loopsCount << " times:\n" << std::flush;

    timer.restart();
    for (std::decay_t<decltype(loopsCount)> j{ 0 }; j != loopsCount; ++j) {
      a[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      b[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      for (size_t i{ 0 }; i != collectionSize; ++i)
        c[i] = a[i] + b[i];
      sum += c[randomInteger() % collectionSize];
    }
    timer.lap();
    PrintTimer("c[i] = a[i] + b[i]", timer, sum);

    {
      timer.restart();
      auto const constA{ a };
      auto const constB{ b };
      auto const constC{ c };
      for (std::decay_t<decltype(loopsCount)> j{ 0 }; j != loopsCount; ++j) {
        constA[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
        constB[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
        for (size_t i{ 0 }; i != collectionSize; ++i)
          constC[i] = constA[i] + constB[i];
        sum += constC[randomInteger() % collectionSize];
      }
      timer.lap();
      PrintTimer("constC[i] = constA[i] + constB[i]", timer, sum);
    }

    timer.restart();
    for (std::decay_t<decltype(loopsCount)> j{ 0 }; j != loopsCount; ++j) {
      va[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      vb[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      for (decltype(vc)::size_type i{ 0 }; i != va.size(); ++i)
        vc[i] = va[i] + vb[i];
      sum += vc[randomInteger() % collectionSize];
    }
    timer.lap();
    PrintTimer("vc[i] = va[i] + vb[i]", timer, sum);

    {
      timer.restart();
      auto const constVa{ va.data() };
      auto const constVb{ vb.data() };
      auto const constVc{ vc.data() };
      for (std::decay_t<decltype(loopsCount)> j{ 0 }; j != loopsCount; ++j) {
        constVa[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
        constVb[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
        for (decltype(vc)::size_type i{ 0 }; i != collectionSize; ++i)
          constVc[i] = constVa[i] + constVb[i];
        sum += constVc[randomInteger() % collectionSize];
      }
      timer.lap();
      PrintTimer("constVc[i] = constVa[i] + constVb[i]", timer, sum);
    }

    timer.restart();
    for (std::decay_t<decltype(loopsCount)> j{ 0 }; j != loopsCount; ++j) {
      aa[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      ab[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      for (decltype(ac)::size_type i{ 0 }; i != aa.size(); ++i)
        ac[i] = aa[i] + ab[i];
      sum += ac[randomInteger() % collectionSize];
    }
    timer.lap();
    PrintTimer("ac[i] = aa[i] + ab[i]", timer, sum);

    timer.restart();
    for (std::decay_t<decltype(loopsCount)> j{ 0 }; j != loopsCount; ++j) {
      (*ua)[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      (*ub)[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      for (std::decay_t<decltype(*uc)>::size_type i{ 0 }; i != ua->size(); ++i)
        (*uc)[i] = (*ua)[i] + (*ub)[i];
      sum += (*uc)[randomInteger() % collectionSize];
    }
    timer.lap();
    PrintTimer("(*uc)[i] = (*ua)[i] + (*ub)[i]", timer, sum);

    timer.restart();
    for (std::decay_t<decltype(loopsCount)> j{ 0 }; j != loopsCount; ++j) {
      (*sa)[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      (*sb)[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      for (std::decay_t<decltype(*sc)>::size_type i{ 0 }; i != sa->size(); ++i)
        (*sc)[i] = (*sa)[i] + (*sb)[i];
      sum += (*sc)[randomInteger() % collectionSize];
    }
    timer.lap();
    PrintTimer("(*sc)[i] = (*sa)[i] + (*sb)[i]", timer, sum);

    timer.restart();
    for (std::decay_t<decltype(loopsCount)> j{ 0 }; j != loopsCount; ++j) {
      aa[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      ab[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      for (std::decay_t<decltype(*uc)>::size_type i{ 0 }; i != aa.size(); ++i)
        (*sc)[i] = aa[i] + ab[i];
      sum += (*sc)[randomInteger() % collectionSize];
    }
    timer.lap();
    PrintTimer("(*sc)[i] = aa[i] + ab[i]", timer, sum);

    timer.restart();
    for (std::decay_t<decltype(loopsCount)> j{ 0 }; j != loopsCount; ++j) {
      aa[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      ab[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      for (decltype(ac)::size_type i{ 0 }; i != collectionSize; ++i)
        ac[i] = aa[i] + ab[i];
      sum += ac[randomInteger() % collectionSize];
    }
    timer.lap();
    PrintTimer("ac[i] = aa[i] + ab[i] for const i", timer, sum);

    {
      timer.restart();
      auto const constAa{ aa.data() };
      auto const constUb{ ub->data() };
      auto const constSc{ sc->data() };
      for (std::decay_t<decltype(loopsCount)> j{ 0 }; j != loopsCount; ++j) {
        constAa[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
        constUb[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
        for (std::decay_t<decltype(*sc)>::size_type i{ 0 }; i != collectionSize; ++i)
          constSc[i] = constAa[i] + constUb[i];
        sum += constSc[randomInteger() % collectionSize];
      }
      timer.lap();
      PrintTimer("constSc[i] = constAa[i] + constUb[i]", timer, sum);
    }

    timer.restart();
    for (std::decay_t<decltype(loopsCount)> j{ 0 }; j != loopsCount; ++j) {
      (caa->values())[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      (cab->values())[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      for (size_t i{ 0 }; i != caa->constValues().size(); ++i)
        (cac->values())[i] = (caa->constValues())[i] + (cab->constValues())[i];
      sum += (cac->values())[randomInteger() % collectionSize];
    }
    timer.lap();
    PrintTimer("(cac->values())[i] = (caa->constValues())[i] + (cab->constValues())[i]", timer, sum);

    timer.restart();
    for (std::decay_t<decltype(loopsCount)> j{ 0 }; j != loopsCount; ++j) {
      (cva->values())[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      (cvb->values())[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      for (size_t i{ 0 }; i != cva->constValues().size(); ++i)
        (cvc->values())[i] = (cva->constValues())[i] + (cvb->constValues())[i];
      sum += (cvc->values())[randomInteger() % collectionSize];
    }
    timer.lap();
    PrintTimer("(cvc->values())[i] = (cva->constValues())[i] + (cvb->constValues())[i]", timer, sum);

    timer.restart();
    for (std::decay_t<decltype(loopsCount)> j{ 0 }; j != loopsCount; ++j) {
      (*caa)[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      (*cab)[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      for (size_t i{ 0 }; i != caa->size(); ++i)
        (*cac)[i] = (*caa)[i] + (*cab)[i];
      sum += (*cac)[randomInteger() % collectionSize];
    }
    timer.lap();
    PrintTimer("(*cac)[i] = (*caa)[i] + (*cab)[i]", timer, sum);

    timer.restart();
    for (std::decay_t<decltype(loopsCount)> j{ 0 }; j != loopsCount; ++j) {
      (*cva)[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      (*cvb)[randomInteger() % collectionSize] = (randomInteger() % range) - maximum;
      for (size_t i{ 0 }; i != cva->size(); ++i)
        (*cvc)[i] = (*cva)[i] + (*cvb)[i];
      sum += (*cvc)[randomInteger() % collectionSize];
    }
    timer.lap();
    PrintTimer("(*cvc)[i] = (*cva)[i] + (*cvb)[i]", timer, sum);

    delete[] c;
    delete[] b;
    delete[] a;
  }
  std::cout << "▒▒ TestSpeeds(" << totalIterations << ", " << collectionSize << ") END\n\n" << std::flush;
}

/*
**********
** MAIN **
**********
*/

int
main()
{
  constexpr size_t const TotalIterations{ 2'000'000'000 };

  TestSpeeds(TotalIterations, 100);
  //  TestSpeeds(TotalIterations, 333);
  TestSpeeds(TotalIterations, 1000);
  //  TestSpeeds(TotalIterations, 3333);
  TestSpeeds(TotalIterations, 10'000);
  //  TestSpeeds(TotalIterations, 33'333);
  TestSpeeds(TotalIterations, 100'000);
  //  TestSpeeds(TotalIterations, 333'333);
  TestSpeeds(TotalIterations, 1000'000);
}
