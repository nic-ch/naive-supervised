// Utilities.hpp

#pragma once

/** @file
    Utility Classes

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

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <ctime>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <new>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

/*
*****************
** DEFINITIONS **
*****************
*/

#ifdef __cpp_lib_hardware_interference_size
#define CACHE_LINE_BYTE_SIZE (std::hardware_destructive_interference_size)
#endif
// Set to 0 if not provided.
#ifndef CACHE_LINE_BYTE_SIZE
#define CACHE_LINE_BYTE_SIZE 0
#endif
// Set to 0 in case provided CACHE_LINE_BYTE_SIZE is empty or not set.
constexpr static unsigned int const ProvidedCacheLineByteSize{ CACHE_LINE_BYTE_SIZE };
// ProvidedCacheLineByteSize is suspicious if 0 or < 8. Should be 64 bytes on x86-64.
constexpr static unsigned int const CacheLineByteSize{ (ProvidedCacheLineByteSize < 8) ? 64
                                                                                       : ProvidedCacheLineByteSize };
#define ALIGN_CACHE_FRIENDLY alignas(CacheLineByteSize)

#if (2 << 1) == 1
#define SHIFT_DECREASE <<
#define SHIFT_INCREASE >>
#else
#define SHIFT_INCREASE <<
#define SHIFT_DECREASE >>
#endif

/*
****************
** PROCEDURES **
****************
*/

/** Try to open file provided in fileName in binary read mode.
    Returns std::tuple {std::ifstream file, std::string errorMessage, std::streamsize fileSize},
    where errorMessage is provided on failure an fileSize (in char_types) is valid on success.
*/
static class
{
  // DEFINITIONS //
public:
  using Status = std::tuple<std::ifstream, std::string, std::streamsize>;

  // PRIVATE INSTANCE METHODS //
private:
  // De-templatize as much as possible (although the compiler may inline it back).
  void getJustOpenedBinaryFileStatus(Status& status)
  {
    auto& [file, errorMessage, fileSize] = status;

    fileSize = 0;
    if (file.good()) {
      // Determine end of file position.
      file.seekg(0, std::ios_base::end);
      if (file.good()) {
        const auto endPosition{ file.tellg() };
        if (file.good()) {
          // Determine beginning of file position.
          file.seekg(0, std::ios_base::beg);
          if (file.good()) {
            const auto beginningPosition{ file.tellg() };
            if (file.good())
              fileSize = (endPosition - beginningPosition);
            else
              errorMessage = "Can not read file's beginning position.";
          } else
            errorMessage = "Can not seek to file's beginning.";
        } else
          errorMessage = "Can not read file's end position.";
      } else
        errorMessage = "Can not seek to file's end.";
    } else
      errorMessage = "Can not open file for reading.";
  }

  // To not have to copy-and-paste the code above in the three following functions.
  template<typename FileName>
  decltype(auto) openBinaryFileNamed(FileName const& fileName)
  {
    Status status;
    auto& [file, errorMessage, fileSize] = status;

    // Open file named fileName for reading.
    file.open(fileName, std::ios::binary);
    getJustOpenedBinaryFileStatus(status);

    return status;
  }

  // PUBLIC INSTANCE METHODS //
public:
  // To avoid template instantiations proliferating for char[] C-strings of all sizes, define untemplated char*.

  decltype(auto) operator()(char const* const fileName)
  {
    return openBinaryFileNamed(static_cast<char const*>(fileName));
  }

  decltype(auto) operator()(char* const fileName) { return openBinaryFileNamed(static_cast<char const*>(fileName)); }

  template<typename FileName>
  decltype(auto) operator()(FileName const& fileName)
  {
    return openBinaryFileNamed(fileName);
  }
} OpenInputBinaryFileNamed;

/** e.g. String(+"Hello ", 123.45, '.') returns std::string("Hello 123.45.").
    Prefix C-strings literals with a + sign, e.g. +"Hello World!", to decay them all to char const *
    so to avoid template instantiations proliferating due to different char array sizes.
*/
template<typename... Values>
static decltype(auto)
// Not 'Values const &' as Timer's << is not const.
String(Values&&... values)
{
  if constexpr (sizeof...(values)) {
    std::ostringstream stringStream;
    // Binary-left-fold.
    (stringStream << ... << values);
    return stringStream.str();
  } else
    return std::string();
}

