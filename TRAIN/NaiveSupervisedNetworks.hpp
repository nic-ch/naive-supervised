// NaiveSupervisedNetworks.hpp

#pragma once

/** @file
    Naïve Matrix Digraphs.

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

#include "SupervisedNetworksBases.hpp"

/*
***********
** CLASS **
***********
*/

/// Crude geometric weights crafter.
class GeometricWeightsCrafter : public WeightsCrafter
{
  // DEFINITIONS //
public:
  using Numerator = double;

  // Must be < 1, arbitrarily determined by trial-and-error.
  constexpr static Numerator const AlteringsPNumeratorMultiplier{ 0.99 };
  constexpr static Numerator const myAlteringsMinimumPNumerator{ static_cast<Numerator>(0.1) };

  constexpr static Index const MaximumWeightDelta{ WeightsCardinality - 1 };
  // Arbitrarily determined by trial-and-error.
  constexpr static Index const MaximumWeightDeltaDelta{ MaximumWeightDelta / 1000 };

  // INSTANCE VARIABLES //
private:
  decltype(myWeights) myBestWeights;
  std::vector<Index, NoConstructAllocator<Index>> myAlterWeightsIndexes; // Terminated with InvalidIndex.
  std::vector<bool> myAlterDirections;

  Numerator const myAlteringsMaximumPNumerator{ static_cast<Numerator>(myWeightsCount) };
  Numerator myAlteringsPNumerator{ 0 };

  Index myMaximumWeightsInterval{ 0 };
  Index myMaximumWeightDelta{ 0 };

  bool myCrawlToLocalMaximum; // Trying up and down previously successful random alterations in increments of 1.
  bool myWeightsPreviouslyImproved;

  // DESTRUCTOR //
public:
  // Rule of five.
  ~GeometricWeightsCrafter() override = default;

  // CONSTRUCTORS //
public:
  /// Deleted.
  GeometricWeightsCrafter() = delete;

  explicit GeometricWeightsCrafter(Index const weightsCount)
    : WeightsCrafter(weightsCount)
    , myBestWeights(weightsCount)
    , myAlterWeightsIndexes(weightsCount + 1)
    , myAlterDirections(weightsCount)
  {
    rememberWeights();
    randomizeAlterings();
  }

  GeometricWeightsCrafter(GeometricWeightsCrafter const&) = default;

  /// Use #WeightsCrafterPointer, #ConstWeightsCrafterPointer and #clone() instead.
  GeometricWeightsCrafter(GeometricWeightsCrafter&&) = delete;

  // ASSIGNMENT OPERATORS //
public:
  /// Use #WeightsCrafterPointer, #ConstWeightsCrafterPointer and #clone() instead.
  GeometricWeightsCrafter& operator=(GeometricWeightsCrafter const&) = delete;
  /// Use #WeightsCrafterPointer, #ConstWeightsCrafterPointer and #clone() instead.
  GeometricWeightsCrafter& operator=(GeometricWeightsCrafter&&) = delete;

  // INSTANCE METHODS //
private:
  // Randomize which weights are to be altered as well as up or down.
  void randomizeAlterings()
    // noexcept if instantiating and calling a std::geometric_distribution are both noexcept.
    noexcept(noexcept(std::geometric_distribution<decltype((*myRandomIntegerPointer)())>()) and noexcept(
      std::geometric_distribution<decltype((*myRandomIntegerPointer)())>()(
        *myRandomIntegerPointer)) and noexcept((*myRandomIntegerPointer)()) and noexcept(myRandomBoolean()))
  {
    // Not crawling to local maximum (anymore).
    myCrawlToLocalMaximum = false;
    // Newly created alterings do not yet improve the weights.
    myWeightsPreviouslyImproved = false;

    // Decrease the P Numerator, or reset if too small.
    if ((myAlteringsPNumerator *= AlteringsPNumeratorMultiplier) < myAlteringsMinimumPNumerator)
      myAlteringsPNumerator = myAlteringsMaximumPNumerator * AlteringsPNumeratorMultiplier;

    /* Calculate a geometrically distributed maximum weight interval [1, weights count],
       according to the alterings P numerator.
    */
    std::geometric_distribution<decltype((*myRandomIntegerPointer)())> randomGeometricDistribution(
      myAlteringsPNumerator / myAlteringsMaximumPNumerator);
    if ((myMaximumWeightsInterval = static_cast<Index>(randomGeometricDistribution(*myRandomIntegerPointer) + 1)) >
        myWeightsCount)
      myMaximumWeightsInterval = myWeightsCount;

    Index index{ 0 };
    if (myMaximumWeightsInterval > 1) {
      // weightsIndex is initialized in interval [0, myWeightsCount).
      for (Index weightsIndex{ static_cast<Index>((*myRandomIntegerPointer)() % myMaximumWeightsInterval) };
           weightsIndex < myWeightsCount;
           // weightsIndex is incremented in interval [1, myWeightsCount].
           weightsIndex += static_cast<Index>(((*myRandomIntegerPointer)() % myMaximumWeightsInterval) + 1),
           ++index) {
        myAlterWeightsIndexes[index] = weightsIndex;
        myAlterDirections[index] = myRandomBoolean();
      }
    } else {
      for (; index != myWeightsCount; ++index) {
        myAlterWeightsIndexes[index] = index;
        myAlterDirections[index] = myRandomBoolean();
      }
    }
    myAlterWeightsIndexes[index] = InvalidIndex;
  }

