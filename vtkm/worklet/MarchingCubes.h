//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_worklet_MarchingCubes_h
#define vtk_m_worklet_MarchingCubes_h

#include <vtkm/VectorAnalysis.h>

#include <vtkm/exec/CellDerivative.h>
#include <vtkm/exec/ParametricCoordinates.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleGroupVec.h>
#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/DataSet.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/DynamicArrayHandle.h>
#include <vtkm/cont/Field.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterCounting.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/worklet/MarchingCubesDataTables.h>

namespace vtkm {
namespace worklet {

namespace marchingcubes {


template<typename T> struct float_type { using type = vtkm::FloatDefault; };
template<>  struct float_type< vtkm::Float32 > { using type = vtkm::Float32; };
template<>  struct float_type< vtkm::Float64 > { using type = vtkm::Float64; };

// -----------------------------------------------------------------------------
template<typename S>
vtkm::cont::ArrayHandle<vtkm::Float32,S> make_ScalarField(const vtkm::cont::ArrayHandle<vtkm::Float32,S>& ah)
{ return ah; }

template<typename S>
vtkm::cont::ArrayHandle<vtkm::Float64,S> make_ScalarField(const vtkm::cont::ArrayHandle<vtkm::Float64,S>& ah)
{ return ah; }

template<typename S>
vtkm::cont::ArrayHandleCast<vtkm::FloatDefault, vtkm::cont::ArrayHandle<vtkm::UInt8,S> >
make_ScalarField(const vtkm::cont::ArrayHandle<vtkm::UInt8,S>& ah)
{ return vtkm::cont::make_ArrayHandleCast(ah, vtkm::FloatDefault()); }

template<typename S>
vtkm::cont::ArrayHandleCast<vtkm::FloatDefault, vtkm::cont::ArrayHandle<vtkm::Int8,S> >
make_ScalarField(const vtkm::cont::ArrayHandle<vtkm::Int8,S>& ah)
{ return vtkm::cont::make_ArrayHandleCast(ah, vtkm::FloatDefault()); }

// ---------------------------------------------------------------------------
template<typename T>
class ClassifyCell : public vtkm::worklet::WorkletMapPointToCell
{
public:
  struct ClassifyCellTagType : vtkm::ListTagBase<T> { };

  typedef void ControlSignature(
      WholeArrayIn< ClassifyCellTagType > isoValues,
      FieldInPoint< ClassifyCellTagType > fieldIn,
      CellSetIn cellset,
      FieldOutCell< IdComponentType > outNumTriangles,
      WholeArrayIn< IdComponentType > numTrianglesTable);
  typedef void ExecutionSignature(CellShape,_1, _2, _4, _5);
  typedef _3 InputDomain;


  template<typename IsoValuesType,
           typename FieldInType,
           typename NumTrianglesTablePortalType>
  VTKM_EXEC
  void operator()(vtkm::CellShapeTagGeneric shape,
                  const IsoValuesType& isovalues,
                  const FieldInType &fieldIn,
                  vtkm::IdComponent &numTriangles,
                  const NumTrianglesTablePortalType &numTrianglesTable) const
  {
    if(shape.Id == CELL_SHAPE_HEXAHEDRON )
    {
      this->operator()(vtkm::CellShapeTagHexahedron(),
                       isovalues,
                       fieldIn,
                       numTriangles,
                       numTrianglesTable);
    }
    else
    {
      numTriangles = 0;
    }
  }

  template<typename IsoValuesType,
           typename FieldInType,
           typename NumTrianglesTablePortalType>
  VTKM_EXEC
  void operator()(vtkm::CellShapeTagQuad vtkmNotUsed(shape),
                  const IsoValuesType &vtkmNotUsed(isovalues),
                  const FieldInType &vtkmNotUsed(fieldIn),
                  vtkm::IdComponent &vtkmNotUsed(numTriangles),
                  const NumTrianglesTablePortalType
                        &vtkmNotUsed(numTrianglesTable)) const
  {
  }

