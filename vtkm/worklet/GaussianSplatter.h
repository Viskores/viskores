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
#ifndef vtk_m_worklet_GaussianSplatter_h
#define vtk_m_worklet_GaussianSplatter_h

#include <vtkm/Math.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/Field.h>
#include <vtkm/cont/ExplicitConnectivity.h>
#include <vtkm/cont/DataSet.h>

#define VTK_ACCUMULATION_MODE_SUM 2

namespace vtkm
{
namespace worklet
{

  template<typename DeviceAdapter>
  struct GaussianSplatter
  {

    //Return True if the bounds length of the first vector argument
    //is less than or equal to that of the second vector argument; otherwise, False
    struct DimBoundsCompare
    {
       template<typename T>
       VTKM_EXEC_CONT_EXPORT bool operator()(const vtkm::Vec<T,2> &a,
                                             const vtkm::Vec<T,2> &b) const
       {
           bool isLessThan = false;
           if((a[1] - a[0]) <= (b[1] - b[0]))
           {
             isLessThan = true;
           }
           return isLessThan;
       }
    };

    //For each volume dimension, set the spacing and splat distance properties
    class ConfigureVolumeProps : public vtkm::worklet::WorkletMapField
    {
        private:
            vtkm::Float64 MaxDist;

        public:
            typedef void ControlSignature(FieldIn<>, FieldIn<>, FieldOut<>, FieldOut<>);
            typedef void ExecutionSignature(_1, _2, _3, _4);

            VTKM_CONT_EXPORT
            ConfigureVolumeProps(const vtkm::Float64 &c) : MaxDist(c) { };

            template<typename S, typename T>
            VTKM_EXEC_CONT_EXPORT
            void operator()(const S &dimBounds,
                            const T &sampleDimValue,
                            T &spacing,
                            T &splatDistance) const
            {
                //Set the spacing
                spacing = static_cast<T>((dimBounds[1] - dimBounds[0]) / (sampleDimValue - 1));
                if (spacing <= T(0.0))
                {
                  spacing = T(1.0);
                }

                //Set the splat distance
                splatDistance = static_cast<T>(this->MaxDist / spacing);
            }
    };


    public:

    template <typename StorageT,
              typename StorageU,
              typename StorageV>
    void run(const vtkm::cont::ArrayHandle<vtkm::Float64> xValues,
             const vtkm::cont::ArrayHandle<vtkm::Float64> yValues,
             const vtkm::cont::ArrayHandle<vtkm::Float64> zValues,
             vtkm::cont::ArrayHandle<vtkm::Float64> &output_xValues,
             vtkm::cont::ArrayHandle<vtkm::Float64> &output_yValues,
             vtkm::cont::ArrayHandle<vtkm::Float64> &output_zValues
             )
    {
      //Define the constants for the algorithm
      vtkm::Id sampleDim [] = {50, 50, 50};
      vtkm::cont::ArrayHandle<vtkm::Id> sampleDimensions = vtkm::cont::make_ArrayHandle(sampleDim, 3);

      const vtkm::Float64 radius = 0.1;
      const vtkm::Float64 exponentFactor = -5.0;
      const vtkm::Id accumulationMode = VTK_ACCUMULATION_MODE_MAX;
      const vtkm::Float64 scaleFactor = 1.0;

//----------Configure a volume bounding box------------------------//

      //Get the scalar value bounds - min and max - for each dimension
      vtkm::Float64 b1[2];
      xValues.GetBounds(b1, DeviceAdapter);
      vtkm::Vec<vtkm::Float64,2> xBounds = vtkm::make_Vec(b1[0],b1[1]);

      vtkm::Float64 b2[2];
      yValues.GetBounds(b2, DeviceAdapter);
      vtkm::Vec<vtkm::Float64,2> yBounds = vtkm::make_Vec(b2[0],b2[1]);

      vtkm::Float64 b3[2];
      zValues.GetBounds(b3, DeviceAdapter);
      vtkm::Vec<vtkm::Float64,2> zBounds = vtkm::make_Vec(b3[0],b3[1]);

      //Add the bounds vectors to an ArrayHandle for sorting and future use
      vtkm::Vec<vtkm::Float64,2> bounds [] = {xBounds, yBounds, zBounds};
      vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64,2> > allBounds =
          vtkm::cont::make_ArrayHandle(bounds, 3);

      //Sort the bounds vectors from smallest to largest bound
      vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapter>::Sort(allBounds, DimBoundsCompare());

      vtkm::Vec<vtkm::Float64,2> maxBound = allBounds.GetConstPortalControl().Get(2);
      const vtkm::Float64 maxDist = (maxBound[1] - maxBound[0]) * radius;
      const vtkm::Float64 radius2 = maxDist * maxDist;

      //If desired, invoke a WorkletMapField here to adjust the bounds in
      //each direction by maxDist, so that the model fits strictly inside...

      //Set the volume origin
      vtkm::Float64 orig [] = {xBounds[0], yBounds[0], zBounds[0]};
      vtkm::cont::ArrayHandle<vtkm::Float64> origin = vtkm::cont::make_ArrayHandle(orig, 3);

      //Set the volume spacing and splat distance via a WorkletMapField
      vtkm::cont::ArrayHandle<vtkm::Float64> spacing;
      vtkm::cont::ArrayHandle<vtkm::Float64> splatDistance;
      vtkm::worklet::DispatcherMapField<ConfigureVolumeProps> configVolumeDispatcher;
      configVolumeDispatcher.Invoke(allBounds, sampleDimensions, spacing, splatDistance);

      const vtkm::Id numVolumePoints = sampleDim[0] * sampleDim[1] * sampleDim[2];
      const vtkm::Id numSamplePoints = xValues.GetNumberOfValues();


//------------------------Begin splatting phase-------------------------//

      vtkm::cont::ArrayHandle<vtkm::Float64> splatValues;
      splatValues.Allocate(numVolumePoints);
      vtkm::Id visited[numVolumePoints] = {0};  //Initially set all points to unvisited (zero)
      vtkm::cont::ArrayHandle<vtkm::Id> pointVisited = vtkm::cont::make_ArrayHandle(visited, numVolumePoints);;

      // Traverse all sample input points, splatting each into the volume.
      //For each point, determine it's encompassing voxel (volume unit).
      //Then determine the splatter footprint, or subvolume, that the
      //splat affects, and compute the Gaussian splat value for each voxel.

      //Get each sample point's voxel




    }

  }; //struct GaussianSplatter


}} //namespace vtkm::worklet

#endif //vtk_m_worklet_ExternalFaces_h
