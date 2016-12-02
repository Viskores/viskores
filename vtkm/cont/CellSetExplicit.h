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
#ifndef vtk_m_cont_CellSetExplicit_h
#define vtk_m_cont_CellSetExplicit_h

#include <vtkm/CellShape.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/internal/ConnectivityExplicitInternals.h>
#include <vtkm/exec/ConnectivityExplicit.h>
#include <vtkm/TopologyElementTag.h>

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/DispatcherMapField.h>
#include <vtkm/exec/ExecutionWholeArray.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleConstant.h>

#include <map>
#include <utility>

namespace vtkm {
namespace cont {

namespace detail {

template<typename CellSetType, typename FromTopology, typename ToTopology>
struct CellSetExplicitConnectivityChooser
{
  typedef vtkm::cont::internal::ConnectivityExplicitInternals<>
      ConnectivityType;
};

} // namespace detail


#ifndef VTKM_DEFAULT_SHAPE_STORAGE_TAG
#define VTKM_DEFAULT_SHAPE_STORAGE_TAG VTKM_DEFAULT_STORAGE_TAG
#endif

#ifndef VTKM_DEFAULT_NUM_INDICES_STORAGE_TAG
#define VTKM_DEFAULT_NUM_INDICES_STORAGE_TAG VTKM_DEFAULT_STORAGE_TAG
#endif

#ifndef VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG
#define VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG VTKM_DEFAULT_STORAGE_TAG
#endif

#ifndef VTKM_DEFAULT_OFFSETS_STORAGE_TAG
#define VTKM_DEFAULT_OFFSETS_STORAGE_TAG VTKM_DEFAULT_STORAGE_TAG
#endif

template<typename ShapeStorageTag         = VTKM_DEFAULT_SHAPE_STORAGE_TAG,
         typename NumIndicesStorageTag    = VTKM_DEFAULT_NUM_INDICES_STORAGE_TAG,
         typename ConnectivityStorageTag  = VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
         typename OffsetsStorageTag       = VTKM_DEFAULT_OFFSETS_STORAGE_TAG >
class CellSetExplicit : public CellSet
{
  typedef CellSetExplicit< ShapeStorageTag,
                           NumIndicesStorageTag,
                           ConnectivityStorageTag,
                           OffsetsStorageTag > Thisclass;

  template<typename FromTopology, typename ToTopology>
  struct ConnectivityChooser
  {
    typedef typename detail::CellSetExplicitConnectivityChooser<
        Thisclass,
        FromTopology,
        ToTopology>::ConnectivityType ConnectivityType;

    typedef typename ConnectivityType::ShapeArrayType ShapeArrayType;
    typedef typename ConnectivityType::NumIndicesArrayType NumIndicesArrayType;
    typedef typename ConnectivityType::ConnectivityArrayType ConnectivityArrayType;
    typedef typename ConnectivityType::IndexOffsetArrayType IndexOffsetArrayType;

  };


public:
  typedef vtkm::Id SchedulingRangeType;

  //point to cell is used when iterating cells and asking for point properties
  typedef ConnectivityChooser< vtkm::TopologyElementTagPoint,
                               vtkm::TopologyElementTagCell > PointToCellConnectivityType;

  typedef typename PointToCellConnectivityType::ShapeArrayType ShapeArrayType;
  typedef typename PointToCellConnectivityType::NumIndicesArrayType NumIndicesArrayType;
  typedef typename PointToCellConnectivityType::ConnectivityArrayType ConnectivityArrayType;
  typedef typename PointToCellConnectivityType::IndexOffsetArrayType IndexOffsetArrayType;

  VTKM_CONT
  CellSetExplicit(vtkm::Id numpoints = 0,
                  const std::string &name = std::string())
    : CellSet(name),
      ConnectivityLength(-1),
      NumberOfCells(-1),
      NumberOfPoints(numpoints)
  {
  }

  VTKM_CONT
  CellSetExplicit(vtkm::Id numpoints)
    : CellSet(std::string()),
      ConnectivityLength(-1),
      NumberOfCells(-1),
      NumberOfPoints(numpoints)
  {
  }