  // Alter each weight its alter direction,
  bool alterWeights() noexcept(noexcept((*myRandomIntegerPointer)()))
  {
    bool noWeightWasAltered{ true };

    if (myCrawlToLocalMaximum) {
      for (Index index{ 0 }, weightsIndex; (weightsIndex = myAlterWeightsIndexes[index]) != InvalidIndex; ++index)
        // Increase weight by 1.
        if (myAlterDirections[index]) {
          if (myWeights[weightsIndex] < MaximumWeight) {
            ++myWeights[weightsIndex];
            noWeightWasAltered = false;
          }
          // Decrease weight by 1.
        } else {
          if (myWeights[weightsIndex] > MinimumWeight) {
            --myWeights[weightsIndex];
            noWeightWasAltered = false;
          }
        }
    } else {
      // Linearly decrement myMaximumWeightDelta, and cycle back.
      Index weightDeltaDelta{ static_cast<Index>((*myRandomIntegerPointer)() % MaximumWeightDeltaDelta) + 1 };
      if ((weightDeltaDelta + 2) > myMaximumWeightDelta)
        myMaximumWeightDelta = MaximumWeightDelta;
      else
        myMaximumWeightDelta -= weightDeltaDelta;

      WeightCalculator newWeight;
      for (Index index{ 0 }, weightsIndex; (weightsIndex = myAlterWeightsIndexes[index]) != InvalidIndex; ++index)
        // Increase weight.
        if (myAlterDirections[index]) {
          if (myWeights[weightsIndex] < MaximumWeight) {
            // Linearly randomize weight delta according to the maximum weight delta.
            newWeight = myWeights[weightsIndex] +
                        static_cast<WeightCalculator>((*myRandomIntegerPointer)() % myMaximumWeightDelta) + 1;
            if (newWeight >= MaximumWeight)
              myWeights[weightsIndex] = MaximumWeight;
            else
              myWeights[weightsIndex] = static_cast<Weight>(newWeight);

            noWeightWasAltered = false;
          }
          // Decrease weight.
        } else {
          if (myWeights[weightsIndex] > MinimumWeight) {
            // Linearly randomize weight delta according to the maximum weight delta.
            newWeight = myWeights[weightsIndex] -
                        static_cast<WeightCalculator>((*myRandomIntegerPointer)() % myMaximumWeightDelta) - 1;
            if (newWeight <= MinimumWeight)
              myWeights[weightsIndex] = MinimumWeight;
            else
              myWeights[weightsIndex] = static_cast<Weight>(newWeight);

            noWeightWasAltered = false;
          }
        }
    }

    return noWeightWasAltered;
  }

  void rememberWeights() noexcept
  {
    for (Index index{ 0 }; index != myWeightsCount; ++index)
      myBestWeights[index] = myWeights[index];
  }

  // IMPLEMENTED INTERFACE //
public:
  WeightsCrafterPointer clone() const override { return std::make_shared<std::decay_t<decltype(*this)>>(*this); }

  /** Bring back the best weights, as last call to #weightsImproved and #weightsDidNotImprove
      may have deteriorated them.
  */
  void bringBackBestWeights() noexcept override
  {
    for (Index index{ 0 }; index != myWeightsCount; ++index)
      myWeights[index] = myBestWeights[index];
  }

  /// The latest weights improved, re-alter accordingly.
  void weightsImproved() noexcept(
    noexcept(rememberWeights()) and noexcept(alterWeights()) and noexcept(randomizeAlterings())) override
  {
    rememberWeights();
    myWeightsPreviouslyImproved = true;

    /* Alter the weights similarly since they improved,
       or re-randomize the alterings until at least one weight gets altered.
    */
    while (alterWeights())
      randomizeAlterings();
  }