  template<typename IsoValuesType,
           typename FieldInType,
           typename NumTrianglesTablePortalType>
  VTKM_EXEC
  void operator()(vtkm::CellShapeTagHexahedron vtkmNotUsed(shape),
                  const IsoValuesType& isovalues,
                  const FieldInType &fieldIn,
                  vtkm::IdComponent &numTriangles,
                  const NumTrianglesTablePortalType &numTrianglesTable) const
  {
    vtkm::IdComponent sum = 0;
    for(vtkm::Id i=0; i < isovalues.GetNumberOfValues(); ++i)
    {
      const vtkm::IdComponent caseNumber = ((fieldIn[0] > isovalues[i])      |
                                            (fieldIn[1] > isovalues[i]) << 1 |
                                            (fieldIn[2] > isovalues[i]) << 2 |
                                            (fieldIn[3] > isovalues[i]) << 3 |
                                            (fieldIn[4] > isovalues[i]) << 4 |
                                            (fieldIn[5] > isovalues[i]) << 5 |
                                            (fieldIn[6] > isovalues[i]) << 6 |
                                            (fieldIn[7] > isovalues[i]) << 7);
      sum  += numTrianglesTable.Get(caseNumber);
    }
    numTriangles = sum;
  }
};


/// \brief Used to store data need for the EdgeWeightGenerate worklet.
/// This information is not passed as part of the arguments to the worklet as
/// that dramatically increase compile time by 200%
// -----------------------------------------------------------------------------
template< typename NormalType,
          typename NormalStorage,
          typename DeviceAdapter >
class EdgeWeightGenerateMetaData
{
  template<typename FieldType>
  struct PortalTypes
  {
    typedef vtkm::cont::ArrayHandle<FieldType> HandleType;
    typedef typename HandleType::template ExecutionTypes<DeviceAdapter> ExecutionTypes;

    typedef typename ExecutionTypes::Portal Portal;
    typedef typename ExecutionTypes::PortalConst PortalConst;
  };

  struct NormalPortalTypes
  {
    typedef vtkm::cont::ArrayHandle<vtkm::Vec< NormalType, 3>, NormalStorage> HandleType;
    typedef typename HandleType::template ExecutionTypes<DeviceAdapter> ExecutionTypes;

    typedef typename ExecutionTypes::Portal Portal;
  };

public:
  VTKM_CONT
  EdgeWeightGenerateMetaData(
                     vtkm::Id size,
                     vtkm::cont::ArrayHandle< vtkm::Vec<NormalType, 3>, NormalStorage >& normals,
                     vtkm::cont::ArrayHandle< vtkm::FloatDefault >& interpWeights,
                     vtkm::cont::ArrayHandle<vtkm::Id2>& interpIds,
                     vtkm::cont::ArrayHandle<vtkm::UInt8>& interpContourId,
                     const vtkm::cont::ArrayHandle< vtkm::IdComponent >& edgeTable,
                     const vtkm::cont::ArrayHandle< vtkm::IdComponent >& numTriTable,
                     const vtkm::cont::ArrayHandle< vtkm::IdComponent >& triTable,
                     const vtkm::worklet::ScatterCounting& scatter):
  NormalPortal( normals.PrepareForOutput( 3*size, DeviceAdapter() ) ),
  InterpWeightsPortal( interpWeights.PrepareForOutput( 3*size, DeviceAdapter()) ),
  InterpIdPortal( interpIds.PrepareForOutput( 3*size, DeviceAdapter() ) ),
  InterpContourPortal( interpContourId.PrepareForOutput( 3*size, DeviceAdapter() ) ),
  EdgeTable( edgeTable.PrepareForInput(DeviceAdapter()) ),
  NumTriTable( numTriTable.PrepareForInput(DeviceAdapter()) ),
  TriTable( triTable.PrepareForInput(DeviceAdapter()) ),
  Scatter(scatter)
  {
  //any way we can easily build an interface so that we don't need to hold
  //onto a billion portals?

  //Normal and Interp need to be 3 times longer than size as they
  //are per point of the output triangle
  }