  VTKM_CONT
  CellSetExplicit(const Thisclass &src)
    : CellSet(src),
      PointToCell(src.PointToCell),
      CellToPoint(src.CellToPoint),
      ConnectivityLength(src.ConnectivityLength),
      NumberOfCells(src.NumberOfCells),
      NumberOfPoints(src.NumberOfPoints)
  {  }

  VTKM_CONT
  Thisclass &operator=(const Thisclass &src)
  {
    this->CellSet::operator=(src);
    this->PointToCell = src.PointToCell;
    this->CellToPoint = src.CellToPoint;
    this->ConnectivityLength = src.ConnectivityLength;
    this->NumberOfCells = src.NumberOfCells;
    this->NumberOfPoints = src.NumberOfPoints;
    return *this;
  }

  virtual ~CellSetExplicit() {  }

  virtual vtkm::Id GetNumberOfCells() const
  {
    return this->PointToCell.GetNumberOfElements();
  }

  virtual vtkm::Id GetNumberOfPoints() const
  {
    return this->NumberOfPoints;
  }

  VTKM_CONT
  vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagCell) const
  {
    return this->GetNumberOfCells();
  }

  VTKM_CONT
  vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagPoint) const
  {
    return this->GetNumberOfPoints();
  }

  VTKM_CONT
  vtkm::IdComponent GetNumberOfPointsInCell(vtkm::Id cellIndex) const
  {
    return this->PointToCell.NumIndices.GetPortalConstControl().Get(cellIndex);
  }

  VTKM_CONT
  vtkm::Id GetCellShape(vtkm::Id cellIndex) const
  {
    return this->PointToCell.Shapes.GetPortalConstControl().Get(cellIndex);
  }