  /// The latest weights did not improve, re-alter accordingly.
  void weightsDidNotImprove() noexcept(
    noexcept(bringBackBestWeights()) and noexcept(randomizeAlterings()) and noexcept(alterWeights())) override
  {
    bringBackBestWeights();

    // Crawl to the local maximum around the lastly successful random alterations.
    if (myCrawlToLocalMaximum)
      /* Crawling to the local maximum just stopped improving the weights, so re-randomize the alterings.
         Ot alter directions were just reversed and did not improve the weights.
      */
      if (myWeightsPreviouslyImproved)
        randomizeAlterings();
      else
      // First crawling to the local maximum never improved the weights, so reverse the alter directions and try again.
      {
        // Reverse the crawling alter directions.
        for (Index index{ 0 }; myAlterWeightsIndexes[index] != InvalidIndex; ++index)
          myAlterDirections[index] = not myAlterDirections[index];

        // Ensure reversing the alter directions is done only once.
        myWeightsPreviouslyImproved = true;
      }
    else
      // Alterings stopped improving the weights, so start to crawl the local maximum in the same alter directions.
      if (myWeightsPreviouslyImproved) {
        // Start the crawling.
        myCrawlToLocalMaximum = true;
        // Not yet started crawling do not improve the weights.
        myWeightsPreviouslyImproved = false;
      }
      // The current alterings did not improve the weights, so re-randomize the alterings.
      else
        randomizeAlterings();

    // Alter the weights again, or reset the alterings until at least one weight gets altered.
    while (alterWeights())
      randomizeAlterings();
  }

  /// Log useful informations about the current state.
  void logCurrentState(Logger& logger) const override
  {
    logger << "Maximum weight delta is " << myMaximumWeightDelta << '/' << MaximumWeightDelta
           << ". Maximum interval is " << myMaximumWeightsInterval << '/' << myWeightsCount << ".\n";
  }
};

/*
***********
** CLASS **
***********
*/

/// Logarithmicly decreasing network.
class LogarithmicMatrixDigraph : public MatrixDigraph
{
  // INSTANCE VARIABLES //
private:
  std::vector<Value, NoConstructAllocator<Value>> myValues;

  // DESTRUCTOR //
public:
  // Rule of five.
  ~LogarithmicMatrixDigraph() override = default;

  // CONSTRUCTORS //
public:
  /// Deleted.
  LogarithmicMatrixDigraph() = delete;

  explicit LogarithmicMatrixDigraph(Index const rowsCount, Index const columnsCount)
    : MatrixDigraph(rowsCount, columnsCount)
  {
    // Compute the internal layers' value counts, first internal layer covers the input rows twice.
    Index valuesCount{ 0 };
    for (Index layerValuesCount{ rowsCount * 2 }; layerValuesCount;) {
      valuesCount += layerValuesCount;
      /* Match each two ingress values to one egress value.
         If the layer value count is odd, then match the last ingress value to the last egress value.
         e.g. 10 -> (10+1)÷2 = 5 -> (5+1)÷2 = 3 -> (3+1)÷2 = 2 -> (2+1)÷2 = 1 -> 0.
      */
      layerValuesCount = (1 == layerValuesCount) ? 0 : (layerValuesCount + 1) / 2;
    }

    // Resize the internal values vector.
    myValues.resize(valuesCount);

    // Minus the final unique sink value.
    myRequiredWeightsCount = (myInputsCount * 2) + valuesCount - 1;
  }

  LogarithmicMatrixDigraph(LogarithmicMatrixDigraph const&) = default;

  /// Use #MatrixDigraphPointer and #clone() instead.
  LogarithmicMatrixDigraph(LogarithmicMatrixDigraph&&) = delete;

  // ASSIGNMENT OPERATORS //
protected:
  /// Use #MatrixDigraphPointer and #clone() instead.
  LogarithmicMatrixDigraph& operator=(LogarithmicMatrixDigraph&&) = delete;
  /// Use #MatrixDigraphPointer and #clone() instead.
  LogarithmicMatrixDigraph& operator=(LogarithmicMatrixDigraph const&) = delete;

  // IMPLEMENTED INTERFACE //
public:
  MatrixDigraphPointer clone() const override { return std::make_unique<std::decay_t<decltype(*this)>>(*this); }

