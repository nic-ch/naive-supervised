// SupervisedNetworksBases.hpp

#pragma once

/** @file
    Naïve Supervised Networks Base Classes.

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

#include <csignal>
#include <map>

#include "Utilities.hpp"

/*
*****************
** DEFINITIONS **
*****************
*/

using Index = uint32_t;
constexpr static Index const InvalidIndex{ std::numeric_limits<Index>::max() };

/*
****************
** BASE CLASS **
****************
*/

/// Abstract base class for all the weights crafting classes. NOT thread safe. Initial weights are randomized.
class WeightsCrafter
{
  // DEFINITIONS //
public:
  using Weight = int16_t;
  using WeightsCrafterPointer = std::shared_ptr<WeightsCrafter>;
  using ConstWeightsCrafterPointer = std::shared_ptr<WeightsCrafter const>;
  using WeightsCrafterInstantiator = std::function<WeightsCrafterPointer(Index const weightsCount)>;

protected:
  using WeightCalculator = int32_t;
  constexpr static WeightCalculator const MinimumWeight{ std::numeric_limits<Weight>::min() };
  constexpr static WeightCalculator const MaximumWeight{ std::numeric_limits<Weight>::max() };
  constexpr static WeightCalculator const WeightsCardinality{ MaximumWeight + 1 - MinimumWeight };

  // INSTANCE VARIABLES //
protected:
  std::shared_ptr<std::mt19937_64> myRandomIntegerPointer;
  RandomBoolean<decltype(myRandomIntegerPointer)> myRandomBoolean;
  Index myWeightsCount;
  std::vector<Weight, NoConstructAllocator<Weight>> myWeights;

  // DESTRUCTOR //
public:
  // Base class.
  virtual ~WeightsCrafter() = default;

  // PRIVATE INSTANCE METHODS //
private:
  decltype(auto) currentTimeSeed() const
    noexcept(noexcept(std::chrono::high_resolution_clock::now().time_since_epoch().count()))
  {
    return static_cast<std::decay_t<decltype(*myRandomIntegerPointer)>::result_type>(
      std::chrono::high_resolution_clock::now().time_since_epoch().count());
  }

  // CONSTRUCTORS //
  // Base class.
protected:
  /// Deleted.
  WeightsCrafter() = delete;

  explicit WeightsCrafter(Index const weightsCount)
    : myRandomIntegerPointer(std::make_shared<std::decay_t<decltype(*myRandomIntegerPointer)>>(currentTimeSeed()))
    , myRandomBoolean(myRandomIntegerPointer)
    , myWeightsCount(weightsCount)
    , myWeights(myWeightsCount)
  {
    // Linearly randomize weights. Cast operands first as WeightCalculator, then cast result back as Weight.
    for (auto&& weight : myWeights)
      weight = static_cast<Weight>(static_cast<WeightCalculator>((*myRandomIntegerPointer)() % WeightsCardinality) +
                                   MinimumWeight);
  }

  /// The random variables are copied as is and NOT re-seeded. Use #reSeedRandomVariable if needed.
  WeightsCrafter(WeightsCrafter const&) = default;

  /// Use #WeightsCrafterPointer, #ConstWeightsCrafterPointer and #clone() instead.
  WeightsCrafter(WeightsCrafter&&) = delete;

  // ASSIGNMENT OPERATORS //
  // Base class.
protected:
  /// Use #WeightsCrafterPointer, #ConstWeightsCrafterPointer and #clone() instead.
  WeightsCrafter& operator=(WeightsCrafter const&) = delete;
  /// Use #WeightsCrafterPointer, #ConstWeightsCrafterPointer and #clone() instead.
  WeightsCrafter& operator=(WeightsCrafter&&) = delete;

  // PUBLIC INSTANCE METHODS //
public:
  void reSeedRandomVariable() { myRandomIntegerPointer->seed(currentTimeSeed()); }

  /// @return True on success, else false, and log error.
  bool readWeightsFromFile(Logger& logger, decltype(OpenInputBinaryFileNamed(""))& weightsFileStatus)
  {
    auto& [weightsFile, errorMessage, weightsFileSize]{ weightsFileStatus };

    // Validate that the weights file is the right size.
    auto const requiredWeightsFileSize{ static_cast<decltype(weightsFileSize)>(myWeightsCount *
                                                                               sizeof(myWeights[0])) };
    if (weightsFileSize != requiredWeightsFileSize) {
      logger.error();
      logger << "Weights file is of size " << weightsFileSize << " bytes but must be of size "
             << requiredWeightsFileSize << " bytes for " << myWeightsCount<< " weights each of size "
             << sizeof(myWeights[0]) << " bytes.\n\n";
      return false;
    }

    // Load weights from the weights file.
    weightsFile.read(reinterpret_cast<decltype(weightsFile)::char_type*>(myWeights.data()), requiredWeightsFileSize);
    if (weightsFile.good())
      logger << myWeightsCount << " weights were loaded.\n";
    else {
      logger.streamCondition(weightsFile) << "Reading weights file.\n\n";
      return false;
    }

    return true;
  }