static decltype(auto)
TypeNameOfTypeID(std::type_info const& typeID)
{
  constexpr static auto const Numerals{ "0123456789" };

  std::string prettierTypeName;

  std::string typeName{ typeID.name() };
  std::decay_t<decltype(decltype(prettierTypeName)::npos)> index, length;
  bool firstTime{ true };
  while (not typeName.empty()) {
    // Find next number's beginning.
    if ((index = typeName.find_first_of(Numerals)) == std::string::npos)
      break;
    // Erase everything before it.
    if (index > 0)
      typeName.erase(0, index);

    // Find next number's end.
    if ((index = typeName.find_first_not_of(Numerals)) == std::string::npos)
      break;
    // If the length expressed is larger than the rest of typeName then break.
    if (((length = std::stoul(typeName.substr(0, index))) + index) > typeName.length())
      break;

    // Insert ':' between types in prettierTypeName.
    if (firstTime)
      firstTime = false;
    else
      prettierTypeName += ':';
    // Insert the next type in prettierTypeName.
    prettierTypeName += typeName.substr(index, length);

    // Erase all what was just used from typeName.
    typeName.erase(0, length + index);
  }
  if (prettierTypeName.empty())
    prettierTypeName = typeID.name();

  return prettierTypeName;
}
template<typename Object>
static decltype(auto)
TypeNameOf(Object const& object)
{
  return TypeNameOfTypeID(typeid(object));
}

/*
***********
** MIXIN **
***********
*/

/// Mixin for #Array classes to de-templateize generic code.
class ArrayMixin
{
  // DESTRUCTOR //
protected:
  ~ArrayMixin() noexcept = default;

  // CONSTRUCTORS //
protected:
  ArrayMixin() noexcept = default;
  ArrayMixin(ArrayMixin const&) noexcept = default;
  ArrayMixin(ArrayMixin&&) noexcept = default;

  // ASSIGNMENT OPERATORS //
protected:
  ArrayMixin& operator=(ArrayMixin const&) noexcept = default;
  ArrayMixin& operator=(ArrayMixin&&) noexcept = default;

  // PROTECTED METHODS //
protected:
  template<typename Size>
  void throwException_DifferentSizes(Size const receiverSize, Size const argumentSize, char const* methodName) const
  {
    throw std::logic_error(String(
      +"Receiver is of size ", receiverSize, +" and argument is of size ", argumentSize, +" in: ", methodName, '.'));
  }

  template<typename Size>
  void throwException_CanNotResize(Size const receiverSize, Size const newSize, char const* methodName) const
  {
    throw std::logic_error(
      String(+"Receiver of size ", receiverSize, +" can not be resized (to ", newSize, +") in: ", methodName, '.'));
  }

  template<typename Size>
  void throwException_CouldNotBeSized(Size const receiverSize, Size const newSize, char const* methodName) const
  {
    throw std::runtime_error(String(
      +"Receiver's vector (of size ", receiverSize, +") could not be resized to ", newSize, +" in: ", methodName, '.'));
  }
};

/*
***********
** CLASS **
***********
*/

/// Will NOT change its size once set, based on "cache friendly" std::vector.
template<typename Element, typename ElementAllocator = std::allocator<Element>>
class Array : protected virtual ArrayMixin
{
  // INSTANCE VARIABLES //
private:
  // Composition is used instead of inheriting from std::vector.
  std::vector<Element, ElementAllocator> myVector;

  // TYPES //
public:
  using value_type = typename decltype(myVector)::value_type;
  using allocator_type = typename decltype(myVector)::allocator_type;
  using size_type = typename decltype(myVector)::size_type;
  using difference_type = typename decltype(myVector)::difference_type;
  using reference = typename decltype(myVector)::reference;
  using const_reference = typename decltype(myVector)::const_reference;
  using pointer = typename decltype(myVector)::pointer;
  using const_pointer = typename decltype(myVector)::const_pointer;
  using iterator = typename decltype(myVector)::iterator;
  using const_iterator = typename decltype(myVector)::const_iterator;
  using reverse_iterator = typename decltype(myVector)::reverse_iterator;
  using const_reverse_iterator = typename decltype(myVector)::const_reverse_iterator;

  // DESTRUCTOR //
public:
  // Rule of Five.
  ~Array() = default;

  // CONSTRUCTORS //
public:
  Array() noexcept(noexcept(decltype(myVector)())) = default;
  explicit Array(size_t const size)
    : myVector(size)
  {
  }

  // Rule of Five.
  Array(Array const& otherArray) noexcept(noexcept(myVector = otherArray.myVector)) = default;
  Array(Array&& otherArray) noexcept(noexcept(myVector = std::move(otherArray.myVector))) = default;

  // ASSIGNMENT OPERATORS //
public:
  /** @pre THROWS AN EXCEPTION if the receiver is not empty or not of same size as otherArray.
      Exceptions are thrown on pre-condition violations in case the caller wants to recover,
      save some states, or cleanup the stack before propagating the (by definition exceptional) failure.
  */
  Array& operator=(Array const& otherArray) noexcept(false)
  {
    if (this != std::addressof(otherArray)) {
      if (empty() or (size() == otherArray.size()))
        myVector = otherArray.myVector;
      else
        throwException_DifferentSizes(size(), otherArray.size(), __PRETTY_FUNCTION__);
    }

    return *this;
  }

