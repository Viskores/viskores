//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/Hash.h>

#include <vtkm/testing/Testing.h>

#include <algorithm>
#include <vector>

namespace
{

VTKM_CONT
static void CheckUnique(std::vector<vtkm::HashType> hashes)
{
  std::sort(hashes.begin(), hashes.end());

  for (std::size_t index = 1; index < hashes.size(); ++index)
  {
    VTKM_TEST_ASSERT(hashes[index - 1] != hashes[index], "Found duplicate hashes.");
  }
}

template <typename VecType>
VTKM_CONT static void DoHashTest(VecType&&)
{
  std::cout << "Test hash for " << vtkm::testing::TypeName<VecType>::Name() << std::endl;

  const vtkm::Id NUM_HASHES = 100;
  std::cout << "  Make sure the first " << NUM_HASHES << " values are unique." << std::endl;
  // There is a small probability that two values of these 100 could be the same. If this test
  // fails we could just be unlucky (and have to use a different set of 100 hashes), but it is
  // suspicious and you should double check the hashes.
  std::vector<vtkm::HashType> hashes;
  hashes.reserve(NUM_HASHES);
  for (vtkm::Id index = 0; index < NUM_HASHES; ++index)
  {
    hashes.push_back(vtkm::Hash(TestValue(index, VecType())));
  }
  CheckUnique(hashes);

  std::cout << "  Try close values that should have different hashes." << std::endl;
  hashes.clear();
  VecType vec = TestValue(5, VecType());
  hashes.push_back(vtkm::Hash(vec));
  vec[0] = vec[0] + 1;
  hashes.push_back(vtkm::Hash(vec));
  vec[1] = vec[1] - 1;
  hashes.push_back(vtkm::Hash(vec));
  CheckUnique(hashes);
}

VTKM_CONT
static void TestHash()
{
  DoHashTest(vtkm::Id2());
  DoHashTest(vtkm::Id3());
  DoHashTest(vtkm::Vec<vtkm::Id, 10>());
  DoHashTest(vtkm::IdComponent2());
  DoHashTest(vtkm::IdComponent3());
  DoHashTest(vtkm::Vec<vtkm::IdComponent, 10>());
}

} // anonymous namespace

int UnitTestHash(int argc, char* argv[])
{
  return vtkm::testing::Testing::Run(TestHash, argc, argv);
}