  /** Output the weights to file "Weights<name>_<date>_<time>.<bit weight size>w<weights count>".
      @return The file name if successful, else an empty string, and log error.
  */
  decltype(auto) writeWeightsToFile(Logger& logger)
  {
    // Compose the file name.
    std::ostringstream weightsFileNameStream;
    weightsFileNameStream << "WEIGHTS_";

    struct tm dateAndTime;
    auto const timeValue{ std::time(nullptr) };
    if (auto const time{ localtime_r(&timeValue, &dateAndTime) })
      weightsFileNameStream << std::put_time(time, "%Y-%m-%d_%H-%M-%S");
    else
      weightsFileNameStream << (*myRandomIntegerPointer)();
    weightsFileNameStream << '.' << (8 * sizeof(myWeights[0])) << 'w' << myWeightsCount;

    auto weightsFileName{ weightsFileNameStream.str() };
    // Open file named weightsFileName for writing.
    std::ofstream weightsFile(weightsFileName, std::ios::binary | std::ios::trunc);
    if (weightsFile.good()) {
      weightsFile.write(reinterpret_cast<decltype(weightsFile)::char_type const*>(myWeights.data()),
                        static_cast<std::streamsize>(myWeightsCount * sizeof(myWeights[0])));
      if (weightsFile.good())
        logger << myWeightsCount << " weights were written to file '" << weightsFileName << "'.\n";
      else {
        logger.streamCondition(weightsFile) << "Writing to file '" << weightsFileName << "'.\n\n";
        weightsFileName.clear();
      }
    } else {
      logger.streamCondition(weightsFile) << "Can not create/open file '" << weightsFileName << "' for writing.\n\n";
      weightsFileName.clear();
    }

    return weightsFileName;
  }

  decltype(auto) weightsCount() const noexcept { return myWeightsCount; }
  decltype(auto) operator[](Index const index) const noexcept(noexcept(myWeights[index])) { return myWeights[index]; }

  // ABSTRACT INTERFACE //

  /* Strategy pattern. The present base class is not implemented as a Template Method pattern (or NVI),
     as there shall be little invariant commonalities between the subclasses.
  */
public:
  virtual WeightsCrafterPointer clone() const = 0;
  /* Defined in EACH subclass as:
     WeightsCrafterPointer clone() const override { return std::make_shared<std::decay_t<decltype(*this)>>(*this); }
  */

  /// The latest weights improved, re-alter accordingly.
  virtual void weightsImproved() = 0;
  /// The latest weights did not improve, re-alter accordingly.
  virtual void weightsDidNotImprove() = 0;
  /** Bring back the best weights, as last call to #weightsImproved and #weightsDidNotImprove
      may have deteriorated them.
  */
  virtual void bringBackBestWeights() = 0;

  /// Log useful informations about the current state.
  virtual void logCurrentState(Logger& logger) const = 0;
};

/*
****************
** BASE CLASS **
****************
*/

/** Abstract base class for ALL matrix digraph classes.
    The source nodes (leafs) are all of type Input and hold the input layer. The other nodes are of type Value
    and hold the hidden layers, except for the unique sink (root node) that holds the output value.
    Therefore the subclasses must handle the hidden layers and output value.
*/
class MatrixDigraph
{
  // DEFINITIONS //
public:
  using MatrixDigraphPointer = std::unique_ptr<MatrixDigraph>;
  using MatrixDigraphInstantiator =
    std::function<MatrixDigraphPointer(Index const rowsCount, Index const columnsCount)>;

  using Input = uint16_t;
  using Value = int64_t;

  // INSTANCE VARIABLES //
protected:
  Index myRequiredWeightsCount{ 0 };
  std::string myName;
  Index myColumnsCount;
  Index myInputsCount;
  std::vector<Input, NoConstructAllocator<Input>> myInputs;
  WeightsCrafter::ConstWeightsCrafterPointer myWeightsCrafterPointer;

  // DESTRUCTOR //
public:
  // Base class.
  virtual ~MatrixDigraph() = default;

  // CONSTRUCTORS //
  // Base class.
protected:
  /// Deleted.
  MatrixDigraph() = delete;

  // All implicitly created std::string names and rvalue std::string.
  explicit MatrixDigraph(Index const rowsCount, Index const columnsCount)
    : myColumnsCount{ columnsCount }
    , myInputsCount(rowsCount * columnsCount)
    , myInputs(myInputsCount)
  {
    if (rowsCount < 2)
      throw std::logic_error(String(+"rowsCount is ", rowsCount, +" in: ", +__PRETTY_FUNCTION__, '.'));

    if (columnsCount < 2)
      throw std::logic_error(String(+"columnsCount is ", columnsCount, +" in: ", +__PRETTY_FUNCTION__, '.'));
  }

  MatrixDigraph(MatrixDigraph const&) = default;

  /// Use #MatrixDigraphPointer and #clone() instead.
  MatrixDigraph(MatrixDigraph&&) = delete;

  // ASSIGNMENT OPERATORS //
protected:
  /// Use #MatrixDigraphPointer and #clone() instead.
  MatrixDigraph& operator=(MatrixDigraph&&) = delete;
  /// Use #MatrixDigraphPointer and #clone() instead.
  MatrixDigraph& operator=(MatrixDigraph const&) = delete;

