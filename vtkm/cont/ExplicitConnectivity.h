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
#ifndef vtk_m_cont_ExplicitConnectivity_h
#define vtk_m_cont_ExplicitConnectivity_h

#include <vtkm/CellType.h>
#include <vtkm/exec/ExplicitConnectivity.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/internal/ArrayPortalFromIterators.h>

namespace vtkm {
namespace cont {

template<typename ShapeStorageTag         = VTKM_DEFAULT_STORAGE_TAG,
         typename IndiceStorageTag        = VTKM_DEFAULT_STORAGE_TAG,
         typename ConnectivityStorageTag  = VTKM_DEFAULT_STORAGE_TAG >
class ExplicitConnectivity
{
public:

  VTKM_CONT_EXPORT
  ExplicitConnectivity()
  {
    NumShapes = 0;
    ConnectivityLength = 0;
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfElements()
  {
    return Shapes.GetNumberOfValues();
  }
  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfIndices(vtkm::Id index)
  {
    return NumIndices.GetPortalControl().Get(index);
  }
  VTKM_CONT_EXPORT
  vtkm::Id GetElementShapeType(vtkm::Id index)
  {
    return Shapes.GetPortalControl().Get(index);
  }
  template <vtkm::IdComponent ItemTupleLength>
  VTKM_CONT_EXPORT
  void GetIndices(vtkm::Id index, vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    int n = GetNumberOfIndices(index);
    int start = MapCellToConnectivityIndex.GetPortalControl().Get(index);
    for (int i=0; i<n && i<ItemTupleLength; i++)
      ids[i] = Connectivity.GetPortalControl().Get(start+i);
  }

  /// First method to add cells -- one at a time.
  VTKM_CONT_EXPORT
  void PrepareToAddCells(vtkm::Id numShapes, vtkm::Id connectivityMaxLen)
  {
    Shapes.Allocate(numShapes);
    NumIndices.Allocate(numShapes);
    Connectivity.Allocate(connectivityMaxLen);
    MapCellToConnectivityIndex.Allocate(numShapes);
    NumShapes = 0;
    ConnectivityLength = 0;
  }

  template <vtkm::IdComponent ItemTupleLength>
  VTKM_CONT_EXPORT
  void AddCell(vtkm::CellType cellType, int numVertices,
                const vtkm::Vec<vtkm::Id,ItemTupleLength> &ids)
  {
    Shapes.GetPortalControl().Set(NumShapes, cellType);
    NumIndices.GetPortalControl().Set(NumShapes, numVertices);
    for (int i=0; i < numVertices; ++i)
      Connectivity.GetPortalControl().Set(ConnectivityLength+i,ids[i]);
    MapCellToConnectivityIndex.GetPortalControl().Set(NumShapes,
                                                      ConnectivityLength);
    NumShapes++;
    ConnectivityLength += numVertices;
  }

  VTKM_CONT_EXPORT
  void CompleteAddingCells()
  {
    Connectivity.Shrink(ConnectivityLength);
  }

  /// Second method to add cells -- all at once.
  /// Copies the data from the vectors, so they can be released.
  VTKM_CONT_EXPORT
  void FillViaCopy(const std::vector<vtkm::Id> &cellTypes,
                   const std::vector<vtkm::Id> &numIndices,
                   const std::vector<vtkm::Id> &connectivity)
  {

    this->Shapes.Allocate( static_cast<vtkm::Id>(cellTypes.size()) );
    std::copy(cellTypes.begin(), cellTypes.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(this->Shapes.GetPortalControl()));

    this->NumIndices.Allocate( static_cast<vtkm::Id>(numIndices.size()) );
    std::copy(numIndices.begin(), numIndices.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(this->NumIndices.GetPortalControl()));

    this->Connectivity.Allocate( static_cast<vtkm::Id>(connectivity.size()) );
    std::copy(connectivity.begin(), connectivity.end(),
              vtkm::cont::ArrayPortalToIteratorBegin(this->Connectivity.GetPortalControl()));

    this->NumShapes = this->Shapes.GetNumberOfValues();
    this->ConnectivityLength = this->Connectivity.GetNumberOfValues();

    // allocate and build reverse index
    MapCellToConnectivityIndex.Allocate(NumShapes);
    vtkm::Id counter = 0;
    for (vtkm::Id i=0; i<NumShapes; ++i)
    {
      MapCellToConnectivityIndex.GetPortalControl().Set(i, counter);
      counter += NumIndices.GetPortalControl().Get(i);
    }
  }

  /// Second method to add cells -- all at once.
  /// Assigns the array handles to the explicit connectivity. This is
  /// the way you can fill the memory from another system without copying
  void Fill(const vtkm::cont::ArrayHandle<vtkm::Id, ShapeStorageTag> &cellTypes,
            const vtkm::cont::ArrayHandle<vtkm::Id, IndiceStorageTag> &numIndices,
            const vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag> &connectivity)
  {
    this->Shapes = cellTypes;
    this->NumIndices = numIndices;
    this->Connectivity = connectivity;

    this->NumShapes = this->Shapes.GetNumberOfValues();
    this->ConnectivityLength = this->Connectivity.GetNumberOfValues();

    // allocate and build reverse index
    MapCellToConnectivityIndex.Allocate(this->NumShapes);
    vtkm::Id counter = 0;
    for (vtkm::Id i=0; i<this->NumShapes; ++i)
    {
      MapCellToConnectivityIndex.GetPortalControl().Set(i, counter);
      counter += this->NumIndices.GetPortalControl().Get(i);
    }
  }

  template <typename DeviceAdapterTag>
  struct ExecutionTypes
  {
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id, ShapeStorageTag> ShapeHandle;
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id, IndiceStorageTag> IndiceHandle;
    typedef typename vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag> ConnectivityHandle;
    typedef vtkm::cont::ArrayHandle<vtkm::Id> MapCellToConnectivityHandle;

    typedef typename ShapeHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst ShapePortalType;
    typedef typename IndiceHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst IndicePortalType;
    typedef typename ConnectivityHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst ConnectivityPortalType;
    typedef typename MapCellToConnectivityHandle::template ExecutionTypes<DeviceAdapterTag>::PortalConst MapConnectivityPortalType;

    typedef vtkm::exec::ExplicitConnectivity<ShapePortalType,
                                             IndicePortalType,
                                             ConnectivityPortalType,
                                             MapConnectivityPortalType
                                             > ExecObjectType;
  };

  template<typename DeviceAdapterTag>
  typename ExecutionTypes<DeviceAdapterTag>::ExecObjectType
  PrepareForInput(DeviceAdapterTag tag) const
  {
    typedef typename ExecutionTypes<DeviceAdapterTag>::ExecObjectType ExecObjType;
    return ExecObjType(this->Shapes.PrepareForInput(tag),
                       this->NumIndices.PrepareForInput(tag),
                       this->Connectivity.PrepareForInput(tag),
                       this->MapCellToConnectivityIndex.PrepareForInput(tag));
  }

  VTKM_CONT_EXPORT
  virtual void PrintSummary(std::ostream &out)
  {
      out<<"    ExplicitConnectivity: #shapes= "<<NumShapes<<" #connectivity= "<<ConnectivityLength<<"\n";
      out<<"     Shapes: ";
      printSummary_ArrayHandle(Shapes, out);
      out<<"\n";
      out<<"     NumIndices: ";
      printSummary_ArrayHandle(NumIndices, out);
      out<<"\n";
      out<<"     Connectivity: ";
      printSummary_ArrayHandle(Connectivity, out);
      out<<"\n";
  }


private:
  vtkm::Id ConnectivityLength;
  vtkm::Id NumShapes;
  vtkm::cont::ArrayHandle<vtkm::Id, ShapeStorageTag> Shapes;
  vtkm::cont::ArrayHandle<vtkm::Id, IndiceStorageTag> NumIndices;
  vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag> Connectivity;
  vtkm::cont::ArrayHandle<vtkm::Id> MapCellToConnectivityIndex;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ExplicitConnectivity_h