  typename NormalPortalTypes::Portal NormalPortal;
  typename PortalTypes<vtkm::FloatDefault>::Portal InterpWeightsPortal;
  typename PortalTypes<vtkm::Id2>::Portal InterpIdPortal;
  typename PortalTypes<vtkm::UInt8>::Portal InterpContourPortal;
  typename PortalTypes<vtkm::IdComponent>::PortalConst EdgeTable;
  typename PortalTypes<vtkm::IdComponent>::PortalConst NumTriTable;
  typename PortalTypes<vtkm::IdComponent>::PortalConst TriTable;
  vtkm::worklet::ScatterCounting Scatter;
};

/// \brief Compute the weights for each edge that is used to generate
/// a point in the resulting iso-surface
// -----------------------------------------------------------------------------
template< typename T,
          typename NormalType,
          typename NormalStorage,
          typename DeviceAdapter >
class EdgeWeightGenerate : public vtkm::worklet::WorkletMapPointToCell
{
public:
  struct ClassifyCellTagType : vtkm::ListTagBase< typename float_type<T>::type > { };

  typedef vtkm::worklet::ScatterCounting ScatterType;

  typedef void ControlSignature(
      CellSetIn cellset, // Cell set
      WholeArrayIn< ClassifyCellTagType > isoValues,
      FieldInPoint< ClassifyCellTagType > fieldIn, // Input point field defining the contour
      FieldInPoint<Vec3> pcoordIn // Input point coordinates
      );
  typedef void ExecutionSignature(CellShape, _2, _3, _4, WorkIndex, VisitIndex, FromIndices);

  typedef _1 InputDomain;


  VTKM_CONT
  EdgeWeightGenerate(bool genNormals,
                     const EdgeWeightGenerateMetaData<NormalType, NormalStorage, DeviceAdapter>& meta) :
    GenerateNormals(genNormals),
    MetaData( meta )
  {
  }

  template<typename IsoValuesType,
           typename FieldInType, // Vec-like, one per input point
           typename CoordType,
           typename IndicesVecType>
  VTKM_EXEC
  void operator()(
      vtkm::CellShapeTagGeneric shape,
      const IsoValuesType &isovalues,
      const FieldInType & fieldIn, // Input point field defining the contour
      const CoordType & coords, // Input point coordinates
      vtkm::Id outputCellId,
      vtkm::IdComponent visitIndex,
      const IndicesVecType & indices) const
  { //covers when we have hexs coming from unstructured data
    if(shape.Id == CELL_SHAPE_HEXAHEDRON )
    {
      this->operator()(vtkm::CellShapeTagHexahedron(),
                       isovalues,
                       fieldIn,
                       coords,
                       outputCellId,
                       visitIndex,
                       indices);
    }
  }

  template<typename IsoValuesType,
           typename FieldInType, // Vec-like, one per input point
           typename CoordType,
           typename IndicesVecType>
  VTKM_EXEC
  void operator()(
      CellShapeTagQuad vtkmNotUsed(shape),
      const IsoValuesType &vtkmNotUsed(isovalues),
      const FieldInType & vtkmNotUsed(fieldIn), // Input point field defining the contour
      const CoordType & vtkmNotUsed(coords), // Input point coordinates
      vtkm::Id vtkmNotUsed(outputCellId),
      vtkm::IdComponent vtkmNotUsed(visitIndex),
      const IndicesVecType & vtkmNotUsed(indices) ) const
  { //covers when we have quads coming from 2d structured data
  }

