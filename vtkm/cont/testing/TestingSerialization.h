//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_testing_TestingSerialization_h
#define vtk_m_cont_testing_TestingSerialization_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/VariantArrayHandle.h>
#include <vtkm/cont/testing/Testing.h>

#include <vtkm/thirdparty/diy/serialization.h>

#include <random>

namespace vtkm
{
namespace cont
{
namespace testing
{
namespace serialization
{

//-----------------------------------------------------------------------------
static std::default_random_engine generator;

template <typename T>
class UniformRandomValueGenerator
{
private:
  using DistributionType = typename std::conditional<std::is_integral<T>::value,
                                                     std::uniform_int_distribution<vtkm::Id>,
                                                     std::uniform_real_distribution<T>>::type;

public:
  UniformRandomValueGenerator()
    : Distribution(-127, 127)
  {
  }

  UniformRandomValueGenerator(T min, T max)
    : Distribution(static_cast<typename DistributionType::result_type>(min),
                   static_cast<typename DistributionType::result_type>(max))
  {
  }

  T operator()() { return static_cast<T>(this->Distribution(generator)); }

private:
  DistributionType Distribution;
};

template <typename T, typename Tag = typename vtkm::VecTraits<T>::HasMultipleComponents>
struct BaseScalarType;

template <typename T>
struct BaseScalarType<T, vtkm::VecTraitsTagSingleComponent>
{
  using Type = T;
};

template <typename T>
struct BaseScalarType<T, vtkm::VecTraitsTagMultipleComponents>
{
  using Type = typename BaseScalarType<typename vtkm::VecTraits<T>::ComponentType>::Type;
};

template <typename T>
using BaseScalarType_t = typename BaseScalarType<T>::Type;

template <typename T>
struct RandomValue_
{
  static T Make(UniformRandomValueGenerator<T>& rangen) { return static_cast<T>(rangen()); }
};

template <typename T, vtkm::IdComponent NumComponents>
struct RandomValue_<vtkm::Vec<T, NumComponents>>
{
  using VecType = vtkm::Vec<T, NumComponents>;

  static VecType Make(UniformRandomValueGenerator<BaseScalarType_t<T>>& rangen)
  {
    VecType val{};
    for (vtkm::IdComponent i = 0; i < NumComponents; ++i)
    {
      val[i] = RandomValue_<T>::Make(rangen);
    }
    return val;
  }
};

template <typename T>
struct RandomValue : RandomValue_<T>
{
  using RandomValue_<T>::Make;

  static T Make(BaseScalarType_t<T> min, BaseScalarType_t<T> max)
  {
    auto rangen = UniformRandomValueGenerator<BaseScalarType_t<T>>(min, max);
    return Make(rangen);
  }

  static T Make()
  {
    auto rangen = UniformRandomValueGenerator<BaseScalarType_t<T>>();
    return Make(rangen);
  }
};

template <typename T>
struct RandomArrayHandle
{
  static vtkm::cont::ArrayHandle<T> Make(UniformRandomValueGenerator<BaseScalarType_t<T>>& rangen,
                                         vtkm::Id length)
  {
    vtkm::cont::ArrayHandle<T> a;
    a.Allocate(length);

    for (vtkm::Id i = 0; i < length; ++i)
    {
      a.GetPortalControl().Set(i, RandomValue<T>::Make(rangen));
    }

    return a;
  }

  static vtkm::cont::ArrayHandle<T> Make(vtkm::Id length,
                                         BaseScalarType_t<T> min,
                                         BaseScalarType_t<T> max)
  {
    auto rangen = UniformRandomValueGenerator<BaseScalarType_t<T>>(min, max);
    return Make(rangen, length);
  }

  static vtkm::cont::ArrayHandle<T> Make(vtkm::Id length)
  {
    auto rangen = UniformRandomValueGenerator<BaseScalarType_t<T>>();
    return Make(rangen, length);
  }
};

//-----------------------------------------------------------------------------
template <typename T>
struct Block
{
  T send;
  T received;
};

template <typename T, typename TestEqualFunctor>
void TestSerialization(const T& obj, const TestEqualFunctor& test)
{
  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  vtkmdiy::Master master(comm);

  auto nblocks = comm.size();
  vtkmdiy::RoundRobinAssigner assigner(comm.size(), nblocks);

  std::vector<int> gids;
  assigner.local_gids(comm.rank(), gids);
  VTKM_ASSERT(gids.size() == 1);
  auto gid = gids[0];

  Block<T> block;
  block.send = obj;

  vtkmdiy::Link* link = new vtkmdiy::Link;
  vtkmdiy::BlockID neighbor;

  // send neighbor
  neighbor.gid = (gid < (nblocks - 1)) ? (gid + 1) : 0;
  neighbor.proc = assigner.rank(neighbor.gid);
  link->add_neighbor(neighbor);

  // recv neighbor
  neighbor.gid = (gid > 0) ? (gid - 1) : (nblocks - 1);
  neighbor.proc = assigner.rank(neighbor.gid);
  link->add_neighbor(neighbor);

  master.add(gid, &block, link);

  // compute, exchange, compute
  master.foreach ([](Block<T>* b, const vtkmdiy::Master::ProxyWithLink& cp) {
    cp.enqueue(cp.link()->target(0), b->send);
  });
  master.exchange();
  master.foreach ([](Block<T>* b, const vtkmdiy::Master::ProxyWithLink& cp) {
    cp.dequeue(cp.link()->target(1).gid, b->received);
  });

  comm.barrier();

  test(block.send, block.received);
}
}
}
}
} // vtkm::cont::testing::serialization

#endif // vtk_m_cont_testing_TestingSerialization_h
