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
#ifndef vtk_m_rendering_Plot_h
#define vtk_m_rendering_Plot_h

#include <vtkm/rendering/SceneRenderer.h>
#include <vtkm/rendering/View.h>
#include <vector>

namespace vtkm {
namespace rendering {

class Plot
{
public:
    //Plot(points, cells, field, colortable) {}
    VTKM_CONT_EXPORT
    Plot(const vtkm::cont::DynamicCellSet &cs,
         const vtkm::cont::CoordinateSystem &c,
         const vtkm::cont::Field &f,
         const vtkm::rendering::ColorTable &ct) :
        cellSet(cs), coords(c), scalarField(f), colorTable(ct)
    {
        f.GetBounds(scalarBounds,
                    VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
        c.GetBounds(spatialBounds,
                    VTKM_DEFAULT_DEVICE_ADAPTER_TAG());
    }

    template<typename SceneRendererType, typename SurfaceType>
    VTKM_CONT_EXPORT
    void Render(SceneRendererType &sr,
                SurfaceType &surface, //surface
                vtkm::rendering::View &view)
    {
        sr.SetRenderSurface(&surface);
        sr.SetActiveColorTable(colorTable);
        sr.RenderCells(cellSet, coords, scalarField,
                       colorTable, view, scalarBounds);
    }
    
    vtkm::cont::DynamicCellSet cellSet;
    vtkm::cont::CoordinateSystem coords;
    vtkm::cont::Field scalarField;
    vtkm::rendering::ColorTable colorTable;

    vtkm::Float64 scalarBounds[2];
    vtkm::Float64 spatialBounds[6];
};

}} //namespace vtkm::rendering

#endif //vtk_m_rendering_Plot_h
