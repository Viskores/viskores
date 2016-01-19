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

#include <vtkm/cont/ArrayHandleIndex.h>
#include <vtkm/cont/ArrayHandleZip.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/CellSetSingleType.h>

#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/ScatterCounting.h>

namespace
{
typedef vtkm::filter::FilterTraits<vtkm::filter::MarchingCubes>::InputFieldTypeList InputTypes;

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

// -----------------------------------------------------------------------------
template<typename T, typename U>
VTKM_EXEC_EXPORT
int GetHexahedronClassification(const T& values, const U isoValue)
{
  return ((values[0] > isoValue)      |
          (values[1] > isoValue) << 1 |
          (values[2] > isoValue) << 2 |
          (values[3] > isoValue) << 3 |
          (values[4] > isoValue) << 4 |
          (values[5] > isoValue) << 5 |
          (values[6] > isoValue) << 6 |
          (values[7] > isoValue) << 7);
}

class ClassifyCell : public vtkm::worklet::WorkletMapPointToCell
{
public:
  typedef void ControlSignature(
      FieldInPoint<InputTypes> inNodes,
      TopologyIn topology,
      FieldOutCell< IdComponentType > outNumTriangles,
      WholeArrayIn< IdComponentType > numTrianglesTable);
  typedef void ExecutionSignature(_1, _3, _4);
  typedef _2 InputDomain;

  vtkm::Float64 Isovalue;

  VTKM_CONT_EXPORT
  ClassifyCell(vtkm::Float64 isovalue) :
    Isovalue(isovalue)
  {
  }

  template<typename FieldInType,
           typename NumTrianglesTablePortalType>
  VTKM_EXEC_EXPORT
  void operator()(const FieldInType &fieldIn,
                  vtkm::IdComponent &numTriangles,
                  const NumTrianglesTablePortalType &numTrianglesTable) const
  {
    typedef typename vtkm::VecTraits<FieldInType>::ComponentType FieldType;
    const FieldType iso = static_cast<FieldType>(this->Isovalue);

    const vtkm::IdComponent caseNumber =
                            GetHexahedronClassification(fieldIn, iso);
    numTriangles = numTrianglesTable.Get(caseNumber);
  }
};

/// \brief Compute the weights for each edge that is used to generate
/// a point in the resulting iso-surface
class EdgeWeightGenerate : public vtkm::worklet::WorkletMapPointToCell
{
  typedef vtkm::Vec< vtkm::Id2, 3 > Vec3Id2;
  typedef vtkm::Vec< vtkm::Vec<vtkm::Float32,3>, 3 > FVec3x3;
  typedef vtkm::Vec< vtkm::Vec<vtkm::Float64,3>, 3 > DVec3x3;

public:

  typedef vtkm::worklet::ScatterCounting ScatterType;

  struct InterpolateIdTypes : vtkm::ListTagBase< Vec3Id2 > { };
  struct Vec3FloatTypes : vtkm::ListTagBase< FVec3x3, DVec3x3> { };

  typedef void ControlSignature(
      TopologyIn topology, // Cell set
      FieldInPoint<Scalar> fieldIn, // Input point field defining the contour
      FieldInPoint<Vec3> pcoordIn, // Input point coordinates
      FieldOutCell<Vec3FloatTypes> normalsOut, // Estimated normals (one per tri vertex)
      FieldOutCell<Vec3> interpolationWeights,
      FieldOutCell<InterpolateIdTypes> interpolationIds,
      WholeArrayIn<IdComponentType> EdgeTable, // An array portal with the edge table
      WholeArrayIn<IdComponentType> TriTable // An array portal with the triangle table
      );
  typedef void ExecutionSignature(
      CellShape, _2, _3, _4, _5, _6, _7, _8, VisitIndex, FromIndices);

  typedef _1 InputDomain;

