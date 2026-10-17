/*
  TypeInfoSetStorageTest - a test program for libosmscout
  Copyright (C) 2026  Tim Teulings

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <limits>
#include <memory>
#include <new>
#include <random>
#include <set>
#include <string>
#include <vector>

#include <osmscout/TypeConfig.h>
#include <osmscout/TypeInfoSet.h>

#include <catch2/catch_test_macros.hpp>

/*
 * Tests for the contract of the storage of a set of types (spec type-set): the bytes a
 * set allocates follow the types it contains and not the number of types the type
 * configuration defines.
 *
 * The tests build their own type configuration, so they need no database and no type
 * definition file. The allocation counter replaces the global operator new/delete (the same
 * approach as Tests/src/MapPainterAreaPreparationTest.cpp); it is disabled in sanitizer
 * builds, whose runtimes provide their own operators.
 */

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define TYPE_SET_HAVE_ALLOCATION_COUNTER 0
#elif defined(__has_feature)
#if __has_feature(memory_sanitizer) || __has_feature(address_sanitizer) || __has_feature(thread_sanitizer)
#define TYPE_SET_HAVE_ALLOCATION_COUNTER 0
#else
#define TYPE_SET_HAVE_ALLOCATION_COUNTER 1
#endif
#else
#define TYPE_SET_HAVE_ALLOCATION_COUNTER 1
#endif

#if TYPE_SET_HAVE_ALLOCATION_COUNTER

namespace {

  /**
   * Sums the size of every heap allocation of the test process. The difference between two
   * calls measures the bytes the code in between allocates, which is the property that tells
   * whether a set of types allocates per defined type or per type it contains. Aligned
   * allocations are not counted.
   */
  std::atomic<size_t> allocatedBytes{0};
}

void* operator new(std::size_t size)
{
  allocatedBytes.fetch_add(size,std::memory_order_relaxed);

  void* memory=std::malloc(size);

  if (memory==nullptr) {
    throw std::bad_alloc();
  }

  return memory;
}

void* operator new[](std::size_t size)
{
  return ::operator new(size);
}

void* operator new(std::size_t size,
                   const std::nothrow_t&) noexcept
{
  allocatedBytes.fetch_add(size,std::memory_order_relaxed);

  return std::malloc(size);
}

void* operator new[](std::size_t size,
                     const std::nothrow_t& tag) noexcept
{
  return ::operator new(size,tag);
}

void operator delete(void* memory) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory) noexcept
{
  std::free(memory);
}

void operator delete(void* memory,
                     std::size_t /*size*/) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory,
                       std::size_t /*size*/) noexcept
{
  std::free(memory);
}

void operator delete(void* memory,
                     const std::nothrow_t&) noexcept
{
  std::free(memory);
}

void operator delete[](void* memory,
                       const std::nothrow_t&) noexcept
{
  std::free(memory);
}

size_t GetAllocatedBytes()
{
  return allocatedBytes.load(std::memory_order_relaxed);
}

#else

size_t GetAllocatedBytes()
{
  return 0;
}

#endif

namespace {

  /**
   * The number of types of the type set the backlog entry measures (671 -> 1527 types).
   */
  constexpr size_t definedTypeCount=1527;

  /**
   * The bytes a set of 'typeCount' defined types needs for a bit per defined type, rounded
   * to whole words of the bitmap.
   */
  size_t BitmapBytes(size_t typeCount)
  {
    constexpr size_t bitsPerWord=std::numeric_limits<uint64_t>::digits;

    return ((typeCount+bitsPerWord-1)/bitsPerWord)*sizeof(uint64_t);
  }

  osmscout::TypeConfigRef BuildTypeConfig(size_t typeCount)
  {
    osmscout::TypeConfigRef typeConfig=std::make_shared<osmscout::TypeConfig>();

    while (typeConfig->GetTypeCount()<typeCount) {
      typeConfig->RegisterType(std::make_shared<osmscout::TypeInfo>("_type_set_type_"+
                                                                std::to_string(typeConfig->GetTypeCount())));
    }

    return typeConfig;
  }

  /**
   * The types of the model of a set, in ascending index order
   */
  std::vector<size_t> Order(const std::set<size_t>& model)
  {
    return std::vector<size_t>(model.begin(),model.end());
  }