  template <vtkm::IdComponent ItemTupleLength>
  VTKM_CONT
  void GetIndices(vtkm::Id index,
                  vtkm::Vec<vtkm::Id,ItemTupleLength> &ids) const
  {
    this->PointToCell.BuildIndexOffsets(VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    vtkm::IdComponent numIndices = this->GetNumberOfPointsInCell(index);
    vtkm::Id start =
        this->PointToCell.IndexOffsets.GetPortalConstControl().Get(index);
    for (vtkm::IdComponent i=0; i<numIndices && i<ItemTupleLength; i++)
      ids[i] = this->PointToCell.Connectivity.GetPortalConstControl().Get(start+i);
  }

  /// First method to add cells -- one at a time.
  VTKM_CONT
  void PrepareToAddCells(vtkm::Id numShapes, vtkm::Id connectivityMaxLen)
  {
    this->PointToCell.Shapes.Allocate(numShapes);
    this->PointToCell.NumIndices.Allocate(numShapes);
    this->PointToCell.Connectivity.Allocate(connectivityMaxLen);
    this->PointToCell.IndexOffsets.Allocate(numShapes);
    this->NumberOfCells = 0;
    this->ConnectivityLength = 0;
  }

  template <typename IndexableType>
  VTKM_CONT
  void AddCell(vtkm::UInt8 cellType,
               vtkm::IdComponent numVertices,
               const IndexableType &ids)
  {
    this->PointToCell.Shapes.GetPortalControl().Set(this->NumberOfCells, cellType);
    this->PointToCell.NumIndices.GetPortalControl().Set(this->NumberOfCells, numVertices);
    for (vtkm::IdComponent i=0; i < numVertices; ++i)
    {
      this->PointToCell.Connectivity.GetPortalControl().Set(
            this->ConnectivityLength+i,ids[i]);
    }
    this->PointToCell.IndexOffsets.GetPortalControl().Set(
          this->NumberOfCells, this->ConnectivityLength);
    this->NumberOfCells++;
    this->ConnectivityLength += numVertices;
  }

  VTKM_CONT
  void CompleteAddingCells()
  {
    this->PointToCell.Connectivity.Shrink(ConnectivityLength);
    this->PointToCell.ElementsValid = true;
    this->PointToCell.IndexOffsetsValid = true;
    this->NumberOfCells = this->ConnectivityLength = -1;
  }

  /// Second method to add cells -- all at once.
  /// Assigns the array handles to the explicit connectivity. This is
  /// the way you can fill the memory from another system without copying
  VTKM_CONT
  void Fill(const vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorageTag> &cellTypes,
            const vtkm::cont::ArrayHandle<vtkm::IdComponent, NumIndicesStorageTag> &numIndices,
            const vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag> &connectivity,
            const vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorageTag> &offsets
                  = vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorageTag>() )
  {
    this->PointToCell.Shapes = cellTypes;
    this->PointToCell.NumIndices = numIndices;
    this->PointToCell.Connectivity = connectivity;

    this->PointToCell.ElementsValid = true;

    if(offsets.GetNumberOfValues() == cellTypes.GetNumberOfValues())
    {
      this->PointToCell.IndexOffsets = offsets;
      this->PointToCell.IndexOffsetsValid = true;
    }
    else
    {
      this->PointToCell.IndexOffsetsValid = false;
      if (offsets.GetNumberOfValues() != 0)
      {
        throw vtkm::cont::ErrorControlBadValue(
             "Explicit cell offsets array unexpected size. "
             "Use an empty array to automatically generate.");
      }
    }
  }

  template <typename DeviceAdapter, typename FromTopology, typename ToTopology>
  struct ExecutionTypes
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapter);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);

    typedef  ConnectivityChooser<FromTopology,ToTopology> ConnectivityTypes;

    typedef typename ConnectivityTypes::ShapeArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst ShapePortalType;
    typedef typename ConnectivityTypes::NumIndicesArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst IndicePortalType;
    typedef typename ConnectivityTypes::ConnectivityArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst ConnectivityPortalType;
    typedef typename ConnectivityTypes::IndexOffsetArrayType::template ExecutionTypes<DeviceAdapter>::PortalConst IndexOffsetPortalType;

    typedef vtkm::exec::ConnectivityExplicit<ShapePortalType,
                                             IndicePortalType,
                                             ConnectivityPortalType,
                                             IndexOffsetPortalType
                                             > ExecObjectType;
  };

  template<typename Device, typename FromTopology, typename ToTopology>
  typename ExecutionTypes<Device,FromTopology,ToTopology>::ExecObjectType
  PrepareForInput(Device, FromTopology, ToTopology) const
  {
    this->BuildConnectivity(Device(), FromTopology(), ToTopology());

    const typename
        ConnectivityChooser<FromTopology,ToTopology>::ConnectivityType
        &connectivity = this->GetConnectivity(FromTopology(), ToTopology());

    VTKM_ASSERT(connectivity.ElementsValid);

    typedef typename
        ExecutionTypes<Device,FromTopology,ToTopology>::ExecObjectType
            ExecObjType;
    return ExecObjType(connectivity.Shapes.PrepareForInput(Device()),
                       connectivity.NumIndices.PrepareForInput(Device()),
                       connectivity.Connectivity.PrepareForInput(Device()),
                       connectivity.IndexOffsets.PrepareForInput(Device()));
  }

  template<typename Device, typename FromTopology, typename ToTopology>
  VTKM_CONT
  void BuildConnectivity(Device, FromTopology, ToTopology) const
  {
    typedef CellSetExplicit<ShapeStorageTag,
                            NumIndicesStorageTag,
                            ConnectivityStorageTag,
                            OffsetsStorageTag> CSE;

    CSE *self = const_cast<CSE*>(this);

    self->CreateConnectivity(Device(), FromTopology(), ToTopology());

    self->GetConnectivity(FromTopology(), ToTopology()).
      BuildIndexOffsets(Device());
  }

  template<typename Device>
  VTKM_CONT
  void CreateConnectivity(Device,
                          vtkm::TopologyElementTagPoint,
                          vtkm::TopologyElementTagCell)
  {
    // nothing to do
  }

  // Worklet to expand the PointToCell numIndices array by repeating cell index
  class ExpandIndices : public vtkm::worklet::WorkletMapField
  {
  public:
    typedef void ControlSignature(FieldIn<> cellIndex,
                                  FieldIn<> offset,
                                  FieldIn<> numIndices,
                                  WholeArrayOut<> cellIndices);
    typedef void ExecutionSignature(_1,_2,_3,_4);
    typedef _1 InputDomain;

    VTKM_CONT
    ExpandIndices() {}

    template<typename PortalType>
    VTKM_EXEC
    void operator()(const vtkm::Id &cellIndex,
                    const vtkm::Id &offset,
                    const vtkm::Id &numIndices,
                    const PortalType &cellIndices) const
    {
      VTKM_ASSERT(cellIndices.GetNumberOfValues() >= offset + numIndices);
      vtkm::Id startIndex = offset;
      for (vtkm::Id i = 0; i < numIndices; i++)
      {
        cellIndices.Set(startIndex++, cellIndex);
      }
    }
  };

  template<typename Device>
  VTKM_CONT
  void CreateConnectivity(Device,
                          vtkm::TopologyElementTagCell,
                          vtkm::TopologyElementTagPoint)
  {
    // PointToCell connectivity array (point indices) will be
    // transformed into the CellToPoint numIndices array using reduction
    //
    // PointToCell numIndices array using expansion will be
    // transformed into the CellToPoint connectivity array

    if (this->CellToPoint.ElementsValid)
    {
      return;
    }

    typedef vtkm::cont::DeviceAdapterAlgorithm<Device> Algorithm;

    // Sizes of the PointToCell information
    vtkm::Id numberOfCells = this->GetNumberOfCells();
    vtkm::Id connectivityLength = this->PointToCell.Connectivity.GetNumberOfValues();

    // PointToCell connectivity will be basis of CellToPoint numIndices
    vtkm::cont::ArrayHandle<vtkm::Id> pointIndices;
    Algorithm::Copy(this->PointToCell.Connectivity, pointIndices);

    // PointToCell numIndices will be basis of CellToPoint connectivity

    this->CellToPoint.Connectivity.Allocate(connectivityLength);
    vtkm::cont::ArrayHandleCounting<vtkm::Id> index(0, 1, numberOfCells);

    this->PointToCell.BuildIndexOffsets(Device());
    vtkm::worklet::DispatcherMapField<ExpandIndices, Device> expandDispatcher;
    expandDispatcher.Invoke(index,
                            this->PointToCell.IndexOffsets,
                            this->PointToCell.NumIndices,
                            this->CellToPoint.Connectivity);

    // SortByKey where key is PointToCell connectivity and value is the expanded cellIndex
    Algorithm::SortByKey(pointIndices, this->CellToPoint.Connectivity);

    if(this->GetNumberOfPoints() <= 0)
    {
      this->NumberOfPoints = pointIndices.GetPortalControl().Get(connectivityLength - 1) + 1;
    }
    vtkm::Id numberOfPoints = this->GetNumberOfPoints();

    // CellToPoint numIndices from the now sorted PointToCell connectivity
    vtkm::cont::ArrayHandleConstant<vtkm::Id> numArray(1, connectivityLength);
    vtkm::cont::ArrayHandle<vtkm::Id> uniquePoints;
    vtkm::cont::ArrayHandle<vtkm::Id> numIndices;
    uniquePoints.Allocate(numberOfPoints);
    numIndices.Allocate(numberOfPoints);

    Algorithm::ReduceByKey(pointIndices, numArray,
                           uniquePoints, numIndices,
                           vtkm::Add());

    // Set the CellToPoint information
    this->CellToPoint.Shapes = vtkm::cont::make_ArrayHandleConstant(
      static_cast<vtkm::UInt8>(CELL_SHAPE_VERTEX), numberOfPoints);
    Algorithm::Copy(numIndices, this->CellToPoint.NumIndices);

    this->CellToPoint.ElementsValid = true;
    this->CellToPoint.IndexOffsetsValid = false;
  }

  virtual void PrintSummary(std::ostream &out) const
  {
      out << "   ExplicitCellSet: " << this->Name << std::endl;
      out << "   PointToCell: " << std::endl;
      this->PointToCell.PrintSummary(out);
      out << "   CellToPoint: " << std::endl;
      this->CellToPoint.PrintSummary(out);
  }

  template<typename FromTopology, typename ToTopology>
  VTKM_CONT
  const typename ConnectivityChooser<FromTopology,ToTopology>::ShapeArrayType &
  GetShapesArray(FromTopology,ToTopology) const
  {
    return this->GetConnectivity(FromTopology(), ToTopology()).Shapes;
  }

  template<typename FromTopology, typename ToTopology>
  VTKM_CONT
  const typename ConnectivityChooser<FromTopology,ToTopology>::NumIndicesArrayType &
  GetNumIndicesArray(FromTopology,ToTopology) const
  {
    return this->GetConnectivity(FromTopology(), ToTopology()).NumIndices;
  }

  template<typename FromTopology, typename ToTopology>
  VTKM_CONT
  const typename ConnectivityChooser<FromTopology,ToTopology>::ConnectivityArrayType &
  GetConnectivityArray(FromTopology,ToTopology) const
  {
    return this->GetConnectivity(FromTopology(), ToTopology()).Connectivity;
  }

  template<typename FromTopology, typename ToTopology>
  VTKM_CONT
  const typename ConnectivityChooser<FromTopology,ToTopology>::IndexOffsetArrayType &
  GetIndexOffsetArray(FromTopology,ToTopology) const
  {
    return this->GetConnectivity(FromTopology(), ToTopology()).IndexOffsets;
  }