  // PUBLIC INSTANCE METHODS //
public:
  decltype(auto) requiredWeightsCount() const noexcept { return myRequiredWeightsCount; }

  void setName(decltype(myName)&& name) noexcept(noexcept(myName = std::move(name))) { myName = std::move(name); }
  void setName(decltype(myName) const& name) noexcept(noexcept(myName = name)) { myName = name; }
  auto const& name() const noexcept { return myName; }

  /// @return True on success, else false, and log error.
  bool readInputsFromStream(Logger& logger, std::istream& inputsStream)
  {
    inputsStream.read(reinterpret_cast<std::decay_t<decltype(inputsStream)>::char_type*>(myInputs.data()),
                      static_cast<std::streamsize>(myInputsCount * sizeof(myInputs[0])));
    if (inputsStream.good())
      return true;
    else {
      logger.streamCondition(inputsStream) << "Reading the inputs stream into Matrix digraph '" << myName << "'.\n\n";
      return false;
    }
  }

  void useWeightsCrafter(decltype(myWeightsCrafterPointer) const& weightsCrafterPointer)
  {
    if (not weightsCrafterPointer)
      throw std::logic_error(String(+"Null weightsCrafterPointer in: ", +__PRETTY_FUNCTION__, '.'));
    if (myRequiredWeightsCount != weightsCrafterPointer->weightsCount())
      throw std::logic_error(String(+"Provided weightsCrafter's weightsCount (",
                                    weightsCrafterPointer->weightsCount(),
                                    +") is not equal to myRequiredWeightsCount (",
                                    myRequiredWeightsCount,
                                    +") in: ",
                                    +__PRETTY_FUNCTION__,
                                    '.'));

    myWeightsCrafterPointer = weightsCrafterPointer;
  }
  bool canApplyWeights() const noexcept { return static_cast<bool>(myWeightsCrafterPointer); }

  // ABSTRACT INTERFACE //
  /* Strategy pattern. The present base class is not implemented as a Template Method pattern (or NVI),
     as there shall be little invariant commonalities between the subclasses.
  */
public:
  virtual MatrixDigraphPointer clone() const = 0;
  /* Defined in EACH subclass as:
     MatrixDigraphPointer clone() const override { return std::make_unique<std::decay_t<decltype(*this)>>(*this); }
  */

  virtual void applyWeights() = 0;
  virtual Value uniqueSinkValue() const = 0;
};

/*
***********
** CLASS **
***********
*/

/// Creates and holds a collection MatrixDigraphs and subclasses.
class SupervisedNetworkEvent
{
  // DEFINITIONS //
private:
  using FileHeaderDatum = uint32_t;

  // INSTANCE VARIABLES //
private:
  std::string myName;
  // Vector of pointers used for subclassing MatrixDigraph.
  std::vector<MatrixDigraph::MatrixDigraphPointer> myMatrixDigraphPointers;
  Index myDesiredMatrixDigraphIndex;
  std::string myDesiredMatrixName;

  // DESTRUCTOR //
public:
  // Rule of five.
  ~SupervisedNetworkEvent() = default;

  // CONSTRUCTORS //
public:
  SupervisedNetworkEvent() { clearMatrixDigraphs(); }

  // Disabled for now as needs deep copying.
  SupervisedNetworkEvent(SupervisedNetworkEvent const&) = delete;

  SupervisedNetworkEvent(SupervisedNetworkEvent&&) = default;

  // ASSIGNMENT OPERATORS //
  // Disabled for now as needs deep copying.
  SupervisedNetworkEvent& operator=(SupervisedNetworkEvent const&) = delete;

  SupervisedNetworkEvent& operator=(SupervisedNetworkEvent&&) = default;

  // PUBLIC INSTANCE METHODS //
public:
  void clearMatrixDigraphs() noexcept(noexcept(myMatrixDigraphPointers.clear()))
  {
    myMatrixDigraphPointers.clear();
    myDesiredMatrixDigraphIndex = InvalidIndex;
  }

  void setName(decltype(myName)&& name) noexcept(noexcept(myName = std::move(name))) { myName = std::move(name); }
  void setName(decltype(myName) const& name) noexcept(noexcept(myName = name)) { myName = name; }
  auto const& name() const noexcept { return myName; }

  decltype(auto) matrixDigraphsCount() const noexcept(noexcept(myMatrixDigraphPointers.size()))
    { return myMatrixDigraphPointers.size(); }
  decltype(auto) empty() const noexcept(noexcept(myMatrixDigraphPointers.empty()))
    { return myMatrixDigraphPointers.empty(); }

  auto const& desiredMatrixName() const noexcept { return myDesiredMatrixName; }

