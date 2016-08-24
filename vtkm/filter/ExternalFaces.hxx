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

namespace vtkm {
namespace filter {

namespace
{

template< typename DeviceAdapter >
class ExternalFacesWorkletWrapper
{
  vtkm::cont::DataSet* Output;
    bool* Valid;
public:


  ExternalFacesWorkletWrapper(vtkm::cont::DataSet& data, bool& v):
    Output(&data),
    Valid(&v)
  { }

  template<typename T, typename U, typename V, typename W>
  void operator()(const vtkm::cont::CellSetExplicit<T,U,V,W>& cellset ) const
  {
    vtkm::cont::ArrayHandle<vtkm::UInt8>       shapes;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> numIndices;
    vtkm::cont::ArrayHandle<vtkm::Id>          conn;

    vtkm::cont::ArrayHandle<vtkm::UInt8>       output_shapes;
    vtkm::cont::ArrayHandle<vtkm::IdComponent> output_numIndices;
    vtkm::cont::ArrayHandle<vtkm::Id>          output_conn;

    shapes = cellset.GetShapesArray(vtkm::TopologyElementTagPoint(),
                                    vtkm::TopologyElementTagCell());

    numIndices = cellset.GetNumIndicesArray(vtkm::TopologyElementTagPoint(),
                                            vtkm::TopologyElementTagCell());

    conn = cellset.GetConnectivityArray(vtkm::TopologyElementTagPoint(),
                                        vtkm::TopologyElementTagCell());


    vtkm::worklet::ExternalFaces exfaces;
    exfaces.run(shapes, numIndices, conn,
                output_shapes, output_numIndices, output_conn,
                DeviceAdapter());

    vtkm::cont::CellSetExplicit<> output_cs(cellset.GetNumberOfPoints(),
                                            cellset.GetName());
    output_cs.Fill(output_shapes, output_numIndices, output_conn);

    this->Output->AddCellSet(output_cs);
    *this->Valid = true;
  }

  template<typename T>
  void operator()(const T& ) const
  {
    //don't support this cell type
    *this->Valid = false;
  }
};

}

//-----------------------------------------------------------------------------
ExternalFaces::ExternalFaces():
  vtkm::filter::FilterDataSet<ExternalFaces>()
{

}

//-----------------------------------------------------------------------------
template<typename DerivedPolicy,
         typename DeviceAdapter>
vtkm::filter::ResultDataSet ExternalFaces::DoExecute(const vtkm::cont::DataSet& input,
                                                     const vtkm::filter::PolicyBase<DerivedPolicy>& policy,
                                                     const DeviceAdapter&)
{
  //1. extract the cell set
  const vtkm::cont::DynamicCellSet& cells =
                  input.GetCellSet(this->GetActiveCellSetIndex());

  //2. using the policy convert the dynamic cell set, and verify
  // that we have an explicit cell set. Once that is found run the
  // external faces worklet
  vtkm::cont::DataSet output;
  bool workletRan = false;
  ExternalFacesWorkletWrapper<DeviceAdapter> wrapper(output, workletRan);
  vtkm::cont::CastAndCall(vtkm::filter::ApplyPolicyUnstructured(cells,policy),
                          wrapper);

  if(!workletRan)
    {
    return vtkm::filter::ResultDataSet();
    }

  //3. add coordinates, etc to the
  output.AddCoordinateSystem(
          input.GetCoordinateSystem(this->GetActiveCoordinateSystemIndex()) );
  return vtkm::filter::ResultDataSet(output);

}

//-----------------------------------------------------------------------------
template<typename T,
         typename StorageType,
         typename DerivedPolicy,
         typename DeviceAdapter>
bool ExternalFaces::DoMapField(vtkm::filter::ResultDataSet&,
                               const vtkm::cont::ArrayHandle<T, StorageType>&,
                               const vtkm::filter::FieldMetadata&,
                               const vtkm::filter::PolicyBase<DerivedPolicy>&,
                               const DeviceAdapter&)
{
  return false;
}

}
}