  /** @pre THROWS AN EXCEPTION if the receiver is not empty or not of same size as otherArray.
      Exceptions are thrown on pre-condition violations in case the caller wants to recover,
      save some states, or cleanup the stack before propagating the (by definition exceptional) failure.
  */
  Array& operator=(Array&& otherArray) noexcept(false)
  {
    if (this != std::addressof(otherArray)) {
      if (empty() or (size() == otherArray.size()))
        myVector = std::move(otherArray.myVector);
      else
        throwException_DifferentSizes(size(), otherArray.size(), __PRETTY_FUNCTION__);
    }

    return *this;
  }

  // PUBLIC INSTANCE METHODS //
public:
  // Element access.
  decltype(auto) at(size_t const index) noexcept(noexcept(myVector.at(index))) { return myVector.at(index); }
  decltype(auto) at(size_t const index) const noexcept(noexcept(myVector.at(index))) { return myVector.at(index); }
  decltype(auto) operator[](size_t const index) noexcept(noexcept(myVector[index])) { return myVector[index]; }
  decltype(auto) operator[](size_t const index) const noexcept(noexcept(myVector[index])) { return myVector[index]; }
  decltype(auto) front() noexcept(noexcept(myVector.front())) { return myVector.front(); }
  decltype(auto) front() const noexcept(noexcept(myVector.front())) { return myVector.front(); }
  decltype(auto) back() noexcept(noexcept(myVector.back())) { return myVector.back(); }
  decltype(auto) back() const noexcept(noexcept(myVector.back())) { return myVector.back(); }
  decltype(auto) data() noexcept(noexcept(myVector.data())) { return myVector.data(); }
  decltype(auto) data() const noexcept(noexcept(myVector.data())) { return myVector.data(); }

  // Iterators.
  decltype(auto) begin() noexcept(noexcept(myVector.begin())) { return myVector.begin(); }
  decltype(auto) begin() const noexcept(noexcept(myVector.begin())) { return myVector.begin(); }
  decltype(auto) cbegin() const noexcept(noexcept(myVector.cbegin())) { return myVector.cbegin(); }
  decltype(auto) end() noexcept(noexcept(myVector.end())) { return myVector.end(); }
  decltype(auto) end() const noexcept(noexcept(myVector.end())) { return myVector.end(); }
  decltype(auto) cend() const noexcept(noexcept(myVector.cend())) { return myVector.cend(); }
  decltype(auto) rbegin() noexcept(noexcept(myVector.rbegin())) { return myVector.rbegin(); }
  decltype(auto) rbegin() const noexcept(noexcept(myVector.rbegin())) { return myVector.rbegin(); }
  decltype(auto) crbegin() const noexcept(noexcept(myVector.crbegin())) { return myVector.crbegin(); }
  decltype(auto) rend() noexcept(noexcept(myVector.rend())) { return myVector.rend(); }
  decltype(auto) rend() const noexcept(noexcept(myVector.rend())) { return myVector.rend(); }
  decltype(auto) crend() const noexcept(noexcept(myVector.crend())) { return myVector.crend(); }

  // Capacity.
  decltype(auto) empty() const noexcept(noexcept(myVector.empty())) { return myVector.empty(); }
  decltype(auto) size() const noexcept(noexcept(myVector.size())) { return myVector.size(); }
  decltype(auto) max_size() const noexcept(noexcept(size())) { return size(); }

  // Comparators.
  bool operator==(Array const& otherArray) const noexcept(noexcept(myVector == otherArray.myVector))
  {
    if (this == std::addressof(otherArray))
      return true;
    return myVector == otherArray.myVector;
  }
  bool operator!=(Array const& otherArray) const noexcept(noexcept(*this == otherArray))
  {
    return not(*this == otherArray);
  }

  bool operator<(Array const& otherArray) const noexcept(noexcept(myVector < otherArray.myVector))
  {
    if (this == std::addressof(otherArray))
      return false;
    return myVector < otherArray.myVector;
  }
  bool operator<=(Array const& otherArray) const noexcept(noexcept(myVector <= otherArray.myVector))
  {
    return not(otherArray < *this);
  }
  bool operator>(Array const& otherArray) const noexcept(noexcept(myVector > otherArray.myVector))
  {
    return otherArray < *this;
  }
  bool operator>=(Array const& otherArray) const noexcept(noexcept(myVector >= otherArray.myVector))
  {
    return not(*this < otherArray);
  }

  // Operations.

  /// @pre Throws an exception if can not be sized or resized.
  void setSize(size_t const newSize)
  {
    if (myVector.empty()) {
      myVector.resize(newSize);
      if (newSize != myVector.size())
        throwException_CouldNotBeSized(myVector.size(), newSize, __PRETTY_FUNCTION__);
    } else if (newSize != myVector.size())
      throwException_CanNotResize(size(), newSize, __PRETTY_FUNCTION__);
  }