  VTKM_CONT_EXPORT
  EdgeWeightGenerate(vtkm::Float64 isovalue,
                     bool genNormals,
                     const vtkm::worklet::ScatterCounting scatter) :
    Isovalue(isovalue),
    GenerateNormals(genNormals),
    Scatter( scatter ) {  }

  template<typename CellShapeTag,
           typename FieldInType, // Vec-like, one per input point
           typename CoordType,
           typename NormalType,
           typename WeightType,
           typename IdType,
           typename EdgeTablePortalType, // Whole Array portal
           typename TriTablePortalType, // Whole Array portal
           typename IndicesVecType>
  VTKM_EXEC_EXPORT
  void operator()(
      CellShapeTag shape,
      const FieldInType &fieldIn, // Input point field defining the contour
      const CoordType &coords, // Input point coordinates
      NormalType &normalsOut, // Estimated normals (one per tri vertex)
      WeightType &interpolationWeights,
      IdType &interpolationIds,
      const EdgeTablePortalType &edgeTable,
      const TriTablePortalType &triTable,
      vtkm::IdComponent visitIndex,
      const IndicesVecType &indices) const
  {
    typedef typename vtkm::VecTraits<FieldInType>::ComponentType FieldType;
    const FieldType iso = static_cast<FieldType>(this->Isovalue);

    // Compute the Marching Cubes case number for this cell
    const vtkm::IdComponent caseNumber =
                            GetHexahedronClassification(fieldIn, iso);

    // Interpolate for vertex positions and associated scalar values
    const vtkm::Id triTableOffset =
        static_cast<vtkm::Id>(caseNumber*16 + visitIndex*3);
    for (vtkm::IdComponent triVertex = 0; triVertex < 3; triVertex++)
    {
      const vtkm::IdComponent edgeIndex =
          triTable.Get(triTableOffset + triVertex);
      const vtkm::IdComponent edgeVertex0 = edgeTable.Get(2*edgeIndex + 0);
      const vtkm::IdComponent edgeVertex1 = edgeTable.Get(2*edgeIndex + 1);
      const FieldType fieldValue0 = fieldIn[edgeVertex0];
      const FieldType fieldValue1 = fieldIn[edgeVertex1];

      interpolationIds[triVertex][0] = indices[edgeVertex0];
      interpolationIds[triVertex][1] = indices[edgeVertex1];

      //we need to cast each side of the division to WeightType::ComponentType
      //so that the interpolation works properly even when iso-contouring
      //char/uchar fields
      typedef typename vtkm::VecTraits<WeightType>::ComponentType WType;
      WType interpolant =
          static_cast<WType>(iso - fieldValue0) /
          static_cast<WType>(fieldValue1 - fieldValue0);
      interpolationWeights[triVertex] = interpolant;

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

        normalsOut[triVertex] =
          vtkm::Normal(vtkm::exec::CellDerivative(
                         fieldIn, coords, interpPCoord, shape, *this));
      }
    }
  }

  VTKM_CONT_EXPORT
  ScatterType GetScatter() const
  {
    return this->Scatter;
  }


private:
  const vtkm::Float64 Isovalue;
  const bool GenerateNormals;
  ScatterType Scatter;
};


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

    VTKM_CONT_EXPORT
    ApplyToField() {}

    template <typename WeightType, typename InFieldPortalType, typename OutFieldType>
    VTKM_EXEC_EXPORT
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

}

namespace vtkm {
namespace filter {

//-----------------------------------------------------------------------------
MarchingCubes::MarchingCubes():
  vtkm::filter::DataSetWithFieldFilter<MarchingCubes>(),
  IsoValue(0),
  MergeDuplicatePoints(true),
  GenerateNormals(false),
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

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
vtkm::filter::DataSetResult MarchingCubes::DoExecute(const vtkm::cont::DataSet& input,
                                                     const vtkm::cont::ArrayHandle<T, StorageType>& field,
                                                     const vtkm::filter::FieldMetadata& fieldMeta,
                                                     const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                                     const DeviceAdapter&)
{

  if(fieldMeta.IsPointField() == false)
  {
    //todo: we need to mark this as a failure of input, not a failure
    //of the algorithm
    return vtkm::filter::DataSetResult();
  }

  //get the cells and coordinates of the dataset
  const vtkm::cont::DynamicCellSet& cells =
                  input.GetCellSet(this->GetActiveCellSetIndex());

  const vtkm::cont::CoordinateSystem& coords =
                      input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex());

