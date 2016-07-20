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




// ---------------------------------------------------------------------------
template<typename T>
class ClassifyCell : public vtkm::worklet::WorkletMapPointToCell
{
  struct ClassifyCellTagType : vtkm::ListTagBase<T> { };
public:
  typedef void ControlSignature(
      FieldInPoint< ClassifyCellTagType > inNodes,
      CellSetIn cellset,
      FieldOutCell< IdComponentType > outNumTriangles,
      WholeArrayIn< IdComponentType > numTrianglesTable);
  typedef void ExecutionSignature(_1, _3, _4);
  typedef _2 InputDomain;

  T Isovalue;

  VTKM_CONT_EXPORT
  ClassifyCell(T isovalue) :
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

// ---------------------------------------------------------------------------
struct FirstValueSame
{
  template<typename T, typename U>
  VTKM_EXEC_CONT_EXPORT bool operator()(const vtkm::Pair<T,U>& a,
                                        const vtkm::Pair<T,U>& b) const
  {
    return (a.first == b.first);
  }
};

}

/// \brief Compute the isosurface for a uniform grid data set
template< typename SupportedFieldTypes = marchingcubes::TypeListTagScalars >
class MarchingCubes
{
public:
  typedef vtkm::cont::ArrayHandle<FieldType> WeightHandle;
  typedef vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Id,2> > IdPairHandle;


//----------------------------------------------------------------------------
MarchingCubes::MarchingCubes(bool mergeDuplicates=true,
                             bool generateNormals=false):
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
template<typename ValueType,
         typename CellSetType,
         typename StorageTagField,
         typename StorageTagVertices,
         typename StorageTagNormals,
         typename CoordinateType,
         typename DeviceAdapter>
vtkm::cont::CellSetSingleType< >
     Run(const ValueType &isovalue,
         const CellSetType& cells,
         const vtkm::cont::CoordinateSystem& coordinateSystem,
         const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
         vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3>, StorageTagVertices > vertices,
         const DeviceAdapter& device)
{
  return this->DoRun(isovalue,cells,coordinateSystem,input,vertices, ,false, device);
}

//----------------------------------------------------------------------------
template<typename ValueType,
         typename CellSetType,
         typename StorageTagField,
         typename StorageTagVertices,
         typename StorageTagNormals,
         typename CoordinateType,
         typename DeviceAdapter>
vtkm::cont::CellSetSingleType< >
     Run(const ValueType &isovalue,
         const CellSetType& cells,
         const vtkm::cont::CoordinateSystem& coordinateSystem,
         const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
         vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3>, StorageTagVertices > vertices,
         vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3>, StorageTagNormals > normals,
         const DeviceAdapter& )
{
  return this->DoRun(isovalue,cells,coordinateSystem,input,vertices, normals,true, device);
}

//----------------------------------------------------------------------------
template<typename ArrayHandleIn,
         typename ArrayHandleOut,
         typename DeviceAdapter>
void MapFieldOntoIsosurface(const ArrayHandleIn& input,
                            ArrayHandleOut& output,
                            const DeviceAdapter&)
{
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
         typename StorageTagField,
         typename StorageTagVertices,
         typename StorageTagNormals,
         typename CoordinateType,
         typename DeviceAdapter>
vtkm::cont::CellSetSingleType< >
     DoRun(const ValueType &isovalue,
         const CellSetType& cells,
         const vtkm::cont::CoordinateSystem& coordinateSystem,
         const vtkm::cont::ArrayHandle<ValueType, StorageTagField>& input,
         vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3>, StorageTagVertices > vertices,
         vtkm::cont::ArrayHandle< vtkm::Vec<CoordinateType,3>, StorageTagNormals > normals,
         bool withNormals,
         const DeviceAdapter& )
{
  //With normals
}
{
  using vtkm::worklet::marchingcubes::ApplyToField;
  using vtkm::worklet::marchingcubes::EdgeWeightGenerate;
  using vtkm::worklet::marchingcubes::EdgeWeightGenerateMetaData;
  using vtkm::worklet::marchingcubes::ClassifyCell;

  // Setup the Dispatcher Typedefs
  typedef typename vtkm::worklet::DispatcherMapTopology<
                                      ClassifyCell,
                                      DeviceAdapter
                                      >             ClassifyDispatcher;

  typedef typename vtkm::worklet::DispatcherMapTopology<
                                      EdgeWeightGenerate<DeviceAdapter>,
                                      DeviceAdapter
                                      >             GenerateDispatcher;


  // Call the ClassifyCell functor to compute the Marching Cubes case numbers
  // for each cell, and the number of vertices to be generated
  ClassifyCell<ValueType> classifyCell( isovalue );
  ClassifyDispatcher classifyCellDispatcher(classifyCell);

  vtkm::cont::ArrayHandle<vtkm::IdComponent> numOutputTrisPerCell;
  classifyCellDispatcher.Invoke(field,
                                cells,
                                numOutputTrisPerCell,
                                this->NumTrianglesTable);


  //Pass 2 Generate the edges
  typedef vtkm::cont::ArrayHandle< vtkm::Vec< vtkm::Float32,3> > Vec3HandleType;
  Vec3HandleType normals;

  vtkm::worklet::ScatterCounting scatter(numOutputTrisPerCell, DeviceAdapter());

  EdgeWeightGenerateMetaData<DeviceAdapter> metaData(
                                     scatter.GetOutputRange(numOutputTrisPerCell.GetNumberOfValues()),
                                     normals,
                                     this->InterpolationWeights,
                                     this->InterpolationIds,
                                     this->EdgeTable,
                                     this->NumTrianglesTable,
                                     this->TriangleTable,
                                     scatter);


  EdgeWeightGenerate<DeviceAdapter> weightGenerate(isovalue,
                                                   this->GenerateNormals,
                                                   metaData);

  GenerateDispatcher edgeDispatcher(weightGenerate);
  edgeDispatcher.Invoke( cells,
                        //cast to a scalar field if not one, as cellderivative only works on those
                        marchingcubes::make_ScalarField(field),
                        coords
                        );

  //Now that we have the edge interpolation finished we can generate the
  //following:
  //1. Coordinates ( with option to do point merging )
  //
  //
  typedef vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter> Algorithm;

  vtkm::cont::DataSet output;
  vtkm::cont::ArrayHandle< vtkm::Id > connectivity;
  vtkm::cont::ArrayHandle< vtkm::Vec< vtkm::Float32,3> > vertices;

  typedef vtkm::cont::ArrayHandle< vtkm::Id2 > Id2HandleType;
  typedef vtkm::cont::ArrayHandle<vtkm::FloatDefault> WeightHandleType;
  if(this->MergeDuplicatePoints)
  {
    //Do merge duplicate points we need to do the following:
    //1. Copy the interpolation Ids
    Id2HandleType uniqueIds;
    Algorithm::Copy(this->InterpolationIds, uniqueIds);

    if(this->GenerateNormals)
      {
      typedef vtkm::cont::ArrayHandleZip<WeightHandleType, Vec3HandleType> KeyType;
      KeyType keys = vtkm::cont::make_ArrayHandleZip(this->InterpolationWeights, normals);

      //2. now we need to do a sort by key, making duplicate ids be adjacent
      Algorithm::SortByKey(uniqueIds, keys);

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
      Algorithm::SortByKey(uniqueIds, this->InterpolationWeights);

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
    {

    vtkm::cont::ArrayHandleIndex temp(this->InterpolationIds.GetNumberOfValues());
    Algorithm::Copy(temp, connectivity);
    }
  }

  //no cleanup of the normals is required
  if(this->GenerateNormals)
  {
    vtkm::cont::Field normalField(std::string("normals"),
                                  vtkm::cont::Field::ASSOC_POINTS, normals);
    output.AddField( normalField );
  }

  //assign the connectivity to the cell set
  CellShapeTagTriangle triangleTag;
  vtkm::cont::CellSetSingleType< > outputCells( triangleTag );
  outputCells.Fill( connectivity );
  output.AddCellSet( outputCells );


  //generate the vertices's
  ApplyToField applyToField;
  vtkm::worklet::DispatcherMapField<ApplyToField,
                                    DeviceAdapter> applyFieldDispatcher(applyToField);

  applyFieldDispatcher.Invoke(this->InterpolationIds,
                              this->InterpolationWeights,
                              vtkm::filter::ApplyPolicy(coords, policy),
                              vertices);

  //add the coordinates to the output dataset
  vtkm::cont::CoordinateSystem outputCoords("coordinates", vertices);
  output.AddCoordinateSystem( outputCoords );

  //todo: figure out how to pass the fields to interpolate to the Result
  return vtkm::filter::ResultDataSet(output);
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