  void fill(Element const& value) noexcept(noexcept(myVector.assign(size(), value))) { myVector.assign(size(), value); }

  /// @pre Throws an exception if the receiver is not of same size as otherArray.
  void swap(Array& otherArray) noexcept(false)
  {
    if (this != std::addressof(otherArray)) {
      if (size() == otherArray.size())
        myVector.swap(otherArray.myVector);
      else
        throwException_DifferentSizes(size(), otherArray.size(), __PRETTY_FUNCTION__);
    }
  }
  /// @pre Throws an exception if both arguments are not of same size.
  friend void swap(Array& firstArray, Array& secondArray) noexcept(false) { firstArray.swap(secondArray); }
};

/*
***********
** CLASS **
***********
*/

/** Pool of gofer threads that will eventually run all errands euqueued.
    Errands MUST be thread-safe OR share NO data.
    Each errand must capture by reference ONLY values that are guaranteed to outlive it.
    ABSOLUTELY NO PROTECTION is built-in against errands that will deadlock or not end.
    GoferThreadsPool itself is thread-safe if shared, and ONLY IF not destroyed by a sharer
    while other sharers are still using it.
*/
class GoferThreadsPool
{
  // DEFINITIONS //
public:
  using ErrandProcedure = std::function<void()>;

  constexpr static decltype(std::thread::hardware_concurrency()) const MinimumGoferThreadsCount{ 1 };
  constexpr static decltype(std::thread::hardware_concurrency()) const MaximumGoferThreadsCount{ 1024 };

  // INSTANCE VARIABLES //
private:
  mutable std::mutex myMutex;           // 40 bytes. Protects the variables below.
  unsigned int myErrandsLeftCount{ 0 }; // + 4 = 44 bytes. Being run AND still waiting in myErrandsQueue.
  /* Only used by the destructor to signal the gofer threads to die, as it is assumed that if GoferThreadsPool
     itself is shared, then it will NOT be destroyed by a sharer while other sharers are still using it.
  */
  bool myMustDie{ false };                                              // + 4(1) = 48 byte.
  std::queue<ErrandProcedure> myErrandsQueue;                           // + 80 = 128 = 2×64 bytes.
  mutable std::condition_variable_any myGoferThreadsConditionVariable;  // + 64 = 192 = 3×64 bytes.
  mutable std::condition_variable_any myClientThreadsConditionVariable; // + 64 = 256 = 4×64 bytes.
  std::vector<std::thread> myGoferThreadsVector;                        // + 24 = 280 = 4.375×64 bytes.

  // DESTRUCTOR //
public:
  ~GoferThreadsPool() noexcept
  {
    // Set myMustDie to true and notify all gofer threads.
    try {
      // Lock guard context.
      {
        std::lock_guard const lockGuard(myMutex);
        myMustDie = true;
      }
      /* Empty the gofer threads condition variable.
         ALL myGoferThreadsConditionVariable.wait() calls are done via the same mutex.
      */
      myGoferThreadsConditionVariable.notify_all();

      // Wait for all the joinable threads to end by joining them.
      for (auto&& goferThread : myGoferThreadsVector)
        if (goferThread.joinable())
          try {
            goferThread.join();
          } catch (...) {
            try {
              goferThread.detach();
            } catch (...) {
            }
          }
    } catch (...) {
      // Thrown locking the mutex. THIS SHOULD NEVER HAPPEN.

      // Try again to set myMustDie to true without the mutex.
      myMustDie = true;
      // Wait 100 milliseconds in hope caches have time to invalidate myMustDie.
      try {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
      } catch (...) {
      }
      // And empty the gofer threads condition variables.
      myGoferThreadsConditionVariable.notify_all();

      // Abandon all the joinable threads.
      for (auto&& goferThread : myGoferThreadsVector)
        if (goferThread.joinable())
          try {
            goferThread.detach();
          } catch (...) {
          }
    }
  }

  // CONSTRUCTORS //
public:
  /** @param[in] goferThreadsCount 0 for the number of cores ÷ 2.
      @post Throws an exception if a newly created thread is not joinable.
  */
  explicit GoferThreadsPool(decltype(std::thread::hardware_concurrency()) goferThreadsCount = 0)
  {
    if (goferThreadsCount < 1)
      // Real CPU core count is not guaranteed.
      goferThreadsCount = std::thread::hardware_concurrency() / 2;

    if (goferThreadsCount < MinimumGoferThreadsCount)
      goferThreadsCount = MinimumGoferThreadsCount;
    else if (goferThreadsCount > MaximumGoferThreadsCount)
      goferThreadsCount = MaximumGoferThreadsCount;

    myGoferThreadsVector.reserve(goferThreadsCount);
    while (goferThreadsCount--) {
      if (not(myGoferThreadsVector.emplace_back([this]() { this->goferThreadMethod(); })).joinable())
        throw std::runtime_error(String(+"Newly created thread is not joinable in: ", +__PRETTY_FUNCTION__, '.'));
    }
  }

