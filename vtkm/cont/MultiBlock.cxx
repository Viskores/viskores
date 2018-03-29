//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/StaticAssert.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/AssignerMultiBlock.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DecomposerMultiBlock.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/EnvironmentTracker.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/MultiBlock.h>

// clang-format off
VTKM_THIRDPARTY_PRE_INCLUDE
#include <vtkm/thirdparty/diy/Configure.h>
#include VTKM_DIY(diy/decomposition.hpp)
#include VTKM_DIY(diy/master.hpp)
#include VTKM_DIY(diy/partners/all-reduce.hpp)
#include VTKM_DIY(diy/partners/swap.hpp)
#include VTKM_DIY(diy/reduce.hpp)
VTKM_THIRDPARTY_POST_INCLUDE
// clang-format on

namespace vtkm
{
namespace cont
{
namespace detail
{
template <typename PortalType>
VTKM_CONT std::vector<typename PortalType::ValueType> CopyArrayPortalToVector(
  const PortalType& portal)
{
  const size_t count =
    portal.GetNumberOfValues() > 0 ? static_cast<size_t>(portal.GetNumberOfValues()) : 0;
  using ValueType = typename PortalType::ValueType;
  std::vector<ValueType> result(count);
  if (count > 0)
  {
    vtkm::cont::ArrayPortalToIterators<PortalType> iterators(portal);
    std::copy(iterators.GetBegin(), iterators.GetEnd(), result.begin());
  }
  return result;
}

template <typename T>
const vtkm::cont::DataSet& GetBlock(const vtkm::cont::MultiBlock& mb, const T&);

template <>
const vtkm::cont::DataSet& GetBlock(const vtkm::cont::MultiBlock& mb,
                                    const diy::Master::ProxyWithLink& cp)
{
  const int lid = cp.master()->lid(cp.gid());
  return mb.GetBlock(lid);
}
}
}
}