  template<typename IsoValuesType,
           typename FieldInType, // Vec-like, one per input point
           typename CoordType,
           typename IndicesVecType>
  VTKM_EXEC
  void operator()(
      vtkm::CellShapeTagHexahedron shape,
      const IsoValuesType &isovalues,
      const FieldInType &fieldIn, // Input point field defining the contour
      const CoordType &coords, // Input point coordinates
      vtkm::Id outputCellId,
      vtkm::IdComponent visitIndex,
      const IndicesVecType &indices) const
  { //covers when we have hexs coming from 3d structured data
    const vtkm::Id outputPointId = 3 * outputCellId;
    typedef typename vtkm::VecTraits<FieldInType>::ComponentType FieldType;

    vtkm::IdComponent sum = 0, caseNumber = 0;
    vtkm::Id i=0;
    for(i=0; i < isovalues.GetNumberOfValues(); ++i)
    {
      // Compute the Marching Cubes case number for this cell. We need to iterate
      // the isovalues until the sum >= our visit index. But we need to make
      // sure the caseNumber is correct before stoping
      caseNumber = ((fieldIn[0] > isovalues[i])      |
                    (fieldIn[1] > isovalues[i]) << 1 |
                    (fieldIn[2] > isovalues[i]) << 2 |
                    (fieldIn[3] > isovalues[i]) << 3 |
                    (fieldIn[4] > isovalues[i]) << 4 |
                    (fieldIn[5] > isovalues[i]) << 5 |
                    (fieldIn[6] > isovalues[i]) << 6 |
                    (fieldIn[7] > isovalues[i]) << 7);
      sum  += MetaData.NumTriTable.Get(caseNumber);
      if(sum > visitIndex)
      {
        break;
      }
    }


    visitIndex = sum - visitIndex - 1;

    // Interpolate for vertex positions and associated scalar values
    const vtkm::Id triTableOffset =
        static_cast<vtkm::Id>(caseNumber*16 + visitIndex*3);
    for (vtkm::IdComponent triVertex = 0; triVertex < 3; triVertex++)
    {
      const vtkm::IdComponent edgeIndex =
          MetaData.TriTable.Get(triTableOffset + triVertex);
      const vtkm::IdComponent edgeVertex0 = MetaData.EdgeTable.Get(2*edgeIndex + 0);
      const vtkm::IdComponent edgeVertex1 = MetaData.EdgeTable.Get(2*edgeIndex + 1);
      const FieldType fieldValue0 = fieldIn[edgeVertex0];
      const FieldType fieldValue1 = fieldIn[edgeVertex1];

      MetaData.InterpContourPortal.Set(outputPointId+triVertex, static_cast<vtkm::UInt8>(i) );
      MetaData.InterpIdPortal.Set(
            outputPointId+triVertex,
            vtkm::Id2(indices[edgeVertex0],
                      indices[edgeVertex1]));

      vtkm::FloatDefault interpolant =
          static_cast<vtkm::FloatDefault>(isovalues[i] - fieldValue0) /
          static_cast<vtkm::FloatDefault>(fieldValue1 - fieldValue0);

      //need to factor in outputCellId
      MetaData.InterpWeightsPortal.Set(outputPointId+triVertex, interpolant);

      if(this->GenerateNormals)
      {
      const vtkm::Vec<vtkm::FloatDefault,3> edgePCoord0 =
          vtkm::exec::ParametricCoordinatesPoint(
            fieldIn.GetNumberOfComponents(), edgeVertex0, shape, *this);
      const vtkm::Vec<vtkm::FloatDefault,3> edgePCoord1 =
          vtkm::exec::ParametricCoordinatesPoint(
            fieldIn.GetNumberOfComponents(), edgeVertex1, shape, *this);

        const vtkm::Vec<vtkm::FloatDefault,3> interpPCoord =
            vtkm::Lerp(edgePCoord0, edgePCoord1, interpolant);

        //need to factor in outputCellId
        MetaData.NormalPortal.Set(outputPointId+triVertex,
          vtkm::Normal(vtkm::exec::CellDerivative(
                         fieldIn, coords, interpPCoord, shape, *this))
          );
      }

    }
  }

  VTKM_CONT
  ScatterType GetScatter() const
  {
    return this->MetaData.Scatter;
  }

private:
  const bool GenerateNormals;
  EdgeWeightGenerateMetaData<NormalType, NormalStorage, DeviceAdapter> MetaData;

  void operator=(const EdgeWeightGenerate<T,NormalType,NormalStorage,DeviceAdapter> &) = delete;
};


// ---------------------------------------------------------------------------
class ApplyToField : public vtkm::worklet::WorkletMapField
{
public:
  typedef void ControlSignature(FieldIn< Id2Type > interpolation_ids,
                                FieldIn< Scalar > interpolation_weights,
                                WholeArrayIn<> inputField,
                                FieldOut<> output
                                );
  typedef void ExecutionSignature(_1, _2, _3, _4);
  typedef _1 InputDomain;

  VTKM_CONT
  ApplyToField() {}