  /// Deleted as gofer threads point to the instance variables.
  GoferThreadsPool(GoferThreadsPool const&) = delete;
  /// Deleted as gofer threads point to the instance variables.
  GoferThreadsPool(GoferThreadsPool&&) = delete;

  // ASSIGNMENT OPERATORS //
public:
  /// Deleted as gofer threads point to the instance variables.
  GoferThreadsPool& operator=(GoferThreadsPool const&) = delete;
  /// Deleted as gofer threads point to the instance variables.
  GoferThreadsPool& operator=(GoferThreadsPool&&) = delete;

  // PRIVATE INSTANCE METHODS //
private:
  /* When a gofer thread is wakened up from myGoferThreadsConditionVariable, thus inside the lock guard context,
     it finds itself in one of four conditions:
     1. myMustDie is true, thus return right away;
     2. myErrandsLeftCount is 0 so just notify all in myClientThreadsConditionVariable
        and then go wait back in myGoferThreadsConditionVariable;
     3. myErrandsLeftCount is not 0 but myErrandsQueue is empty,
        just go back in myGoferThreadsConditionVariable;
     4. myErrandsLeftCount is not 0 and therefore myErrandsQueue is not empty,
        pop an errand and run it outside the lock guard context,
        and then get back in the lock guard context and repeat as if just out of myGoferThreadsConditionVariable.
  */
  void goferThreadMethod()
  {
    // Lock guard context.
    {
      std::lock_guard const lockGuard(myMutex);

      // Simulate that an errand was just run.
      ++myErrandsLeftCount;
    }

    // Loop forever, or return if myMustDie.
    // ## Run-errands loop ##
    for (std::decay_t<decltype(myErrandsQueue.front())> errand; ; ) {
      // ## Lock guard context ##
      {
        std::lock_guard const lockGuard(myMutex);

        // One errand was just ran by me below.
        --myErrandsLeftCount;

        // Loop forever, or return if myMustDie, run an errand.
        // ## Look-for-an-errand-to-run loop ##
        for (;;) {
          if (myMustDie)
            return;

          // If there are errands left.
          if (myErrandsLeftCount) {
            // Some may still be in the queue, not being run.
            if (not myErrandsQueue.empty()) {
              // Get one errand (errands queue's front moved from as destroyed right after in pop()).
              errand = std::move(myErrandsQueue.front());
              myErrandsQueue.pop();
              // AND go run it OUT of the lock guard context.
              break;
            }
            // There is no errand left in the queue, so go wait in the gofer threads condition variable below.
          } else {
            // There is no errand left so signal and then go wait in the gofer threads condition variable below.
            myClientThreadsConditionVariable.notify_all();
          }

          myGoferThreadsConditionVariable.wait(myMutex);
        } // ## End of look-for-an-errand-to-run loop ##

        // 'break;' above breaks here.

      } // ## End of lock guard context ##

      // Run the errand OUT of the lock guard context.
      errand();
    } // ## End of run-errands loop ##
  }

  // Only get errands of type ErrandProcedure.
  template<typename Errand>
  bool privateEnQueueErrand(Errand&& errand)
  {
    if (errand) {
      // Lock guard context.
      {
        std::lock_guard const lockGuard(myMutex);
        // errand is queued-in.
        myErrandsQueue.push(std::forward<Errand>(errand));
        ++myErrandsLeftCount;
      }

      /* Notify the gofer threads condition variable. (In C++17 Standard N4660, ¶33.5-2: "Condition variables
         permit concurrent invocation of the [...] notify_one and notify_all member functions.")
         ALL myGoferThreadsConditionVariable.wait() calls are done via the same mutex.
      */
      myGoferThreadsConditionVariable.notify_one();

      return true;
    }

    return false;
  }

  // PUBLIC INSTANCE METHODS //
public:
  decltype(auto) goferThreadsCount() const noexcept(noexcept(myGoferThreadsVector[0].joinable()))
  {
    unsigned int count{ 0 };
    // Assume std::thread::joinable() is itself thread-safe, as it is const.
    for (auto const& goferThread : myGoferThreadsVector)
      if (goferThread.joinable())
        ++count;

    return count;
  }

