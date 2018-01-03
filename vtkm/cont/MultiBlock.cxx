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

#if defined(VTKM_ENABLE_MPI)
#include <diy/decomposition.hpp>
#include <diy/master.hpp>
#include <diy/partners/all-reduce.hpp>
#include <diy/partners/swap.hpp>
#include <diy/reduce.hpp>

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
  using ValueType = typename PortalType::ValueType;
  std::vector<ValueType> result(portal.GetNumberOfValues());
  vtkm::cont::ArrayPortalToIterators<PortalType> iterators(portal);
  std::copy(iterators.GetBegin(), iterators.GetEnd(), result.begin());
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

#endif

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
#if defined(VTKM_ENABLE_MPI)
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
#else
  return this->GetNumberOfBlocks();
#endif
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
void MultiBlock::AddBlock(vtkm::cont::DataSet& ds)
{
  this->Blocks.insert(this->Blocks.end(), ds);
  return;
}

void MultiBlock::AddBlocks(std::vector<vtkm::cont::DataSet>& mblocks)
{
  this->Blocks.insert(this->Blocks.end(), mblocks.begin(), mblocks.end());
  return;
}

VTKM_CONT
void MultiBlock::InsertBlock(vtkm::Id index, vtkm::cont::DataSet& ds)
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
void MultiBlock::ReplaceBlock(vtkm::Id index, vtkm::cont::DataSet& ds)
{
  if (index < static_cast<vtkm::Id>(this->Blocks.size()))
    this->Blocks.at(static_cast<std::size_t>(index)) = ds;
  else
  {
    std::string msg = "invalid replace position\n ";
    throw ErrorExecution(msg);
  }
}

VTKM_CONT
vtkm::Bounds MultiBlock::GetBounds(vtkm::Id coordinate_system_index) const
{
  return this->GetBounds(coordinate_system_index,
                         VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG(),
                         VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG());
}

template <typename TypeList>
VTKM_CONT vtkm::Bounds MultiBlock::GetBounds(vtkm::Id coordinate_system_index, TypeList) const
{
  VTKM_IS_LIST_TAG(TypeList);
  return this->GetBounds(
    coordinate_system_index, TypeList(), VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG());
}
template <typename TypeList, typename StorageList>
VTKM_CONT vtkm::Bounds MultiBlock::GetBounds(vtkm::Id coordinate_system_index,
                                             TypeList,
                                             StorageList) const
{
  VTKM_IS_LIST_TAG(TypeList);
  VTKM_IS_LIST_TAG(StorageList);

#if defined(VTKM_ENABLE_MPI)
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
      *data = coords.GetBounds(TypeList(), StorageList());
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

#else
  const vtkm::Id index = coordinate_system_index;
  const size_t num_blocks = this->Blocks.size();

  vtkm::Bounds bounds;
  for (size_t i = 0; i < num_blocks; ++i)
  {
    vtkm::Bounds block_bounds = this->GetBlockBounds(i, index, TypeList(), StorageList());
    bounds.Include(block_bounds);
  }
  return bounds;
#endif
}

VTKM_CONT
vtkm::Bounds MultiBlock::GetBlockBounds(const std::size_t& block_index,
                                        vtkm::Id coordinate_system_index) const
{
  return this->GetBlockBounds(block_index,
                              coordinate_system_index,
                              VTKM_DEFAULT_COORDINATE_SYSTEM_TYPE_LIST_TAG(),
                              VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG());
}

template <typename TypeList>
VTKM_CONT vtkm::Bounds MultiBlock::GetBlockBounds(const std::size_t& block_index,
                                                  vtkm::Id coordinate_system_index,
                                                  TypeList) const
{
  VTKM_IS_LIST_TAG(TypeList);
  return this->GetBlockBounds(block_index,
                              coordinate_system_index,
                              TypeList(),
                              VTKM_DEFAULT_COORDINATE_SYSTEM_STORAGE_LIST_TAG());
}

template <typename TypeList, typename StorageList>
VTKM_CONT vtkm::Bounds MultiBlock::GetBlockBounds(const std::size_t& block_index,
                                                  vtkm::Id coordinate_system_index,
                                                  TypeList,
                                                  StorageList) const
{
  VTKM_IS_LIST_TAG(TypeList);
  VTKM_IS_LIST_TAG(StorageList);

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
  return coords.GetBounds(TypeList(), StorageList());
}

VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> MultiBlock::GetGlobalRange(const int& index) const
{
  return this->GetGlobalRange(index, VTKM_DEFAULT_TYPE_LIST_TAG(), VTKM_DEFAULT_STORAGE_LIST_TAG());
}

template <typename TypeList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> MultiBlock::GetGlobalRange(const int& index,
                                                                          TypeList) const
{
  VTKM_IS_LIST_TAG(TypeList);
  return this->GetGlobalRange(index, TypeList(), VTKM_DEFAULT_STORAGE_LIST_TAG());
}

template <typename TypeList, typename StorageList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> MultiBlock::GetGlobalRange(const int& index,
                                                                          TypeList,
                                                                          StorageList) const
{
  VTKM_IS_LIST_TAG(TypeList);
  VTKM_IS_LIST_TAG(StorageList);

  assert(this->Blocks.size() > 0);
  vtkm::cont::Field field = this->Blocks.at(0).GetField(index);
  std::string field_name = field.GetName();
  return this->GetGlobalRange(field_name, TypeList(), StorageList());
}

VTKM_CONT
vtkm::cont::ArrayHandle<vtkm::Range> MultiBlock::GetGlobalRange(const std::string& field_name) const
{
  return this->GetGlobalRange(
    field_name, VTKM_DEFAULT_TYPE_LIST_TAG(), VTKM_DEFAULT_STORAGE_LIST_TAG());
}

template <typename TypeList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range> MultiBlock::GetGlobalRange(
  const std::string& field_name,
  TypeList) const
{
  VTKM_IS_LIST_TAG(TypeList);
  return this->GetGlobalRange(field_name, TypeList(), VTKM_DEFAULT_STORAGE_LIST_TAG());
}

template <typename TypeList, typename StorageList>
VTKM_CONT vtkm::cont::ArrayHandle<vtkm::Range>
MultiBlock::GetGlobalRange(const std::string& field_name, TypeList, StorageList) const
{
#if defined(VTKM_ENABLE_MPI)
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
      const vtkm::cont::ArrayHandle<vtkm::Range> range = field.GetRange(TypeList(), StorageList());
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
#else
  bool valid_field = true;
  const size_t num_blocks = this->Blocks.size();

  vtkm::cont::ArrayHandle<vtkm::Range> range;
  vtkm::Id num_components = 0;

  for (size_t i = 0; i < num_blocks; ++i)
  {
    if (!this->Blocks[i].HasField(field_name))
    {
      valid_field = false;
      break;
    }

    const vtkm::cont::Field& field = this->Blocks[i].GetField(field_name);
    vtkm::cont::ArrayHandle<vtkm::Range> sub_range = field.GetRange(TypeList(), StorageList());

    vtkm::cont::ArrayHandle<vtkm::Range>::PortalConstControl sub_range_control =
      sub_range.GetPortalConstControl();
    vtkm::cont::ArrayHandle<vtkm::Range>::PortalControl range_control = range.GetPortalControl();

    if (i == 0)
    {
      num_components = sub_range_control.GetNumberOfValues();
      range = sub_range;
      continue;
    }

    vtkm::Id components = sub_range_control.GetNumberOfValues();

    if (components != num_components)
    {
      std::stringstream msg;
      msg << "GetRange call failed. The number of components (" << components << ") in field "
          << field_name << " from block " << i << " does not match the number of components "
          << "(" << num_components << ") in block 0";
      throw ErrorExecution(msg.str());
    }


    for (vtkm::Id c = 0; c < components; ++c)
    {
      vtkm::Range s_range = sub_range_control.Get(c);
      vtkm::Range c_range = range_control.Get(c);
      c_range.Include(s_range);
      range_control.Set(c, c_range);
    }
  }

  if (!valid_field)
  {
    std::string msg = "GetRange call failed. ";
    msg += " Field " + field_name + " did not exist in at least one block.";
    throw ErrorExecution(msg);
  }

  return range;
#endif
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
