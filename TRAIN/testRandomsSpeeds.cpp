// testRandomsSpeeds.cpp

/** @file
    Test speeds of different random engines and and used in RandomBoolean.

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
****************
** PROCEDURES **
****************
*/

template<typename Random, typename RandomName>
void
testRandom(RandomName const& randomName)
{
  constexpr static uint32_t const Iterations{ 1000'000'000 };
  Random random(
    static_cast<typename Random::result_type>(std::chrono::high_resolution_clock::now().time_since_epoch().count()));
  std::decay_t<decltype(Iterations)> ones{ 0 };

  Timer timer;
  for (std::decay_t<decltype(Iterations)> i{ 0 }; i != Iterations; ++i)
    if (random() % 2)
      ++ones;
  timer.lap();
  std::cout << Iterations << " iteration of " << randomName << " mod 2 produced " << ones << " ones and took " << timer
            << ".\n";
}

template<typename Random, typename RandomName>
void
testRandomBoolean(RandomName const& randomName)
{
  constexpr static uint32_t const Iterations{ 1000'000'000 };
  RandomBoolean randomBoolean(std::make_unique<Random>(
    static_cast<typename Random::result_type>(std::chrono::high_resolution_clock::now().time_since_epoch().count())));
  std::decay_t<decltype(Iterations)> trues{ 0 };

  Timer timer;
  for (std::decay_t<decltype(Iterations)> i{ 0 }; i != Iterations; ++i)
    if (randomBoolean())
      ++trues;
  timer.lap();
  std::cout << Iterations << " iteration of RandomBoolean<" << randomName << "> produced " << trues
            << " trues and took " << timer << ".\n";
}

/*
**********
** MAIN **
**********
*/

int
main()
{
  testRandom<std::mt19937_64>("std::mt19937_64");
  testRandom<std::mt19937>("std::mt19937");
  // testRandom<std::ranlux24_base>("std::ranlux24_base");
  testRandom<std::ranlux48_base>("std::ranlux48_base");

  testRandomBoolean<std::mt19937_64>("std::mt19937_64");
  testRandomBoolean<std::mt19937>("std::mt19937");
  // testRandomBoolean<std::ranlux24_base>("std::ranlux24_base");
  testRandomBoolean<std::ranlux48_base>("std::ranlux48_base");
}