  /** @param[in] errand to be eventually run by the gofer threads,
      of type void() e.g. +[] { ... } or [=, &a]() { a += b; }.
      @pre Errands MUST be thread-safe OR share NO data.
      Each errand must capture by reference ONLY values that are guaranteed to outlive it.
  */
  bool enQueueErrand(ErrandProcedure&& errand) { return privateEnQueueErrand(std::move(errand)); }
  /** @param[in] errand to be eventually run by the gofer threads,
      of type void() e.g. [=, &a]() { a += b; }.
      @pre Errands MUST be thread-safe OR share NO data.
      Each errand must capture by reference ONLY values that are guaranteed to outlive it.
  */
  bool enQueueErrand(ErrandProcedure const& errand) { return privateEnQueueErrand(errand); }

  /** @param[in] errandsContainer Container of thread-safe errands to be eventually run by the gofer threads.
      It must support range-based for loops. Each errand must be of type void() e.g. [=, &a]() { a += b; }.
      Marked 'const' although its elements may be moved from according to next argument.
      @param[in] preserveErrands FALSE if ALL the errands from the container can be moved from.
      @pre Errands MUST be thread-safe OR share NO data.
      Each errand must capture by reference ONLY values that are guaranteed to outlive it.
  */
  template<typename Container>
  decltype(auto) enQueueErrands(Container const& errandsContainer, bool const preserveErrands = true)
  {
    // All the errands that must be of type std::function<void()>
    static_assert(
      std::is_same_v<std::decay_t<decltype(myErrandsQueue.front())>, std::decay_t<decltype(errandsContainer[0])>>,
      "errandsContainer must contain errands of type std::function<void()>.");

    decltype(myErrandsLeftCount) errandsEnqueuedCount{ 0 };
    if (not errandsContainer.empty()) {
      // Lock guard context.
      {
        std::lock_guard const lockGuard(myMutex);

        // Copy-queue-in all the errands.
        if (preserveErrands) {
          // Check if errand is callable.
          for (auto const& errand : errandsContainer)
            if (errand) {
              // errand is copied-queued-in.
              myErrandsQueue.push(errand);
              ++errandsEnqueuedCount;
            }
        } else {
          // Check if errand is callable.
          for (auto&& errand : errandsContainer)
            if (errand) {
              // errand is moved-from-queued-in.
              myErrandsQueue.push(std::move(errand));
              ++errandsEnqueuedCount;
            }
        }

        myErrandsLeftCount += errandsEnqueuedCount;
      }
      // End of lock guard context

      /* Notify the gofer threads condition variable. (In C++17 Standard N4660, ¶33.5-2: "Condition variables
         permit concurrent invocation of the [...] notify_one and notify_all member functions.")
         ALL condition variables wait() calls are done via the same myMutex.
      */
      if (errandsEnqueuedCount > 1)
        myGoferThreadsConditionVariable.notify_all();
      else if (errandsEnqueuedCount)
        myGoferThreadsConditionVariable.notify_one();
    }

    return errandsEnqueuedCount;
  }

  decltype(auto) errandsLeftCount() const
  // Lock guard context.
  {
    std::lock_guard const lockGuard(myMutex);
    return myErrandsLeftCount;
  }

  /// @post This WILL deadlock if an errand deadlocks or does not end.
  void waitForAllErrandsToComplete() const
  // Lock guard context.
  {
    std::lock_guard const lockGuard(myMutex);
    myClientThreadsConditionVariable.wait(myMutex, [this]() { return not myErrandsLeftCount; });
  }
  /** @param[in] timePeriod A time period of type std::chrono::time_point<Clock, Duration>.
      @return True if all errands completed within timePeriod, else false.
  */
  template<typename Time>
  bool waitForAllErrandsToCompleteFor(Time const& timePeriod) const
  // Lock guard context.
  {
    std::lock_guard const lockGuard(myMutex);
    return myClientThreadsConditionVariable.wait_for(myMutex, timePeriod, [this]() { return not myErrandsLeftCount; });
  }
  /** @param[in] time Absolute time of type std::chrono::time_point<Clock, Duration>.
      @return True if all errands completed within time, else false.
  */
  template<typename Time>
  bool waitForAllErrandsToCompleteUntil(Time const& time) const
  // Lock guard context.
  {
    std::lock_guard const lockGuard(myMutex);
    return myClientThreadsConditionVariable.wait_until(myMutex, time, [this]() { return not myErrandsLeftCount; });
  }
};

/*
***********
** CLASS **
***********
*/

/// Output on cout and in a file if a log file name prefix is provided. NOT thread safe.
class Logger
{
  // DEFINITIONS //
public:
  constexpr static auto const DateTimeFormat{ "%Y-%m-%d %H:%M:%S" };
  constexpr static auto const DateTimeFormatForFileName{ "%Y-%m-%d_%H-%M-%S" };

  // INSTANCE VARIABLES //
private:
  std::ofstream myLogFile;
  bool myLogFileIsOpen{ false };

  // DESTRUCTOR //
public:
  ~Logger() noexcept
  {
    try {
      std::cout.flush();
    } catch (...) {
    }
  }