  /// @return True on success, else false, and log error. Throw an exceptions on error.
  bool buildMatrixDigraphs(Logger& logger,
                           std::string const& desiredMatrixName,
                           decltype(OpenInputBinaryFileNamed(""))& eventFileStatus,
                           MatrixDigraph::MatrixDigraphInstantiator const& matrixDigraphInstantiator)
  {
    return buildMatrixDigraphs(logger, std::string(desiredMatrixName), eventFileStatus, matrixDigraphInstantiator);
  }
  /// @return True on success, else false, and log error. Throw an exceptions on error.
  bool buildMatrixDigraphs(Logger& logger,
                           std::string&& desiredMatrixName,
                           decltype(OpenInputBinaryFileNamed(""))& eventFileStatus,
                           MatrixDigraph::MatrixDigraphInstantiator const& matrixDigraphInstantiator)
  {
    // Check if matrixDigraphInstantiator is callable.
    if (not matrixDigraphInstantiator)
      throw std::logic_error(String(+"matrixDigraphInstantiator is not callable in: ", +__PRETTY_FUNCTION__, '.'));
    if (desiredMatrixName.empty())
      throw std::logic_error(String(+"Empty desiredMatrixName in: ", +__PRETTY_FUNCTION__, '.'));

    myDesiredMatrixName = std::move(desiredMatrixName);

    clearMatrixDigraphs();

    auto& [eventFile, errorMessage, eventFileSize]{ eventFileStatus };
    // Validate that the header can be read. An event file is made of a number of matrices.
    struct Header
    {
      FileHeaderDatum matricesCount;
      FileHeaderDatum matrixRowsCount;
      FileHeaderDatum matrixColumnsCount;
      FileHeaderDatum matrixNameSize;
    } eventFileHeader;
    if (eventFileSize < static_cast<decltype(eventFileSize)>(sizeof(eventFileHeader))) {
      logger.error() << "File is too small to extract the header.\n\n";
      return false;
    }

    // Read the header.
    eventFile.read(reinterpret_cast<decltype(eventFile)::char_type*>(&eventFileHeader), sizeof(eventFileHeader));
    if (not eventFile.good()) {
      logger.streamCondition(eventFile) << "Reading the header.\n\n";
      return false;
    }
    // Sanity test.
    if (eventFileHeader.matricesCount < 1) {
      logger.error() << "Matrices count is " << eventFileHeader.matricesCount << ".\n\n";
      return false;
    }
    if (eventFileHeader.matrixRowsCount < 2) {
      logger.error() << "Matrix rows count is " << eventFileHeader.matrixRowsCount << ".\n\n";
      return false;
    }
    if (eventFileHeader.matrixColumnsCount < 2) {
      logger.error() << "Matrix columns count is " << eventFileHeader.matrixColumnsCount << ".\n\n";
      return false;
    }
    if (eventFileHeader.matrixNameSize < 1) {
      logger.error() << "Matrix name size is " << eventFileHeader.matrixNameSize << ".\n\n";
      return false;
    }

    // Validate the event file size.
    auto const requiredEventFileSize{ (
      sizeof(eventFileHeader) +
      (eventFileHeader.matricesCount *
       (eventFileHeader.matrixNameSize +
        (eventFileHeader.matrixRowsCount * eventFileHeader.matrixColumnsCount * sizeof(MatrixDigraph::Input))))) };
    if (eventFileSize != static_cast<decltype(eventFileSize)>(requiredEventFileSize)) {
      logger.error() << "File is of size " << eventFileSize << " bytes but should be of size " << requiredEventFileSize
                     << " bytes according to its header stating that it contains " << eventFileHeader.matricesCount
                     << " matrices each made of: a name of size " << eventFileHeader.matrixNameSize << " bytes, "
                     << eventFileHeader.matrixRowsCount << " rows, " << eventFileHeader.matrixColumnsCount
                     << " columns, and a cell size of " << sizeof(MatrixDigraph::Input) << " bytes.\n\n";
      return false;
    }

    // Char vector to extract the matrix names.
    std::vector<char> matrixNameCString((eventFileHeader.matrixNameSize + 1), 0);

    // Build all the matrix digraphs.
    myMatrixDigraphPointers.resize(eventFileHeader.matricesCount);
    for (Index index{ 0 }; index != eventFileHeader.matricesCount; ++index) {
      // Extract the matrix name.
      eventFile.read(matrixNameCString.data(), eventFileHeader.matrixNameSize);
      if (not eventFile.good()) {
        logger.streamCondition(eventFile) << "Reading a matrix name.\n\n";
        return false;
      }

      std::decay_t<decltype(myMatrixDigraphPointers[index]->name())> matrixName(matrixNameCString.data());
      // Instantiate the next matrix digraph.
      if ((myMatrixDigraphPointers[index] =
             matrixDigraphInstantiator(eventFileHeader.matrixRowsCount, eventFileHeader.matrixColumnsCount))) {

        // Populate the matrix digraph just created.
        if (not myMatrixDigraphPointers[index]->readInputsFromStream(logger, eventFile))
          return false;
        myMatrixDigraphPointers[index]->setName(std::move(matrixName));

        // Try to locate the desired iputs matrix name.
        if (myDesiredMatrixName == myMatrixDigraphPointers[index]->name()) {
          if (myDesiredMatrixDigraphIndex == InvalidIndex)
            myDesiredMatrixDigraphIndex = index;
          else {
            logger.error() << "Desired matrix '" << myDesiredMatrixName << "' was encountered more than once.\n\n";
            return false;
          }
        }
      } else
        throw std::logic_error(String(+"matrixDigraphInstantiator failed to create matrix digraph '",
                                      matrixName,
                                      +"' in: ",
                                      +__PRETTY_FUNCTION__,
                                      '.'));
    }
    // Verify that a desired matrix was found.
    if (myDesiredMatrixDigraphIndex == InvalidIndex) {
      logger.error() << "Desired matrix '" << myDesiredMatrixName << "' was NOT encountered.\n\n";
      return false;
    }

    logger << "    ◦ Created " << eventFileHeader.matricesCount << " matrix digraphs of "
           << eventFileHeader.matrixRowsCount << " rows by " << eventFileHeader.matrixColumnsCount
           << " columns, and requiring " << requiredWeightsCount() << " weights.\n";

    return true;
  }

