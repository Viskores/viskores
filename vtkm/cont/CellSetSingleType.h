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
#ifndef vtk_m_cont_CellSetSingleType_h
#define vtk_m_cont_CellSetSingleType_h

#include <vtkm/CellShape.h>
#include <vtkm/CellTraits.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/CellSetExplicit.h>

#include <map>
#include <utility>

namespace vtkm {
namespace cont {


//Only works with fixed sized cell sets

template< typename ConnectivityStorageTag = VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG >
class CellSetSingleType  :
  public vtkm::cont::CellSetExplicit<
    typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag, //ShapeStorageTag
    typename vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>::StorageTag,  //NumIndicesStorageTag
    ConnectivityStorageTag,
    typename vtkm::cont::ArrayHandleCounting<vtkm::Id>::StorageTag  //IndexOffsetStorageTag
    >
{
  typedef vtkm::cont::CellSetExplicit<
      typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag,
      typename vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>::StorageTag,
      ConnectivityStorageTag,
      typename vtkm::cont::ArrayHandleCounting<vtkm::Id>::StorageTag > Superclass;

public:
  template<typename CellShapeTag>
  VTKM_CONT_EXPORT
  CellSetSingleType(CellShapeTag, const std::string &name = std::string())
    : Superclass(0, name, vtkm::CellTraits<CellShapeTag>::TOPOLOGICAL_DIMENSIONS),
      CellTypeAsId(CellShapeTag::Id)
  {
  }

  VTKM_CONT_EXPORT
  CellSetSingleType(const std::string &name = std::string())
    : Superclass(0, name, vtkm::CellTraits<CellShapeTagEmpty>::TOPOLOGICAL_DIMENSIONS),
      CellTypeAsId(CellShapeTagEmpty::Id)
  {
  }

  /// First method to add cells -- one at a time.
  VTKM_CONT_EXPORT
  void PrepareToAddCells(vtkm::Id numShapes, vtkm::Id connectivityMaxLen)
  {
    vtkm::IdComponent numberOfPointsPerCell = this->DetermineNumberOfPoints();
    const vtkm::UInt8 shapeTypeValue = static_cast<vtkm::UInt8>(this->CellTypeAsId);
    this->PointToCell.Shapes =
              vtkm::cont::make_ArrayHandleConstant(shapeTypeValue, numShapes);
    this->PointToCell.NumIndices =
              vtkm::cont::make_ArrayHandleConstant(numberOfPointsPerCell,
                                                   numShapes);
    this->PointToCell.IndexOffsets =
              vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0),
                                                   static_cast<vtkm::Id>(numberOfPointsPerCell),
                                                   numShapes );

    this->PointToCell.Connectivity.Allocate(connectivityMaxLen);

    this->NumberOfCells = 0;
    this->ConnectivityLength = 0;
  }

  /// Second method to add cells -- one at a time.
  template <vtkm::IdComponent ItemTupleLength>
  VTKM_CONT_EXPORT
  void AddCell(vtkm::UInt8 vtkmNotUsed(cellType),
               vtkm::IdComponent numVertices,
               const vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    for (vtkm::IdComponent i=0; i < numVertices; ++i)
    {
      this->PointToCell.Connectivity.GetPortalControl().Set(
            this->ConnectivityLength+i,ids[i]);
    }
    this->NumberOfCells++;
    this->ConnectivityLength += numVertices;
  }

  /// Third and final method to add cells -- one at a time.
  VTKM_CONT_EXPORT
  void CompleteAddingCells()
  {
    this->PointToCell.Connectivity.Shrink(this->ConnectivityLength);
    this->PointToCell.ElementsValid = true;
    this->PointToCell.IndexOffsetsValid = true;
    this->NumberOfCells = this->ConnectivityLength = -1;
  }

  //This is the way you can fill the memory from another system without copying
  VTKM_CONT_EXPORT
  void Fill(const vtkm::cont::ArrayHandle<vtkm::Id> &connectivity)
  {
    vtkm::IdComponent numberOfPointsPerCell = this->DetermineNumberOfPoints();
    const vtkm::Id length = connectivity.GetNumberOfValues() / numberOfPointsPerCell;
    const vtkm::UInt8 shapeTypeValue = static_cast<vtkm::UInt8>(this->CellTypeAsId);
    this->PointToCell.Shapes =
              vtkm::cont::make_ArrayHandleConstant(shapeTypeValue, length);
    this->PointToCell.NumIndices =
              vtkm::cont::make_ArrayHandleConstant(numberOfPointsPerCell,
                                                   length);
    this->PointToCell.IndexOffsets =
              vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0),
                                                   static_cast<vtkm::Id>(numberOfPointsPerCell),
                                                   length );
    this->PointToCell.Connectivity = connectivity;

    this->PointToCell.ElementsValid = true;
    this->PointToCell.IndexOffsetsValid = true;
  }

  /// Second method to add cells -- all at once.
  /// Copies the data from the vectors, so they can be released.
  VTKM_CONT_EXPORT
  void FillViaCopy(const std::vector<vtkm::Id> &connectivity)
  {
    vtkm::IdComponent numberOfPointsPerCell = this->DetermineNumberOfPoints();
    const vtkm::Id connectSize = static_cast<vtkm::Id>(connectivity.size());

    const vtkm::Id length = connectSize / numberOfPointsPerCell;
    const vtkm::UInt8 shapeTypeValue = static_cast<vtkm::UInt8>(this->CellTypeAsId);
    this->PointToCell.Shapes =
              vtkm::cont::make_ArrayHandleConstant(shapeTypeValue, length);
    this->PointToCell.NumIndices =
              vtkm::cont::make_ArrayHandleConstant(numberOfPointsPerCell,
                                                   length);
    this->PointToCell.IndexOffsets =
              vtkm::cont::make_ArrayHandleCounting(vtkm::Id(0),
                                                   static_cast<vtkm::Id>(numberOfPointsPerCell),
                                                   length );


    this->PointToCell.Connectivity.Allocate( connectSize  );
    std::copy(connectivity.begin(), connectivity.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(
                this->PointToCell.Connectivity.GetPortalControl()));

    this->PointToCell.ElementsValid = true;
    this->PointToCell.IndexOffsetsValid = true;
  }

private:
  template< typename CellShapeTag>
  void DetermineNumberOfPoints(CellShapeTag,
                               vtkm::CellTraitsTagSizeFixed,
                               vtkm::IdComponent& numberOfPoints) const
  {
    numberOfPoints = vtkm::CellTraits<CellShapeTag>::NUM_POINTS;
  }

  template< typename CellShapeTag>
  void DetermineNumberOfPoints(CellShapeTag,
                               vtkm::CellTraitsTagSizeVariable,
                               vtkm::IdComponent& numberOfPoints) const
  { //variable length cells can't be used with this class
    numberOfPoints = -1;
  }


  vtkm::IdComponent DetermineNumberOfPoints() const
  {
    vtkm::IdComponent numberOfPointsPerCell = -1;
    switch (this->CellTypeAsId)
    {
      vtkmGenericCellShapeMacro( this->DetermineNumberOfPoints(CellShapeTag(),
                                                               vtkm::CellTraits<CellShapeTag>::IsSizeFixed(),
                                                               numberOfPointsPerCell) );
      default:
        throw vtkm::cont::ErrorControlBadValue(
          "CellSetSingleType unable to determine the cell type");
    }
    return numberOfPointsPerCell;
  }

  vtkm::Id CellTypeAsId;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetSingleType_h