  // CONSTRUCTORS //
public:
  /// Complain (log) if can not open file for writing.
  explicit Logger(std::string const& logFileNamePrefix = "")
  {
    if (not logFileNamePrefix.empty()) {
      // Manufacture the log file name.
      std::ostringstream logFileNameStream;
      logFileNameStream << logFileNamePrefix;

      // And insert current date-and-time.
      struct tm dateAndTime;
      auto const timeValue{ std::time(nullptr) };
      if (auto const time{ localtime_r(std::addressof(timeValue), std::addressof(dateAndTime)) })
        logFileNameStream << '_' << std::put_time(time, DateTimeFormatForFileName);
      logFileNameStream << ".log";

      // Open the log file for writing.
      auto const logFileName{ logFileNameStream.str() };
      myLogFile.open(logFileName, std::ios_base::ate);
      if (not(myLogFileIsOpen = myLogFile.good()))
        streamCondition(myLogFile) << "Opening log file '" << logFileName << "' for writing.\n";
    }

    // Set the number of outputted decimals in numbers.
    *this << std::boolalpha << std::fixed << std::setprecision(2);
  }

  /// Deleted as logging to a file.
  Logger(Logger const&) = delete;
  Logger(Logger&&) = default;

  // ASSIGNMENT OPERATORS //
public:
  /// Deleted as logging to a file.
  Logger& operator=(Logger const&) = delete;
  Logger& operator=(Logger&&) = default;

  // PRIVATE INSTANCE METHODS //
private:
  // To avoid template instantiations proliferating.
  template<typename Value>
  // Not 'Values const &' as Timer's << is not const.
  Logger& privateInsert(Value&& value)
  {
    std::cout << value;
    if (myLogFileIsOpen)
      myLogFile << value;

    return *this;
  }

  // PUBLIC INSTANCE METHODS //
public:
  // To avoid template instantiations proliferating.
  /// Log one single value of any type recognized by ostream's <<.
  Logger& operator<<(char const* const cString) { return privateInsert(static_cast<char const*>(cString)); }
  /// Log one single value of any type recognized by ostream's <<.
  Logger& operator<<(char* const cString) { return privateInsert(static_cast<char const*>(cString)); }
  /// Log one single value of any type recognized by ostream's <<.
  template<typename Value>
  // Not 'Values const &' as Timer's << is not const.
  Logger& operator<<(Value&& value)
  {
    return privateInsert(value);
  }

  // Log the current time.
  Logger& currentTime()
  {
    struct tm dateAndTime;
    auto const timeValue{ std::time(nullptr) };
    if (auto const time{ localtime_r(&timeValue, &dateAndTime) })
      return *this << std::put_time(time, DateTimeFormat) << ": ";
    else
      return *this << "UNKNOWN TIME: ";
  }
  /// Start a banner with the current time.
  Logger& banner() { return (*this << "\n▒▒ ").currentTime(); }

  /// Log "ERROR! ".
  Logger& error() { return *this << "\nERROR! "; }
  /// Log "Warning! ".
  Logger& warning() { return *this << "\nWarning! "; }

  /// Log the current error condition (if any) of stream, else "Success. ".
  Logger& streamCondition(std::ios const& stream)
  {
    if (stream.good()) {
      return *this << "Success. ";
    } else {
      error();
      if (stream.bad())
        return *this << "I/O error. ";
      else if (stream.eof())
        return *this << "End of file reached. ";
      else
        return *this << "Failed. ";
    }
  }
};

/*
***********
** CLASS **
***********
*/

/** Allocator that just does NOT construct/initialize Type but is otherwise a subclass of BaseAllocator
    (defaulted to std::allocator<Type>).
    Only construct() is redefined, to nothing. Rebind allocators are set to BaseAllocator's own rebinds.
    Used to speed up creating arithmetic types that do not need to be initialized (zeroed).
*/
template<typename Type, typename BaseAllocator = std::allocator<Type>>
class NoConstructAllocator : public BaseAllocator
{
  // DEFINITIONS //
protected:
  // Rebind allocators are set to BaseAllocator's own rebinds.
  template<typename Rebind>
  using rebind = typename BaseAllocator::template rebind<Rebind>;

  // PUBLIC INSTANCE METHODS //
public:
  // Empty construct().
  void construct(Type*) const noexcept {}
};

/*
***********
** CLASS **
***********
*/

/** Generate random booleans using every bit of an (expensive) random integer
    provided in a shared/unique pointer, that is regenerated only when exhausted.
*/
// Nothing to de-templatize.
template<typename RandomIntegerPointer>
class RandomBoolean
{
  // STATIC ASSERT //
  static_assert(RandomIntegerPointer::element_type::min() == 0, "RandomIntegerPointer::element_type::min() MUST be 0.");