  /// @return 0 if there is no matrix digraph or they do not all agree.
  decltype(myMatrixDigraphPointers[0]->requiredWeightsCount()) requiredWeightsCount() const
  {
    if (empty())
      return 0;

    auto const weightsCount{ myMatrixDigraphPointers[0]->requiredWeightsCount() };
    for (auto const& matrixDigraphPointer : myMatrixDigraphPointers)
      if (weightsCount != matrixDigraphPointer->requiredWeightsCount())
        throw std::logic_error(String(+"requiredWeightsCount() not common in SupervisedNetworkEvent '", myName, +"'."));

    return weightsCount;
  }
  void useWeightsCrafter(WeightsCrafter::ConstWeightsCrafterPointer const& weightsCrafterPointer) const
  {
    for (auto&& matrixDigraphPointer : myMatrixDigraphPointers)
      matrixDigraphPointer->useWeightsCrafter(weightsCrafterPointer);
  }
  /** @pre #canApplyWeights MUST ABSOLUTELY return TRUE before #applyWeights is called.
      For performance, not doing so may result in undefined behaviour.
      @return True if #applyWeights may be called, else false.
  */
  bool canApplyWeights() const
    noexcept(noexcept(myMatrixDigraphPointers.empty()) and noexcept(myMatrixDigraphPointers[0]->canApplyWeights()))
  {
    if (myMatrixDigraphPointers.empty())
      return false;

    for (auto const& matrixDigraphPointer : myMatrixDigraphPointers)
      if (not matrixDigraphPointer->canApplyWeights())
        return false;

    return true;
  }
  /** @pre #canApplyWeights MUST ABSOLUTELY return TRUE before #applyWeights is called.
      For performance, not doing so may result in undefined behaviour.
  */
  void applyWeights() const
    noexcept(noexcept(myMatrixDigraphPointers[0]->applyWeights()))
  {
    for (auto&& matrixDigraphPointer : myMatrixDigraphPointers)
      matrixDigraphPointer->applyWeights();
  }

  /// @return 0 if there is no desired matrix digraph.
  decltype(auto) desiredMatrixDigraphRank() const
    noexcept(noexcept(myMatrixDigraphPointers[0]->uniqueSinkValue()))
  {
    Index rank{ 0 };

    if (myDesiredMatrixDigraphIndex == InvalidIndex)
      return rank;

    auto const desiredMatrixDigraphUniqueSinkValue{
      myMatrixDigraphPointers[myDesiredMatrixDigraphIndex]->uniqueSinkValue()
    };
    // Count how many matrix network's output values (including de desired one's) is >= than the
    // desired one's.
    for (auto const& matrixDigraphPointer : myMatrixDigraphPointers)
      if (matrixDigraphPointer->uniqueSinkValue() >= desiredMatrixDigraphUniqueSinkValue)
        ++rank;

    return rank;
  }
  // Reverse-sort the matrix digraphs by output value.
  void reverseSortMatrixDigraphsByUniqueSinkValue()
  {
    std::sort(myMatrixDigraphPointers.begin(),
              myMatrixDigraphPointers.end(),
              [](auto const& firstMatrixDigraphPointer, auto const& secondMatrixDigraphPointer) {
                return secondMatrixDigraphPointer->uniqueSinkValue() < firstMatrixDigraphPointer->uniqueSinkValue();
              });
  }
  void logUniqueSinkValues(Logger& logger) const
  {
    logger << "In '" << myName << "':";
    for (auto const& matrixDigraphPointer : myMatrixDigraphPointers)
      logger << ' ' << matrixDigraphPointer->name() << '(' << matrixDigraphPointer->uniqueSinkValue() << ')';
    logger << ".\n";
  }
};

/*
***********
** CLASS **
***********
*/

/// Creates and holds a collection of Events and one Weights object, and control the training.
class SupervisedNetworkTrainer
{
public:
  // DEFINITIONS //
  using MatrixDigraphsMap = std::map<std::string, MatrixDigraph::MatrixDigraphInstantiator>;
  using WeightsCraftersMap = std::map<std::string, WeightsCrafter::WeightsCrafterInstantiator>;
  constexpr static Index const SummarySecondsCount{ 60 };

  // INSTANCE VARIABLES //
private:
  std::vector<SupervisedNetworkEvent> mySupervisedNetworkEvents;
  WeightsCrafter::WeightsCrafterPointer myWeightsCrafterPointer;
  std::unique_ptr<GoferThreadsPool> myGoferThreadsPoolPointer;
  long int myMaximumTrainingCyclesCount;
  sig_atomic_t myAlive{ false };

