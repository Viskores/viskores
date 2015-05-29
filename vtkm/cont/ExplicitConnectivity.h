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
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

namespace vtkm {
namespace cont {

class ExplicitConnectivity
{
public:
typedef vtkm::exec::ExplicitConnectivity<VTKM_DEFAULT_DEVICE_ADAPTER_TAG> ExecObjectType;

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
  VTKM_CONT_EXPORT
  void FillViaCopy(const std::vector<vtkm::Id> &cellTypes,
                   const std::vector<vtkm::Id> &numIndices,
                   const std::vector<vtkm::Id> &connectivity)
  {
    vtkm::cont::ArrayHandle<vtkm::Id> t1 = vtkm::cont::make_ArrayHandle(cellTypes);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(t1, Shapes);
    vtkm::cont::ArrayHandle<vtkm::Id> t2 = vtkm::cont::make_ArrayHandle(numIndices);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(t2, NumIndices);
    vtkm::cont::ArrayHandle<vtkm::Id> t3 = vtkm::cont::make_ArrayHandle(connectivity);
    vtkm::cont::DeviceAdapterAlgorithm<VTKM_DEFAULT_DEVICE_ADAPTER_TAG>::Copy(t3, Connectivity);

    NumShapes = cellTypes.size();
    ConnectivityLength = connectivity.size();

    // allocate and build reverse index
    MapCellToConnectivityIndex.Allocate(NumShapes);
    vtkm::Id counter = 0;
    for (vtkm::Id i=0; i<NumShapes; ++i)
    {
      MapCellToConnectivityIndex.GetPortalControl().Set(i, counter);
      counter += NumIndices.GetPortalControl().Get(i);
    }
  }

  template<typename DeviceTag>
  VTKM_CONT_EXPORT
  vtkm::exec::ExplicitConnectivity< DeviceTag> PrepareForInput(DeviceTag dt) const
  {
    vtkm::exec::ExplicitConnectivity< DeviceTag> obj;
    obj.Shapes = Shapes.PrepareForInput(dt);
    obj.NumIndices = NumIndices.PrepareForInput(dt);
    obj.Connectivity = Connectivity.PrepareForInput(dt);
    obj.MapCellToConnectivityIndex = MapCellToConnectivityIndex.PrepareForInput(dt);
    return obj;
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
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> Shapes;
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> NumIndices;
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> Connectivity;
  vtkm::cont::ArrayHandle<vtkm::Id, vtkm::cont::StorageTagBasic> MapCellToConnectivityIndex;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_ExplicitConnectivity_h