  // INSTANCE VARIABLES //
private:
  size_t myShiftPlusOne{ 0 };
  RandomIntegerPointer myRandomInteger;
  decltype((*myRandomInteger)()) myStoredRandomInteger{ 0 }; // Initialized to silence warnings.

  // DEFINITIONS //
private:
  constexpr static decltype(myShiftPlusOne) const StoredIntegerBitSize{ sizeof(myStoredRandomInteger) * 8 };
  constexpr static decltype(myShiftPlusOne) const MaximumShift{ StoredIntegerBitSize - 1 };
  constexpr static decltype(myShiftPlusOne) const RandomIntegerBitSize{ RandomIntegerPointer::element_type::word_size };
  constexpr static decltype(myShiftPlusOne) const EmptyBitsCount{ StoredIntegerBitSize - RandomIntegerBitSize };

  // PRIVATE INSTANCE METHODS //
private:
  void construct() noexcept(false)
  {
    // Sanity test.
    if (not myRandomInteger)
      throw std::logic_error(String(+"Null randomInteger in: ", +__PRETTY_FUNCTION__, '.'));
  }

  // CONSTRUCTORS //
public:
  // Deleted.
  RandomBoolean() = delete;

  /*
    // Inhibits CTAD (Class Template Argument Deduction).
    template<typename RandomIntegerPointer>
      RandomBoolean(RandomIntegerPointer&& randomInteger) noexcept(noexcept(construct()))
      : myRandomInteger(std::forward<RandomIntegerPointer>(randomInteger))
    { construct(); }
  */
  explicit RandomBoolean(RandomIntegerPointer&& randomInteger) noexcept(noexcept(construct()))
    : myRandomInteger(std::move(randomInteger))
  {
    construct();
  }
  explicit RandomBoolean(RandomIntegerPointer const& randomInteger) noexcept(noexcept(construct()))
    : myRandomInteger(randomInteger)
  {
    construct();
  }

  // PUBLIC INSTANCE METHODS //
public:
  /// @return A random boolean.
  bool operator()() noexcept(noexcept((*myRandomInteger)()))
  {
    if (not myShiftPlusOne) {
      myShiftPlusOne = RandomIntegerBitSize;
      myStoredRandomInteger = (*myRandomInteger)();
    }

    return static_cast<bool>((myStoredRandomInteger SHIFT_INCREASE(--myShiftPlusOne + EmptyBitsCount))
                               SHIFT_DECREASE MaximumShift);
  }
};

/*
***********
** CLASS **
***********
*/

class Timer
{
  // DEFINITIONS //
private:
#define NOW_TICKS (std::chrono::high_resolution_clock::now().time_since_epoch().count())

public:
  constexpr static auto const TicksPerSecond{ std::chrono::high_resolution_clock::period::den /
                                              std::chrono::high_resolution_clock::period::num };

  // INSTANCE VARIABLES //
private:
  decltype(NOW_TICKS) myLapTicks;
  decltype(NOW_TICKS) myStartTicks;

  // PUBLIC INSTANCE METHODS //
public:
  /// Start or restart.
  void restart() noexcept(noexcept(NOW_TICKS))
  {
    myLapTicks = 0;
    myStartTicks = NOW_TICKS;
  }
  void lap() noexcept(noexcept(NOW_TICKS))
  {
    myLapTicks = NOW_TICKS;
  }

  // CONSTRUCTOR //
public:
  Timer() noexcept(noexcept(restart()))
  {
    restart();
  }

  // PUBLIC INSTANCE METHODS //
public:
  decltype(auto) elapsedTicks() noexcept(noexcept(lap()))
  {
    if (not myLapTicks)
      lap();

    return myLapTicks - myStartTicks;
  }
  decltype(auto) elapsedSeconds() noexcept(noexcept(lap()))
  {
    return elapsedTicks() / TicksPerSecond;
  }
  decltype(auto) elapsedMilliseconds() noexcept(noexcept(lap()))
  {
    return elapsedTicks() / (TicksPerSecond / 1000);
  }
  decltype(auto) elapsedMicroseconds() noexcept(noexcept(lap()))
  {
    return elapsedTicks() / (TicksPerSecond / 1000'000);
  }

  template<typename Char>
  void printOn(std::basic_ostream<Char>& outputStream)
  {
    auto const microSeconds{ elapsedMicroseconds() };

    // Microseconds.
    if (microSeconds < 10'000)
      outputStream << microSeconds << " μs";
    // Rounded milliseconds.
    else if (microSeconds < 10'000'000)
      outputStream << ((microSeconds + 500) / 1000) << " ms";
    // Rounded seconds.
    else
      outputStream << ((microSeconds + 500'000) / 1000'000) << " s";
  }

  template<typename Char>
  friend std::basic_ostream<Char>& operator<<(std::basic_ostream<Char>& outputStream, Timer& timer)
  {
    timer.printOn(outputStream);

    return outputStream;
  }
};