  // PRIVATE INSTANCE METHODS //
private:
  void logRanks(Logger& logger) const
  {
    Index ranksTotal{ 0 };
    for (auto const& supervisedNetworkEvent : mySupervisedNetworkEvents)
      ranksTotal += supervisedNetworkEvent.desiredMatrixDigraphRank();

    logger << "  ∙ The " << mySupervisedNetworkEvents.size() << " ranks totalling " << ranksTotal << " are:\n";
    for (auto const& supervisedNetworkEvent : mySupervisedNetworkEvents) {
      logger << "    ◦ " << supervisedNetworkEvent.desiredMatrixDigraphRank() << " for '"
             << supervisedNetworkEvent.desiredMatrixName() << "' in '" << supervisedNetworkEvent.name() << "'.\n";
    }
  }

  void train(Logger& logger)
  {
    myAlive = true;

    logger << "\n● Will train for UP TO " << myMaximumTrainingCyclesCount << " cycles...\n";

    // Populate the errands vector.
    std::vector<GoferThreadsPool::ErrandProcedure> errands;
    if (myGoferThreadsPoolPointer) {
      errands.reserve(mySupervisedNetworkEvents.size());
      for (auto&& supervisedNetworkEvent : mySupervisedNetworkEvents)
        errands.emplace_back(
          [&supervisedNetworkEvent]() { supervisedNetworkEvent.applyWeights(); }
        );
    }

    Index const ranksCount{ static_cast<Index>(mySupervisedNetworkEvents.size()) };
    // Set the initial ranks to maximum default.
    Index ranksTotal{ 0 };
    for (auto&& supervisedNetworkEvent : mySupervisedNetworkEvents)
      ranksTotal += supervisedNetworkEvent.matrixDigraphsCount();

    long int cyclesCount, lastCyclesCount{ 0 }, summaryCyclesCount{ 100 };
    Timer timer;
    // Train up to maximum training cycles count or until the total ranks count reaches the event networks count.
    for (cyclesCount = 1, ++myMaximumTrainingCyclesCount;
      myAlive and (cyclesCount != myMaximumTrainingCyclesCount) and (ranksTotal > ranksCount);
      ++cyclesCount)
    {
      if (myGoferThreadsPoolPointer) {
        // Calculate all event networks via the gofer threads pool.
        myGoferThreadsPoolPointer->enQueueErrands(errands);
        myGoferThreadsPoolPointer->waitForAllErrandsToComplete();
      } else
        // Calculate all event networks on the main thread.
        for (auto&& supervisedNetworkEvent : mySupervisedNetworkEvents)
          supervisedNetworkEvent.applyWeights();

      Index newRanksTotal {0};
      for (auto const& supervisedNetworkEvent : mySupervisedNetworkEvents)
        newRanksTotal += supervisedNetworkEvent.desiredMatrixDigraphRank();

      bool const ranksDecreased{ newRanksTotal < ranksTotal };
      if (ranksDecreased) {
        ranksTotal = newRanksTotal;
        // Tell the weights that they improved.
        myWeightsCrafterPointer->weightsImproved();
      } else
        // Tell the weights that they did not improve.
        myWeightsCrafterPointer->weightsDidNotImprove();

      if (ranksDecreased or (cyclesCount == summaryCyclesCount)) {
        auto const elapsedTicks{ timer.elapsedTicks() };
        /* Since elapsedCycles is always used along with elapsedTicks, bake in ticksPerSecond
           so that elapsedTicks "becomes" elapsedSeconds.
        */
        auto const elapsedCycles_ticksPerSecond{ (cyclesCount - lastCyclesCount) * timer.TicksPerSecond };
        auto const secondsLeft{ ((myMaximumTrainingCyclesCount - cyclesCount) * elapsedTicks)
          / elapsedCycles_ticksPerSecond };
        auto const minutesLeft{ secondsLeft / 60 };

        logger << "  ∙ " << cyclesCount << " cycles spent ("
               << static_cast<double>((cyclesCount * 100)) / static_cast<double>(myMaximumTrainingCyclesCount) << "%), ";
        if (minutesLeft > 0)
          logger << (minutesLeft / 60) << " hr " << (minutesLeft % 60) << " min";
        else
          logger << secondsLeft << " seconds";
        logger << " left at " << (elapsedCycles_ticksPerSecond / elapsedTicks) << " cycles/sec.\n    ◦ ";
        myWeightsCrafterPointer->logCurrentState(logger);

        if (ranksDecreased) {
          logRanks(logger);
        }

        summaryCyclesCount = cyclesCount
          + ((elapsedCycles_ticksPerSecond * SummarySecondsCount) / elapsedTicks);
        lastCyclesCount = cyclesCount;

        // Restart the timer.
        timer.restart();
      }
    }
    --cyclesCount;
    --myMaximumTrainingCyclesCount;

    logger << "\n● Trained for " << cyclesCount << " cycles.\n";

    logger << "\n● Saving weights...\n  ∙ ";
    myWeightsCrafterPointer->bringBackBestWeights();
    myWeightsCrafterPointer->writeWeightsToFile(logger);

    myAlive = false;
  }