  template <typename WeightType, typename InFieldPortalType, typename OutFieldType>
  VTKM_EXEC
  void operator()(const vtkm::Id2& low_high,
                  const WeightType &weight,
                  const InFieldPortalType& inPortal,
                  OutFieldType &result) const
  {
    //fetch the low / high values from inPortal
    result = vtkm::Lerp(inPortal.Get(low_high[0]),
                        inPortal.Get(low_high[1]),
                        weight);
  }
};

// ---------------------------------------------------------------------------
struct FirstValueSame
{
  template<typename T, typename U>
  VTKM_EXEC_CONT bool operator()(const vtkm::Pair<T,U>& a,
                                        const vtkm::Pair<T,U>& b) const
  {
    return (a.first == b.first);
  }
};

}

/// \brief Compute the isosurface for a uniform grid data set
class MarchingCubes
{
public:

//----------------------------------------------------------------------------
MarchingCubes(bool mergeDuplicates=true):
  MergeDuplicatePoints(mergeDuplicates),
  EdgeTable(),
  NumTrianglesTable(),
  TriangleTable(),
  InterpolationWeights(),
  InterpolationIds()
{
  // Set up the Marching Cubes case tables as part of the filter so that
  // we cache these tables in the execution environment between execution runs
  this->EdgeTable =
    vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::edgeTable, 24);

  this->NumTrianglesTable =
    vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::numTrianglesTable, 256);