protected:
  typename ConnectivityChooser<
      vtkm::TopologyElementTagPoint,vtkm::TopologyElementTagCell>::
      ConnectivityType PointToCell;

  // TODO: Actually implement CellToPoint and other connectivity. (That is,
  // derive the connectivity from PointToCell.
  typename ConnectivityChooser<
      vtkm::TopologyElementTagCell,vtkm::TopologyElementTagPoint>::
      ConnectivityType CellToPoint;
private:

  // A set of overloaded methods to get the connectivity from a pair of
  // topology element types.
#define VTKM_GET_CONNECTIVITY_METHOD(FromTopology,ToTopology,Ivar) \
  VTKM_CONT \
  const typename ConnectivityChooser< \
      FromTopology,ToTopology>::ConnectivityType & \
  GetConnectivity(FromTopology, ToTopology) const \
  { \
    return this->Ivar; \
  } \
  VTKM_CONT \
  typename ConnectivityChooser< \
      FromTopology,ToTopology>::ConnectivityType & \
  GetConnectivity(FromTopology, ToTopology) \
  { \
    return this->Ivar; \
  }

  VTKM_GET_CONNECTIVITY_METHOD(vtkm::TopologyElementTagPoint,
                               vtkm::TopologyElementTagCell,
                               PointToCell)
  VTKM_GET_CONNECTIVITY_METHOD(vtkm::TopologyElementTagCell,
                               vtkm::TopologyElementTagPoint,
                               CellToPoint)