  // PUBLIC INSTANCE METHODS //
public:
  /** @param[in] logger Logger.
      @param[in] argumentsCount #main's argc.
      @param[in] arguments #main's argv.
      @param[in] matrixDigraphsMap MatrixDigraphsMap containing a map of
                 matrix digraph type name to the corresponding instantiator.
      @param[in] weightsCraftersMap WeightsCraftersMap containing a map of
                 weights crafter type name to the corresponding instantiator.
      @return True on success else false, and log errors.
  */
  bool populateFromArguments(Logger& logger,
                             int const argumentsCount,
                             char const* const* const arguments,
                             MatrixDigraphsMap const& matrixDigraphsMap,
                             WeightsCraftersMap const& weightsCraftersMap)
  {
    // When there is more than one matrix digraph type, we will make them selectable at run time on the command line.
    if (matrixDigraphsMap.size() != 1)
      throw std::logic_error(String(+"matrixDigraphsMap's size is not 1 in: ", +__PRETTY_FUNCTION__, '.'));
    auto const& matrixDigraphName{ matrixDigraphsMap.cbegin()->first };
    auto const& matrixDigraphInstantiator{ matrixDigraphsMap.cbegin()->second };

    // When there is more then one weights crafter type, we will make them selectable at run time on the command line.
    if (weightsCraftersMap.size() != 1)
      throw std::logic_error(String(+"weightsCraftersMap's size is not 1 in: ", +__PRETTY_FUNCTION__, '.'));
    auto const& weightsCrafterName{ weightsCraftersMap.cbegin()->first };
    auto const& weightsCrafterInstantiator{ weightsCraftersMap.cbegin()->second };

    // Check if matrixDigraphInstantiator is callable.
    if (not matrixDigraphInstantiator)
      throw std::logic_error(String(+"matrixDigraphInstantiator is not callable in: ", +__PRETTY_FUNCTION__, '.'));
    // Check if weightsCrafterInstantiator is callable.
    if (not weightsCrafterInstantiator)
      throw std::logic_error(String(+"weightsCrafterInstantiator is not callable in: ", +__PRETTY_FUNCTION__, '.'));

    auto const logUsage{ [&]() {
      logger << "Usage: " << arguments[0] << '\n'
             << "       <maximum number of training cycles>\n"
             << "       <number of training threads, 0 for hardware threads ÷ 2>\n"
             << "       [ <desired matrix name>  <event file name>  ]+\n"
             << "       [ <weights file name> ]\n";
    } };

    // Validate the number of parameters passed.
    if (argumentsCount < 5) {
      logUsage();
      return false;
    }
    // Necessarily positive.
    Index const eventFilesCount{ static_cast<Index>((argumentsCount - 3) / 2) };

    // Output back all arguments.
    for (auto index{ 0 }; index != argumentsCount; ++index)
      logger << '\'' << arguments[index] << "'  ";

    logger << "\n\n● Parsing the command line arguments...\n  ∙ Matrix digraph name is '" << matrixDigraphName
           << "'.\n  ∙ Weights crafter name is '" << weightsCrafterName << "'.\n";

    // Extract the maximum number of training cycles.
    try {
      if ((myMaximumTrainingCyclesCount = std::stol(arguments[1])) < 1)
        throw false;
    } catch (...) {
      logger.error() << "Maximum number of training cycles must be between 1 and "
                     << std::numeric_limits<decltype(myMaximumTrainingCyclesCount)>::max() << ", not '"
                     << arguments[1] << "'.\n\n";
      logUsage();

      return false;
    }
    logger << "  ∙ Maximum number of training cycles is " << myMaximumTrainingCyclesCount << ".\n";

    // Extract the number of training threads.
    decltype(std::stoi(arguments[2])) trainingThreadsCount;
    try {
      trainingThreadsCount = std::stoi(arguments[2]);
      if ((trainingThreadsCount) and
        ((trainingThreadsCount <
          static_cast<decltype(trainingThreadsCount)>(GoferThreadsPool::MinimumGoferThreadsCount))
        or (trainingThreadsCount >
          static_cast<decltype(trainingThreadsCount)>(GoferThreadsPool::MaximumGoferThreadsCount))))
        throw false;
    } catch (...) {
      logger.error() << "Number of training threads must be 0 or be between "
                     << GoferThreadsPool::MinimumGoferThreadsCount
                     << " and " << GoferThreadsPool::MaximumGoferThreadsCount << ", not '" << arguments[2] << "'.\n\n";
      logUsage();

      return false;
    }
    logger << "  ∙ Number of training threads is ";
    if (trainingThreadsCount)
      logger << trainingThreadsCount << ".\n";
    else
      logger << "hardware threads ÷ 2.\n";

    // Extract the number of pairs of event file name and desired matrix name.
    logger << "  ∙ The desired matrix name in each of the " << eventFilesCount << " event files are:\n";
    for (Index index{ 0 }; index != eventFilesCount; ++index)
      logger << "    ◦ '" << arguments[(index * 2) + 3] << "' in file '" << arguments[(index * 2) + 4] << "'.\n";

    auto const weightsFileName{ (argumentsCount % 2) ? nullptr : (arguments[argumentsCount - 1]) };
    if (weightsFileName)
      logger << "  ∙ Weights file name is '" << weightsFileName << "'.\n";
    else
      logger << "  ∙ NO weights file name was provided.\n";

    // Create the supervised network events.
    logger << "\n● Creating " << eventFilesCount << " supervised network events...\n";
    mySupervisedNetworkEvents.resize(eventFilesCount);
    for (Index index{ 0 }; index != eventFilesCount; ++index) {
      auto const desiredMatrixName{ arguments[(index * 2) + 3] };
      auto const eventFileName{ arguments[(index * 2) + 4] };
      logger << "  ∙ Parsing event file '" << eventFileName << "'...\n";

      // Open the event file in binary reading mode.
      auto eventFileStatus{ OpenInputBinaryFileNamed(eventFileName) };
      auto const& [eventFile, errorMessage, eventFileSize] = eventFileStatus;
      if (not eventFile.good()) {
        logger.streamCondition(eventFile) << errorMessage << "\n\n";
        return false;
      }

      // Build a new matrix digraph.
      if (not mySupervisedNetworkEvents[index].buildMatrixDigraphs(
            logger, desiredMatrixName, eventFileStatus, matrixDigraphInstantiator))
        return false;
      mySupervisedNetworkEvents[index].setName(eventFileName);
    }

    // Verify that all weights count are equal, and not 0.
    decltype(mySupervisedNetworkEvents[0].requiredWeightsCount()) commonRequiredWeightsCount{ 0 };
    for (Index index{ 0 }; index != eventFilesCount; ++index) {
      auto const currentRequiredWeightsCount{ mySupervisedNetworkEvents[index].requiredWeightsCount() };
      if (not currentRequiredWeightsCount)
        throw std::logic_error(String(+"requiredWeightsCount() is 0 for SupervisedNetworkEvent '",
                                      mySupervisedNetworkEvents[index].name(),
                                      +"' in: ",
                                      +__PRETTY_FUNCTION__,
                                      '.'));
      else if (not index)
        commonRequiredWeightsCount = currentRequiredWeightsCount;
      else if (currentRequiredWeightsCount != commonRequiredWeightsCount) {
        logger.error() << "Not all supervised network events require the same number of weights.\n\n";
        return false;
      }
    }
    logger << "  ∙ Common required weights count is " << commonRequiredWeightsCount << ".\n";

    // Create the weights crafter.
    if (weightsFileName)
      logger << "\n● Creating the weights crafter parsing file '" << weightsFileName << "'...\n";
    else
      logger << "\n● Creating the randomized weights crafter...\n";

    if (not (myWeightsCrafterPointer = weightsCrafterInstantiator(commonRequiredWeightsCount)))
      throw std::logic_error(String(+"weightsCrafterInstantiator failed to create a weights crafter in: ",
                                    +__PRETTY_FUNCTION__, '.'));

    // Populate the weights crafter if a file name was provided.
    if (weightsFileName) {
      // Open the weights file in binary reading mode.
      auto weightsFileStatus{ OpenInputBinaryFileNamed(weightsFileName) };
      auto const& [weightsFile, errorMessage, weightsFileSize] = weightsFileStatus;
      if (not weightsFile.good()) {
        logger.streamCondition(weightsFile) << errorMessage << "\n\n";
        return false;
      }

      std::cout << "  ∙ ";
      if (not myWeightsCrafterPointer->readWeightsFromFile(logger, weightsFileStatus))
        return false;
    }

    // Assign the newly created weights crafter to all supervised network events.
    logger << "  ∙ Assigning the weights crafter to the supervised network events...\n";
    for (auto&& supervisedNetworkEvent : mySupervisedNetworkEvents)
      supervisedNetworkEvent.useWeightsCrafter(myWeightsCrafterPointer);

    // Create, or not, the gofer threads.
    if (myMaximumTrainingCyclesCount > 1)
    {
      if (trainingThreadsCount == 1)
        logger << "\n● The training will be done on the main thread.\n";
      else {
        logger << "\n● Spawning the training threads...\n";
        myGoferThreadsPoolPointer = std::make_unique<GoferThreadsPool>(trainingThreadsCount);
        logger << "  ∙ " << myGoferThreadsPoolPointer->goferThreadsCount() << " training threads were spawned.\n";
      }
    }

    logger << '\n';
    return true;
  }

  /// To be called asynchronously to stop training.
  void stop() noexcept { myAlive = false; }

  void run(Logger& logger)
  {
    if (myMaximumTrainingCyclesCount > 1)
      train(logger);

    //  Apply the (best) weights to all the non-input values, either one last time or once.
    for (auto&& supervisedNetworkEvent : mySupervisedNetworkEvents)
      supervisedNetworkEvent.applyWeights();
    logger << "\n● The final ranks are:\n";
    logRanks(logger);

    logger << "\n● The final ordered names are:\n";
    for (auto&& supervisedNetworkEvent : mySupervisedNetworkEvents) {
      supervisedNetworkEvent.reverseSortMatrixDigraphsByUniqueSinkValue();
      logger << "  ∙ ";
      supervisedNetworkEvent.logUniqueSinkValues(logger);
    }

    logger << '\n';
  }
};