  // Setup the Dispatcher Typedefs
  typedef typename vtkm::worklet::DispatcherMapTopology<
                                      ClassifyCell,
                                      DeviceAdapter
                                      >             ClassifyDispatcher;

  typedef typename vtkm::worklet::DispatcherMapTopology<
                                      EdgeWeightGenerate,
                                      DeviceAdapter
                                      >             GenerateDispatcher;


  // Call the ClassifyCell functor to compute the Marching Cubes case numbers
  // for each cell, and the number of vertices to be generated
  ClassifyCell classifyCell( this->IsoValue );
  ClassifyDispatcher classifyCellDispatcher(classifyCell);

  vtkm::cont::ArrayHandle<vtkm::IdComponent> numOutputTrisPerCell;
  classifyCellDispatcher.Invoke(field,
                                vtkm::filter::Convert(cells, policy),
                                numOutputTrisPerCell,
                                this->NumTrianglesTable);


  //Pass 2 Generate the edges
  typedef vtkm::cont::ArrayHandle< vtkm::Vec< vtkm::Float32,3> > Vec3HandleType;
  Vec3HandleType normals;

  vtkm::worklet::ScatterCounting scatter(numOutputTrisPerCell, DeviceAdapter());
  EdgeWeightGenerate weightGenerate(this->IsoValue,
                                    this->GenerateNormals,
                                    scatter);

  GenerateDispatcher edgeDispatcher(weightGenerate);
  edgeDispatcher.Invoke(
        vtkm::filter::Convert(cells, policy),
        //cast to a scalar field if not one, as cellderivative only works on those
        make_ScalarField(field),
        vtkm::filter::Convert(coords, policy),
        vtkm::cont::make_ArrayHandleGroupVec<3>(normals),
        vtkm::cont::make_ArrayHandleGroupVec<3>(this->InterpolationWeights),
        vtkm::cont::make_ArrayHandleGroupVec<3>(this->InterpolationIds),
        this->EdgeTable,
        this->TriangleTable);

  //Now that we have the edge interpolation finished we can generate the
  //following:
  //1. Coordinates ( with option to do point merging )
  //2. Normals
  //todo: We need to run the coords through out policy and determine
  //what the output coordinate type should be. We have two problems here
  //1. What is the type? float32/float64
  //2. What is the storage backing
  vtkm::cont::DataSet output;
  vtkm::cont::ArrayHandle< vtkm::Vec< vtkm::Float32,3> > vertices;