namespace vtkm
{
namespace cont
{

VTKM_CONT
MultiBlock::MultiBlock(const vtkm::cont::DataSet& ds)
{
  this->Blocks.insert(this->Blocks.end(), ds);
}

VTKM_CONT
MultiBlock::MultiBlock(const vtkm::cont::MultiBlock& src)
{
  this->Blocks = src.GetBlocks();
}

VTKM_CONT
MultiBlock::MultiBlock(const std::vector<vtkm::cont::DataSet>& mblocks)
{
  this->Blocks = mblocks;
}

VTKM_CONT
MultiBlock::MultiBlock(vtkm::Id size)
{
  this->Blocks.reserve(static_cast<std::size_t>(size));
}

VTKM_CONT
MultiBlock::MultiBlock()
{
}

VTKM_CONT
MultiBlock::~MultiBlock()
{
}

VTKM_CONT
MultiBlock& MultiBlock::operator=(const vtkm::cont::MultiBlock& src)
{
  this->Blocks = src.GetBlocks();
  return *this;
}

VTKM_CONT
vtkm::cont::Field MultiBlock::GetField(const std::string& field_name, const int& block_index)
{
  assert(block_index >= 0);
  assert(static_cast<std::size_t>(block_index) < this->Blocks.size());
  return this->Blocks[static_cast<std::size_t>(block_index)].GetField(field_name);
}

VTKM_CONT
vtkm::Id MultiBlock::GetNumberOfBlocks() const
{
  return static_cast<vtkm::Id>(this->Blocks.size());
}

VTKM_CONT
vtkm::Id MultiBlock::GetGlobalNumberOfBlocks() const
{
  auto world = vtkm::cont::EnvironmentTracker::GetCommunicator();
  const auto local_count = this->GetNumberOfBlocks();

  diy::Master master(world, 1, -1);
  int block_not_used = 1;
  master.add(world.rank(), &block_not_used, new diy::Link());
  // empty link since we're only using collectives.
  master.foreach ([=](void*, const diy::Master::ProxyWithLink& cp) {
    cp.all_reduce(local_count, std::plus<vtkm::Id>());
  });
  master.process_collectives();
  vtkm::Id global_count = master.proxy(0).get<vtkm::Id>();
  return global_count;
}

VTKM_CONT
const vtkm::cont::DataSet& MultiBlock::GetBlock(vtkm::Id blockId) const
{
  return this->Blocks[static_cast<std::size_t>(blockId)];
}

VTKM_CONT
const std::vector<vtkm::cont::DataSet>& MultiBlock::GetBlocks() const
{
  return this->Blocks;
}

VTKM_CONT
void MultiBlock::AddBlock(const vtkm::cont::DataSet& ds)
{
  this->Blocks.insert(this->Blocks.end(), ds);
  return;
}

void MultiBlock::AddBlocks(const std::vector<vtkm::cont::DataSet>& mblocks)
{
  this->Blocks.insert(this->Blocks.end(), mblocks.begin(), mblocks.end());
  return;
}

VTKM_CONT
void MultiBlock::InsertBlock(vtkm::Id index, const vtkm::cont::DataSet& ds)
{
  if (index <= static_cast<vtkm::Id>(this->Blocks.size()))
    this->Blocks.insert(this->Blocks.begin() + index, ds);
  else
  {
    std::string msg = "invalid insert position\n ";
    throw ErrorExecution(msg);
  }
}

VTKM_CONT
void MultiBlock::ReplaceBlock(vtkm::Id index, const vtkm::cont::DataSet& ds)
{
  if (index < static_cast<vtkm::Id>(this->Blocks.size()))
    this->Blocks.at(static_cast<std::size_t>(index)) = ds;
  else
  {
    std::string msg = "invalid replace position\n ";
    throw ErrorExecution(msg);
  }
}

VTKM_CONT vtkm::Bounds MultiBlock::GetBounds(vtkm::Id coordinate_system_index) const
{
  auto world = vtkm::cont::EnvironmentTracker::GetCommunicator();
  diy::Master master(world,
                     1,
                     -1,
                     []() -> void* { return new vtkm::Bounds(); },
                     [](void* ptr) { delete static_cast<vtkm::Bounds*>(ptr); });

  vtkm::cont::AssignerMultiBlock assigner(*this);

  // populate master with blocks from `this`.
  diy::decompose(world.rank(), assigner, master);

  auto self = (*this);
  master.foreach ([&](vtkm::Bounds* data, const diy::Master::ProxyWithLink& cp) {
    const vtkm::cont::DataSet& block = vtkm::cont::detail::GetBlock(self, cp);
    try
    {
      vtkm::cont::CoordinateSystem coords = block.GetCoordinateSystem(coordinate_system_index);
      *data = coords.GetBounds();
    }
    catch (const vtkm::cont::Error&)
    {
    }
  });

  vtkm::cont::DecomposerMultiBlock decomposer(assigner);
  diy::RegularSwapPartners partners(decomposer, /*k=*/2);

  auto callback =
    [](vtkm::Bounds* data, const diy::ReduceProxy& srp, const diy::RegularSwapPartners&) {
      // 1. dequeue.
      std::vector<int> incoming;
      srp.incoming(incoming);
      vtkm::Bounds message;
      for (const int gid : incoming)
      {
        srp.dequeue(gid, message);
        data->Include(message);
      }
      // 2. enqueue
      for (int cc = 0; cc < srp.out_link().size(); ++cc)
      {
        srp.enqueue(srp.out_link().target(cc), *data);
      }
    };
  diy::reduce(master, assigner, partners, callback);
  if (master.size())
  {
    return (*master.block<vtkm::Bounds>(0));
  }
  return vtkm::Bounds();
}

VTKM_CONT vtkm::Bounds MultiBlock::GetBlockBounds(const std::size_t& block_index,
                                                  vtkm::Id coordinate_system_index) const
{
  const vtkm::Id index = coordinate_system_index;
  vtkm::cont::CoordinateSystem coords;
  try
  {
    coords = this->Blocks[block_index].GetCoordinateSystem(index);
  }
  catch (const vtkm::cont::Error& error)
  {
    std::stringstream msg;
    msg << "GetBounds call failed. vtk-m error was encountered while "
        << "attempting to get coordinate system " << index << " from "
        << "block " << block_index << ". vtkm error message: " << error.GetMessage();
    throw ErrorExecution(msg.str());
  }
  return coords.GetBounds();
}

VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> MultiBlock::GetGlobalRange(const int& index) const
{
  assert(this->Blocks.size() > 0);
  vtkm::cont::Field field = this->Blocks.at(0).GetField(index);
  std::string field_name = field.GetName();
  return this->GetGlobalRange(field_name);
}

VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> MultiBlock::GetGlobalRange(
  const std::string& field_name) const
{
  using BlockMetaData = std::vector<vtkm::Range>;

  auto comm = vtkm::cont::EnvironmentTracker::GetCommunicator();
  diy::Master master(comm,
                     1,
                     -1,
                     []() -> void* { return new BlockMetaData(); },
                     [](void* ptr) { delete static_cast<BlockMetaData*>(ptr); });

  vtkm::cont::AssignerMultiBlock assigner(*this);

  diy::decompose(comm.rank(), assigner, master);

  auto self = (*this);
  master.foreach ([&](BlockMetaData* data, const diy::Master::ProxyWithLink& cp) {
    const vtkm::cont::DataSet& block = vtkm::cont::detail::GetBlock(self, cp);
    if (block.HasField(field_name))
    {
      auto field = block.GetField(field_name);
      const vtkm::cont::ArrayHandle<vtkm::Range> range = field.GetRange();
      *data = vtkm::cont::detail::CopyArrayPortalToVector(range.GetPortalConstControl());
    }
  });

  vtkm::cont::DecomposerMultiBlock decomposer(assigner);
  diy::RegularSwapPartners partners(decomposer, /*k=*/2);
  auto callback =
    [](BlockMetaData* data, const diy::ReduceProxy& srp, const diy::RegularSwapPartners&) {
      std::vector<int> incoming;
      srp.incoming(incoming);

      // 1. dequeue
      BlockMetaData message;
      for (const int gid : incoming)
      {
        srp.dequeue(gid, message);
        data->resize(std::max(data->size(), message.size()));
        for (size_t cc = 0; cc < data->size(); ++cc)
        {
          (*data)[cc].Include(message[cc]);
        }
      }
      // 2. enqueue
      for (int cc = 0; cc < srp.out_link().size(); ++cc)
      {
        srp.enqueue(srp.out_link().target(cc), *data);
      }
    };

  diy::reduce(master, assigner, partners, callback);

  BlockMetaData ranges;
  if (master.size())
  {
    ranges = *(master.block<BlockMetaData>(0));
  }
  vtkm::cont::ArrayHandle<vtkm::Range> tmprange = vtkm::cont::make_ArrayHandle(ranges);
  vtkm::cont::ArrayHandle<vtkm::Range> range;
  vtkm::cont::ArrayCopy(vtkm::cont::make_ArrayHandle(ranges), range);
  return range;
}

VTKM_CONT
void MultiBlock::PrintSummary(std::ostream& stream) const
{
  stream << "block "
         << "\n";

  for (size_t block_index = 0; block_index < this->Blocks.size(); ++block_index)
  {
    stream << "block " << block_index << "\n";
    this->Blocks[block_index].PrintSummary(stream);
  }
}
}
} // namespace vtkm::cont