#undef VTKM_GET_CONNECTIVITY_METHOD

protected:
  // These are used in the AddCell and related methods to incrementally add
  // cells. They need to be protected as subclasses of CellSetExplicit
  // need to set these values when implementing Fill()
  vtkm::Id ConnectivityLength;
  vtkm::Id NumberOfCells;
  vtkm::Id NumberOfPoints;
};

namespace detail {

template<typename Storage1, typename Storage2, typename Storage3, typename Storage4>
struct CellSetExplicitConnectivityChooser<
    vtkm::cont::CellSetExplicit<Storage1,Storage2,Storage3,Storage4>,
    vtkm::TopologyElementTagPoint,
    vtkm::TopologyElementTagCell>
{
  typedef vtkm::cont::internal::ConnectivityExplicitInternals<
      Storage1,Storage2,Storage3,Storage4> ConnectivityType;
};


template<typename CellSetType>
struct CellSetExplicitConnectivityChooser<
    CellSetType,
    vtkm::TopologyElementTagCell,
    vtkm::TopologyElementTagPoint>
{
  //only specify the shape type as it will be constant as everything
  //is a vertex. otherwise use the defaults.
  typedef vtkm::cont::internal::ConnectivityExplicitInternals<
      typename ArrayHandleConstant<vtkm::UInt8>::StorageTag > ConnectivityType;
};

} // namespace detail

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetExplicit_h