  typedef vtkm::cont::ArrayHandle< vtkm::Id2 > Id2HandleType;
  typedef vtkm::cont::ArrayHandle<vtkm::FloatDefault> WeightHandleType;
  if(this->MergeDuplicatePoints)
  {
    typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> Algorithm;

    //Do merge duplicate points we need to do the following:
    //1. Copy the interpolation Ids
    Id2HandleType unqiueIds;
    Algorithm::Copy(this->InterpolationIds, unqiueIds);

    if(this->GenerateNormals)
      {
      typedef vtkm::cont::ArrayHandleZip<WeightHandleType, Vec3HandleType> KeyType;
      KeyType keys = vtkm::cont::make_ArrayHandleZip(this->InterpolationWeights, normals);

      //2. now we need to do a sort by key, giving us
      Algorithm::SortByKey(unqiueIds, keys);

      //3. lastly we need to do a unique by key, but since vtkm doesn't
      // offer that feature, we use a zip handle
      vtkm::cont::ArrayHandleZip<Id2HandleType, KeyType> zipped =
                  vtkm::cont::make_ArrayHandleZip(unqiueIds,keys);
      Algorithm::Unique( zipped );
      }
    else
      {
      //2. now we need to do a sort by key, giving us
      Algorithm::SortByKey(unqiueIds, this->InterpolationWeights);

      //3. lastly we need to do a unique by key, but since vtkm doesn't
      // offer that feature, we use a zip handle
      vtkm::cont::ArrayHandleZip<Id2HandleType, WeightHandleType> zipped =
                  vtkm::cont::make_ArrayHandleZip(unqiueIds, this->InterpolationWeights);
      Algorithm::Unique( zipped );
      }

    //4.
    //LowerBounds generates the output cell connections. It does this by
    //finding for each interpolationId where it would be inserted in the
    //sorted & unique subset, which generates an index value aka the lookup
    //value.
    //
    vtkm::cont::ArrayHandle< vtkm::Id> connectivity;
    Algorithm::LowerBounds(unqiueIds, this->InterpolationIds, connectivity);


    vtkm::cont::CellSetSingleType< > outputCells( (vtkm::CellShapeTagTriangle()) );
    outputCells.Fill( connectivity );
    output.AddCellSet( outputCells );
  }
  else
  {
    ApplyToField applyToField;
    vtkm::worklet::DispatcherMapField<ApplyToField,
                                      DeviceAdapter> applyFieldDispatcher(applyToField);

    applyFieldDispatcher.Invoke(this->InterpolationIds,
                                this->InterpolationWeights,
                                vtkm::filter::Convert(coords, policy),
                                vertices);

    //when we don't merge points, the connectivity array can be represented
    //by a counting array
    typedef typename vtkm::cont::ArrayHandleIndex::StorageTag IndexStorageTag;
    vtkm::cont::CellSetSingleType< IndexStorageTag > outputCells( (vtkm::CellShapeTagTriangle()) );
    vtkm::cont::ArrayHandleIndex connectivity(vertices.GetNumberOfValues());
    outputCells.Fill( connectivity );
    output.AddCellSet( outputCells );
  }

  //no cleanup of the normals is required
  if(this->GenerateNormals)
  {
    vtkm::cont::Field normalField(std::string("normals"), 1,
                                  vtkm::cont::Field::ASSOC_POINTS, normals);
    output.AddField( normalField );
  }


  //add the coordinates to the output dataset
  vtkm::cont::CoordinateSystem outputCoords("coordinates", 1, vertices);
  output.AddCoordinateSystem( outputCoords );

  //todo: figure out how to pass the fields to interpolate to the Result
  return vtkm::filter::DataSetResult(output);
}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
bool MarchingCubes::DoMapField(vtkm::filter::DataSetResult& result,
                               const vtkm::cont::ArrayHandle<T, StorageType>& input,
                               const vtkm::filter::FieldMetadata& fieldMeta,
                               const vtkm::filter::PolicyBase<DerivedPolicy>&,
                               const DeviceAdapter&)
{
  if(fieldMeta.IsPointField() == false)
  {
    //not a point field, we can't map it
    return false;
  }

  //we have a point field so lets map it
  ApplyToField applyToField;
  vtkm::worklet::DispatcherMapField<ApplyToField,
                                    DeviceAdapter> applyFieldDispatcher(applyToField);

  //todo: need to use the policy to get the correct storage tag for output
  vtkm::cont::ArrayHandle<T> output;
  applyFieldDispatcher.Invoke(this->InterpolationIds,
                              this->InterpolationWeights,
                              input,
                              output);

  //use the same meta data as the input so we get the same field name, etc.
  result.GetDataSet().AddField( fieldMeta.AsField(output) );
  return true;

}


}
} // namespace vtkm::filter