  /**
   * Asserts that the contained types, the size, the emptiness and the membership answers of the set
   * are the ones of the model
   */
  void RequireAgrees(const osmscout::TypeInfoSet& set,
                     const std::set<size_t>& model,
                     const osmscout::TypeConfig& typeConfig)
  {
    CAPTURE(set.Size(),model.size());
    REQUIRE(set.Size()==model.size());
    REQUIRE(set.Empty()==model.empty());

    std::vector<size_t> iterated;

    for (const auto& type : set) {
      iterated.push_back(type->GetIndex());
    }

    REQUIRE(iterated==Order(model));

    for (size_t index=0; index<typeConfig.GetTypeCount(); index++) {
      REQUIRE(set.IsSet(typeConfig.GetTypeInfo(index))==(model.find(index)!=model.end()));
    }
  }

  bool IntersectsModel(const std::set<size_t>& model,
                       const std::set<size_t>& otherModel)
  {
    for (size_t index : otherModel) {
      if (model.find(index)!=model.end()) {
        return true;
      }
    }

    return false;
  }
}

TEST_CASE("An empty set of a 1527-type configuration allocates a bit per defined type","[TypeInfoSet]")
{
  osmscout::TypeConfigRef typeConfig=BuildTypeConfig(definedTypeCount);

  REQUIRE(typeConfig->GetTypeCount()==definedTypeCount);

  size_t before=GetAllocatedBytes();
  osmscout::TypeInfoSet set(*typeConfig);
  size_t bytes=GetAllocatedBytes()-before;

  INFO("bytes allocated for an empty set of "<<typeConfig->GetTypeCount()<<" defined types: "<<bytes);
  REQUIRE(set.Empty());
  REQUIRE(set.Size()==0);
#if TYPE_SET_HAVE_ALLOCATION_COUNTER
  REQUIRE(bytes<=BitmapBytes(definedTypeCount));
#else
  SUCCEED("allocation counter is disabled in sanitizer builds");
#endif
}

TEST_CASE("A set of a 1527-type configuration allocates per contained type, not per defined type","[TypeInfoSet]")
{
  osmscout::TypeConfigRef typeConfig=BuildTypeConfig(definedTypeCount);

  REQUIRE(typeConfig->GetTypeCount()==definedTypeCount);

  const std::vector<osmscout::TypeInfoRef>& definedTypes=typeConfig->GetTypes();
  std::vector<osmscout::TypeInfoRef>       containedTypes{definedTypes[0],
                                                          definedTypes[definedTypeCount/2],
                                                          definedTypes[definedTypeCount-1]};

  size_t before=GetAllocatedBytes();
  osmscout::TypeInfoSet set(containedTypes);
  size_t bytes=GetAllocatedBytes()-before;

  INFO("bytes allocated for a set of "<<containedTypes.size()<<" types in "<<typeConfig->GetTypeCount()<<" defined types: "<<bytes);
  REQUIRE(set.Size()==containedTypes.size());
  REQUIRE(set.IsSet(containedTypes[0]));
  REQUIRE(set.IsSet(containedTypes[1]));
  REQUIRE(set.IsSet(containedTypes[2]));
#if TYPE_SET_HAVE_ALLOCATION_COUNTER
  // The bitmap, plus one slot per contained type, with room for the growth of the bitmap while the
  // set is filled by its type indices and for the growth of the container that holds the slots
  REQUIRE(bytes<=2*BitmapBytes(definedTypeCount)+2*containedTypes.size()*sizeof(osmscout::TypeInfoRef));
#else
  SUCCEED("allocation counter is disabled in sanitizer builds");
#endif
}

TEST_CASE("Iteration of a set yields the contained types in type index order","[TypeInfoSet]")
{
  const size_t typeCount=64;
  osmscout::TypeConfigRef typeConfig=BuildTypeConfig(typeCount);

  REQUIRE(typeConfig->GetTypeCount()==typeCount);

  const std::vector<osmscout::TypeInfoRef>& definedTypes=typeConfig->GetTypes();

  // Added in an order that differs from the order of the type indices
  std::vector<osmscout::TypeInfoRef> containedTypes{definedTypes[17],
                                                   definedTypes[3],
                                                   definedTypes[42],
                                                   definedTypes[8]};

  osmscout::TypeInfoSet set;

  for (const auto& type : containedTypes) {
    set.Set(type);
  }

  std::vector<osmscout::TypeInfoRef> iterated;

  for (const auto& type : set) {
    iterated.push_back(type);
  }

  std::vector<osmscout::TypeInfoRef> expected{definedTypes[3],
                                             definedTypes[8],
                                             definedTypes[17],
                                             definedTypes[42]};

  INFO("iterated "<<iterated.size()<<" types of "<<set.Size()<<" contained ones");
  REQUIRE(iterated==expected);
  REQUIRE(set.Size()==expected.size());

  for (const auto& type : expected) {
    REQUIRE(set.IsSet(type));
    REQUIRE(set.Intersects(osmscout::TypeInfoSet({type})));
  }
}