  this->TriangleTable =
    vtkm::cont::make_ArrayHandle(vtkm::worklet::internal::triTable, 256*16);
}

//----------------------------------------------------------------------------
void SetMergeDuplicatePoints(bool merge)
{
  this->MergeDuplicatePoints = merge;
}

//----------------------------------------------------------------------------
bool GetMergeDuplicatePoints( ) const
{
  return this->MergeDuplicatePoints;
}

//----------------------------------------------------------------------------
template<typename ValueType,
         typename CellSetType,
         typename CoordinateSystem,
         typename StorageTagField,
         typename CoordinateType,
         typename StorageTagVertices,
         typename DeviceAdapter>
vtkm::cont::CellSetSingleType< >
     Run(const ValueType* const isovalues,
         const vtkm::Id numIsoValues,
         const CellSetType& cells,
         const CoordinateSystem& coordinateSystem,
         const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
         vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3>, StorageTagVertices > vertices,
         const DeviceAdapter& device)
{
  vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3> > normals;
  return this->DoRun(isovalues,numIsoValues,cells,coordinateSystem,input,vertices,normals,false,device);
}

//----------------------------------------------------------------------------
template<typename ValueType,
         typename CellSetType,
         typename CoordinateSystem,
         typename StorageTagField,
         typename CoordinateType,
         typename StorageTagVertices,
         typename StorageTagNormals,
         typename DeviceAdapter>
vtkm::cont::CellSetSingleType< >
     Run(const ValueType* const isovalues,
         const vtkm::Id numIsoValues,
         const CellSetType& cells,
         const CoordinateSystem& coordinateSystem,
         const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
         vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3>, StorageTagVertices > vertices,
         vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3>, StorageTagNormals > normals,
         const DeviceAdapter& device)
{
  return this->DoRun(isovalues,numIsoValues,cells,coordinateSystem,input,vertices,normals,true,device);
}

//----------------------------------------------------------------------------
template<typename ArrayHandleIn,
         typename ArrayHandleOut,
         typename DeviceAdapter>
void MapFieldOntoIsosurface(const ArrayHandleIn& input,
                            ArrayHandleOut& output,
                            const DeviceAdapter&)
{
  using vtkm::worklet::marchingcubes::ApplyToField;
  ApplyToField applyToField;
  vtkm::worklet::DispatcherMapField<ApplyToField,
                                    DeviceAdapter> applyFieldDispatcher(applyToField);


  //todo: need to use the policy to get the correct storage tag for output
  applyFieldDispatcher.Invoke(this->InterpolationIds,
                              this->InterpolationWeights,
                              input,
                              output);
}

private:

//----------------------------------------------------------------------------
template<typename ValueType,
         typename CellSetType,
         typename CoordinateSystem,
         typename StorageTagField,
         typename StorageTagVertices,
         typename StorageTagNormals,
         typename CoordinateType,
         typename NormalType,
         typename DeviceAdapter>
vtkm::cont::CellSetSingleType< >
     DoRun(const ValueType* isovalues,
           const vtkm::Id numIsoValues,
           const CellSetType& cells,
           const CoordinateSystem& coordinateSystem,
           const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& inputField,
           vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3>, StorageTagVertices > vertices,
           vtkm::cont::ArrayHandle< vtkm::Vec<NormalType,3>, StorageTagNormals > normals,
           bool withNormals,
           const DeviceAdapter& )
{
  using vtkm::worklet::marchingcubes::ApplyToField;
  using vtkm::worklet::marchingcubes::EdgeWeightGenerate;
  using vtkm::worklet::marchingcubes::EdgeWeightGenerateMetaData;
  using vtkm::worklet::marchingcubes::ClassifyCell;

  // Setup the Dispatcher Typedefs
  typedef typename vtkm::worklet::DispatcherMapTopology<
                                      ClassifyCell<ValueType>,
                                      DeviceAdapter
                                      >             ClassifyDispatcher;

  typedef typename vtkm::worklet::DispatcherMapTopology<
                                      EdgeWeightGenerate<ValueType,
                                                         NormalType,
                                                         StorageTagNormals,
                                                         DeviceAdapter
                                                        >,
                                      DeviceAdapter
                                      >             GenerateDispatcher;

  vtkm::cont::ArrayHandle<ValueType> isoValuesHandle =
      vtkm::cont::make_ArrayHandle(isovalues, numIsoValues);
  // Call the ClassifyCell functor to compute the Marching Cubes case numbers
  // for each cell, and the number of vertices to be generated
  ClassifyCell<ValueType> classifyCell;
  ClassifyDispatcher classifyCellDispatcher(classifyCell);

  vtkm::cont::ArrayHandle<vtkm::IdComponent> numOutputTrisPerCell;
  classifyCellDispatcher.Invoke(isoValuesHandle,
                                inputField,
                                cells,
                                numOutputTrisPerCell,
                                this->NumTrianglesTable);


  //Pass 2 Generate the edges
  vtkm::worklet::ScatterCounting scatter(numOutputTrisPerCell, DeviceAdapter());

  vtkm::cont::ArrayHandle<vtkm::UInt8> contourIds;
  EdgeWeightGenerateMetaData< NormalType,
                              StorageTagNormals,
                              DeviceAdapter
                            > metaData( scatter.GetOutputRange(numOutputTrisPerCell.GetNumberOfValues()),
                                        normals,
                                        this->InterpolationWeights,
                                        this->InterpolationIds,
                                        contourIds,
                                        this->EdgeTable,
                                        this->NumTrianglesTable,
                                        this->TriangleTable,
                                        scatter
                                      );

  EdgeWeightGenerate<ValueType,
                     CoordinateType,
                     StorageTagNormals,
                     DeviceAdapter
                     > weightGenerate( withNormals,
                                       metaData);

  GenerateDispatcher edgeDispatcher(weightGenerate);
  edgeDispatcher.Invoke( cells,
                         //cast to a scalar field if not one, as cellderivative only works on those
                         marchingcubes::make_ScalarField(isoValuesHandle),
                         marchingcubes::make_ScalarField(inputField),
                         coordinateSystem
                         );

  if(numIsoValues <= 1)
  { //release memory early that we are not going to need again
    contourIds.ReleaseResources();
  }

  //Now that we have the edge interpolation finished we can generate the
  //following:
  //1. Coordinates ( with option to do point merging )
  //
  //
  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> Algorithm;

  vtkm::cont::DataSet output;
  vtkm::cont::ArrayHandle< vtkm::Id > connectivity;

  typedef vtkm::cont::ArrayHandle< vtkm::Id2 > Id2HandleType;
  typedef vtkm::cont::ArrayHandle< vtkm::UInt8 > ContourIdHandleType;
  typedef vtkm::cont::ArrayHandle<vtkm::FloatDefault> WeightHandleType;
  if(this->MergeDuplicatePoints)
  {
    //Do merge duplicate points we need to do the following:
    //1. Copy the interpolation Ids
    Id2HandleType uniqueIds;
    Algorithm::Copy(this->InterpolationIds, uniqueIds);

    if(withNormals)
    {
      typedef vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3>, StorageTagNormals > NormalHandlType;
      typedef vtkm::cont::ArrayHandleZip<WeightHandleType, NormalHandlType> KeyType;
      KeyType keys = vtkm::cont::make_ArrayHandleZip(this->InterpolationWeights, normals);

      //2. now we need to do a sort by key, making duplicate ids be adjacent
      if(numIsoValues > 1)
      {
        vtkm::cont::ArrayHandleZip<
          Id2HandleType, ContourIdHandleType> uniqueIdsWithContourId =
                              vtkm::cont::make_ArrayHandleZip(uniqueIds, contourIds);
        Algorithm::SortByKey(uniqueIdsWithContourId, keys);
      }
      else
      {
        Algorithm::SortByKey(uniqueIds, keys);
      }

      //3. lastly we need to do a unique by key, but since vtkm doesn't
      // offer that feature, we use a zip handle.
      // We need to use a custom comparison operator as we only want to compare
      // the id2 which is the first entry in the zip pair
      vtkm::cont::ArrayHandleZip<Id2HandleType, KeyType> zipped =
                  vtkm::cont::make_ArrayHandleZip(uniqueIds,keys);
      Algorithm::Unique( zipped, marchingcubes::FirstValueSame());
    }
    else
    {
      //2. now we need to do a sort by key, making duplicate ids be adjacent
      if(numIsoValues > 1)
      {
        vtkm::cont::ArrayHandleZip<
          Id2HandleType, ContourIdHandleType> uniqueIdsWithContourId =
                              vtkm::cont::make_ArrayHandleZip(uniqueIds, contourIds);
        Algorithm::SortByKey(uniqueIdsWithContourId, this->InterpolationWeights);
      }
      else
      {
        Algorithm::SortByKey(uniqueIds, this->InterpolationWeights);
      }


      //3. lastly we need to do a unique by key, but since vtkm doesn't
      // offer that feature, we use a zip handle.
      // We need to use a custom comparison operator as we only want to compare
      // the id2 which is the first entry in the zip pair
      vtkm::cont::ArrayHandleZip<Id2HandleType, WeightHandleType> zipped =
                  vtkm::cont::make_ArrayHandleZip(uniqueIds, this->InterpolationWeights);
      Algorithm::Unique( zipped, marchingcubes::FirstValueSame());
    }

    //4.
    //LowerBounds generates the output cell connections. It does this by
    //finding for each interpolationId where it would be inserted in the
    //sorted & unique subset, which generates an index value aka the lookup
    //value.
    //
    Algorithm::LowerBounds(uniqueIds, this->InterpolationIds, connectivity);

    //5.
    //We re-assign the shortened version of unique ids back into the
    //member variable so that 'DoMapField' will work properly
    this->InterpolationIds = uniqueIds;
  }
  else
  {
    //when we don't merge points, the connectivity array can be represented
    //by a counting array. The danger of doing it this way is that the output
    //type is unknown. That is why we use a CellSetSingleType with explicit
    //storage;
    vtkm::cont::ArrayHandleIndex temp(this->InterpolationIds.GetNumberOfValues());
    Algorithm::Copy(temp, connectivity);
  }

  //generate the vertices's
  ApplyToField applyToField;
  vtkm::worklet::DispatcherMapField<ApplyToField,
                                    DeviceAdapter> applyFieldDispatcher(applyToField);

  applyFieldDispatcher.Invoke(this->InterpolationIds,
                              this->InterpolationWeights,
                              coordinateSystem,
                              vertices);

  //assign the connectivity to the cell set
  vtkm::cont::CellSetSingleType< > outputCells("contour");
  outputCells.Fill( vertices.GetNumberOfValues(),
                    vtkm::CELL_SHAPE_TRIANGLE,
                    3,
                    connectivity );

  return outputCells;
}

  bool MergeDuplicatePoints;

  vtkm::cont::ArrayHandle<vtkm::IdComponent> EdgeTable;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> NumTrianglesTable;
  vtkm::cont::ArrayHandle<vtkm::IdComponent> TriangleTable;

  vtkm::cont::ArrayHandle<vtkm::FloatDefault> InterpolationWeights;
  vtkm::cont::ArrayHandle<vtkm::Id2> InterpolationIds;
};

}
} // namespace vtkm::worklet

#endif // vtk_m_worklet_MarchingCubes_h
