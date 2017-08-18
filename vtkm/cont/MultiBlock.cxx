//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#include <vtkm/StaticAssert.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/MultiBlock.h>
namespace vtkm
{
namespace cont
{

VTKM_CONT
MultiBlock::MultiBlock(const vtkm::cont::DataSet& ds)
  : capacitytag(false)
{
  this->blocks.insert(blocks.end(), ds);
}

VTKM_CONT
MultiBlock::MultiBlock(const vtkm::cont::MultiBlock& src)
  : capacitytag(false)
{
  this->blocks = src.GetBlocks();
}

VTKM_CONT
MultiBlock::MultiBlock(const std::vector<vtkm::cont::DataSet>& mblocks)
  : capacitytag(false)
{
  this->blocks = mblocks;
}

VTKM_CONT
MultiBlock::MultiBlock(vtkm::Id size)
{
  this->capacity = size;
  this->capacitytag = true;
  this->blocks.reserve(static_cast<std::size_t>(size));
}

VTKM_CONT
MultiBlock::MultiBlock()
  : capacitytag(false)
{
}

VTKM_CONT
MultiBlock::~MultiBlock()
{
}

VTKM_CONT
MultiBlock& MultiBlock::operator=(const vtkm::cont::MultiBlock& src)
{
  this->blocks = src.GetBlocks();
  return *this;
}

VTKM_CONT
vtkm::cont::Field MultiBlock::GetField(const std::string& field_name, const int& block_index)
{
  assert(block_index >= 0);
  assert(block_index < static_cast<std::size_t>(blocks.size()));
  return blocks[static_cast<std::size_t>(block_index)].GetField(field_name);
}

VTKM_CONT
vtkm::Id MultiBlock::GetNumberOfBlocks() const
{
  return static_cast<vtkm::Id>(this->blocks.size());
}

VTKM_CONT
vtkm::Id MultiBlock::GetCapacity()
{
  if (this->capacitytag)
    return this->capacity;
  else
    std::cout << "structure capacity not set yet\n";
  return 0;
}


VTKM_CONT
void MultiBlock::SetCapacity(vtkm::Id size)
{
  if (static_cast<vtkm::Id>(this->blocks.capacity()) < size)
  {
    this->capacity = size;
    this->capacitytag = true;
    this->blocks.reserve(static_cast<std::size_t>(size));
  }
  else
  {
    std::cout << "required size is too small and rejected<<\n";
  }
}
UnitTests_vtkm_cont_testing

  VTKM_CONT const vtkm::cont::DataSet&
  MultiBlock::GetBlock(vtkm::Id blockId) const
{
  return this->blocks[(std::size_t)blockId];
}

VTKM_CONT
const std::vector<vtkm::cont::DataSet>& MultiBlock::GetBlocks() const
{
  return this->blocks;
}
VTKM_CONT
void MultiBlock::AddBlock(vtkm::cont::DataSet& ds)
{
  this->blocks.insert(blocks.end(), ds);
  return;
}

VTKM_CONT
void MultiBlock::AddBlocks(std::vector<vtkm::cont::DataSet>& mblocks)
{
  for (std::size_t i = 0; i < mblocks.size(); i++)
  {
    AddBlock(mblocks[i]);
  }
  return;
}

VTKM_CONT
void MultiBlock::InsertBlock(vtkm::Id index, vtkm::cont::DataSet& ds)
{
  if (index < static_cast<vtkm::Id>(blocks.size()))
    this->blocks.insert(blocks.begin() + index, ds);
  else
    std::cout << "insertion failure, invalid insertion posotion\n";
}

VTKM_CONT
void MultiBlock::OverWriteBlock(vtkm::Id index, vtkm::cont::DataSet& ds)
{
  if (index < static_cast<vtkm::Id>(blocks.size()))
    this->blocks.at(static_cast<std::size_t>(index)) = ds;
  else
    std::cout << "invalid overwrite posotion\n";
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

  const vtkm::Id index = coordinate_system_index;
  const size_t num_blocks = blocks.size();

  vtkm::Bounds bounds;
  for (size_t i = 0; i < num_blocks; ++i)
  {
    vtkm::Bounds block_bounds = this->GetBlockBounds(i, index, TypeList(), StorageList());
    bounds.Include(block_bounds);
  }

  return bounds;
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
    coords = blocks[block_index].GetCoordinateSystem(index);
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

  assert(blocks.size() > 0);
  vtkm::cont::Field field = blocks.at(0).GetField(index);
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
  bool valid_field = true;
  const size_t num_blocks = blocks.size();

  vtkm::cont::ArrayHandle<vtkm::Range> range;
  vtkm::Id num_components = 0;

  for (size_t i = 0; i < num_blocks; ++i)
  {
    if (!blocks[i].HasField(field_name))
    {
      valid_field = false;
      break;
    }

    const vtkm::cont::Field& field = blocks[i].GetField(field_name);
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
}

VTKM_CONT
void MultiBlock::PrintSummary(std::ostream& stream) const
{
  stream << "block "
         << "\n";

  for (size_t block_index = 0; block_index < blocks.size(); ++block_index)
  {
    stream << "block " << block_index << "\n";
    blocks[block_index].PrintSummary(stream);
  }
}
}
} // namespace vtkm::cont