TEST_CASE("The operations of a set agree with a reference model","[TypeInfoSet]")
{
  const size_t typeCount=64;
  osmscout::TypeConfigRef typeConfig=BuildTypeConfig(typeCount);

  REQUIRE(typeConfig->GetTypeCount()==typeCount);

  const std::vector<osmscout::TypeInfoRef>& definedTypes=typeConfig->GetTypes();

  std::mt19937                           rng(4711);
  std::uniform_int_distribution<size_t> typeDistribution(0,typeCount-1);
  std::uniform_int_distribution<int>    operationDistribution(0,6);

  osmscout::TypeInfoSet set;
  osmscout::TypeInfoSet other;
  std::set<size_t>      setModel;
  std::set<size_t>      otherModel;

  auto fillOther=[&]() {
                  other.Clear();
                  otherModel.clear();

                  for (size_t i=0; i<3; i++) {
                    size_t index=typeDistribution(rng);

                    other.Set(definedTypes[index]);
                    otherModel.insert(index);
                  }
                };

  for (size_t step=0; step<200; step++) {
    int operation=operationDistribution(rng);

    switch (operation) {
      case 0:
      case 1: {
        size_t index=typeDistribution(rng);

        set.Set(definedTypes[index]);
        setModel.insert(index);
        break;
      }
      case 2: {
        size_t index=typeDistribution(rng);

        set.Remove(definedTypes[index]);
        setModel.erase(index);
        break;
      }
      case 3: {
        fillOther();

        set.Add(other);
        setModel.insert(otherModel.begin(),otherModel.end());
        break;
      }
      case 4: {
        fillOther();

        set.Remove(other);

        for (size_t index : otherModel) {
          setModel.erase(index);
        }
        break;
      }
      case 5: {
        fillOther();

        set.Intersection(other);

        for (auto iter=setModel.begin(); iter!=setModel.end();) {
          if (otherModel.find(*iter)==otherModel.end()) {
            iter=setModel.erase(iter);
          }
          else {
            ++iter;
          }
        }
        break;
      }
      default: {
        set.Clear();
        setModel.clear();
        break;
      }
    }

    CAPTURE(step,operation);
    RequireAgrees(set,setModel,*typeConfig);
    REQUIRE(set.Intersects(other)==IntersectsModel(setModel,otherModel));
    REQUIRE((set==other)==(setModel==otherModel));
    REQUIRE((set!=other)==(setModel!=otherModel));
    RequireAgrees(other,otherModel,*typeConfig);
  }

  // The operation that replaces the content of a set from a list of types
  set.Set(definedTypes);
  setModel.clear();

  for (size_t index=0; index<typeCount; index++) {
    setModel.insert(index);
  }

  RequireAgrees(set,setModel,*typeConfig);

  set.Set(std::vector<osmscout::TypeInfoRef>{definedTypes[7],definedTypes[1]});
  setModel.clear();
  setModel.insert(7);
  setModel.insert(1);

  RequireAgrees(set,setModel,*typeConfig);
}

TEST_CASE("A copy of a set keeps its own contents when the original changes","[TypeInfoSet]")
{
  const size_t typeCount=64;
  osmscout::TypeConfigRef typeConfig=BuildTypeConfig(typeCount);

  REQUIRE(typeConfig->GetTypeCount()==typeCount);

  const std::vector<osmscout::TypeInfoRef>& definedTypes=typeConfig->GetTypes();

  osmscout::TypeInfoSet original;

  original.Set(definedTypes[5]);
  original.Set(definedTypes[9]);

  osmscout::TypeInfoSet copy=original;

  REQUIRE(copy==original);
  REQUIRE(copy.Intersects(original));

  original.Clear();
  original.Set(definedTypes[20]);

  REQUIRE(original.Size()==1);
  REQUIRE(!original.IsSet(definedTypes[5]));
  REQUIRE(copy.Size()==2);
  REQUIRE(copy.IsSet(definedTypes[5]));
  REQUIRE(copy.IsSet(definedTypes[9]));
  REQUIRE(copy!=original);
  REQUIRE(!(copy==original));

  // Adapting to a type configuration with more types keeps the contained types
  osmscout::TypeConfigRef largerConfig=BuildTypeConfig(96);

  copy.Adapt(*largerConfig);

  REQUIRE(copy.Size()==2);
  REQUIRE(copy.IsSet(definedTypes[5]));
  REQUIRE(copy.IsSet(definedTypes[9]));
  REQUIRE(!copy.IsSet(largerConfig->GetTypes()[20]));
}