  /** Apply the weights starting from the inputs, then to the values serially layer by layer.
      About 98% of all the computing is done in this function.
      Calculate first the input layer into the first internal values layer,
      then each internal values layer into the next one down, down to the unique sink output value.
  */
  void applyWeights() noexcept override
  {
    Value result;
    Index ingressIndex{ 0 };
    Index weightsIndex{ 0 };
    Index egressIndex{ 0 };
    while (ingressIndex != myInputsCount) {
      auto const afterLastIngressIndex{ ingressIndex + myColumnsCount };

      // myColumnsCount ingress values to the first egress value.
      for (result = 0; ingressIndex != afterLastIngressIndex; ++ingressIndex, ++weightsIndex)
        result += myInputs[ingressIndex] * (*myWeightsCrafterPointer)[weightsIndex];
      myValues[egressIndex++] = result;

      // Exact same myColumnsCount ingress values (as above) to the second egress value.
      for (result = 0, ingressIndex -= myColumnsCount; ingressIndex != afterLastIngressIndex;
           ++ingressIndex, ++weightsIndex)
        result += myInputs[ingressIndex] * (*myWeightsCrafterPointer)[weightsIndex];
      myValues[egressIndex++] = result;
    }

    /* Calculate twice each myColumnsCount ingress input to an egress value.
       At most, ingress inputs occupy 16 bits, weights occupy 16 bits, myColumnsCount is 3 bits.
       First (internal) layer values will thus occupy at most 16 + 16 + 3 = 35 bits.
       Each extra layer will occupy at most an extra 16 (weight) + 1 (two ingress per one egress value) bits.
    */
    constexpr static unsigned int const ShiftCount{ 15 };

    ingressIndex = 0;
    /* For a 5 by 5 matrix, we would get the following, | means out of the inner loop.
       myInputsCount		5
       valuesCount, myValues	21
       myRequiredWeightsCount	70

       Values per layer:  10                   5              3           2        1
       ingressIndex        0  2  4  6  8 |10   10 12 |14:15   15 |17:18   18 |20   |20
       ingressLastIndex    9                   14             17          19       |20
       weightsIndex       50 52 54 56 58 |60   60 62 |64:65   65 |67:68   68 |70   |70
       egressIndex        10 11 12 13 14 |15   15 16 |17:18   18 |19:20   20 |21   |21

       The end is reached when ingressIndex comes to be the same as ingressLastIndex.
    */
    for (Index ingressLastIndex{ egressIndex - 1 }; ingressIndex != ingressLastIndex;
         ingressLastIndex = egressIndex - 1) {
      // Increment the three indexes in the for loop and not inside [] in case of reordering and unrolling.
      for (; ingressIndex < ingressLastIndex; ingressIndex += 2, weightsIndex += 2, ++egressIndex)
        // A positive number decrease-shifted converges to 0, a negative number converges to -1 (2's complement).
        myValues[egressIndex] =
          ((myValues[ingressIndex] * (*myWeightsCrafterPointer)[weightsIndex]) +
           (myValues[ingressIndex + 1] * (*myWeightsCrafterPointer)[weightsIndex + 1])) SHIFT_DECREASE ShiftCount;
      /*
      if ((result = (myValues[ingressIndex] * (*myWeightsCrafterPointer)[weightsIndex]) +
                    (myValues[ingressIndex + 1] * (*myWeightsCrafterPointer)[weightsIndex + 1])) >= 0)
        myValues[egressIndex] = result SHIFT_DECREASE ShiftCount;
      else
        myValues[egressIndex] = -((-result) SHIFT_DECREASE ShiftCount);
      */

      // If ingressIndex is ingressLastIndex then this last lonely ingress value goes to the last egress value.
      if (ingressIndex == ingressLastIndex)
        // A positive number decrease-shifted converges to 0, a negative number converges to -1 (2's complement).
        myValues[egressIndex++] =
          (myValues[ingressIndex++] * (*myWeightsCrafterPointer)[weightsIndex++]) SHIFT_DECREASE ShiftCount;
      /*
      if ((result = myValues[ingressIndex++] * (*myWeightsCrafterPointer)[weightsIndex++]) >= 0)
        myValues[egressIndex++] = result SHIFT_DECREASE ShiftCount;
      else
        myValues[egressIndex++] = -((-result) SHIFT_DECREASE ShiftCount);
      */
    }
  }

  Value uniqueSinkValue() const noexcept(noexcept(myValues.back())) override { return myValues.back(); }
};
