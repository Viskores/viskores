//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//o
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#include <GL/glew.h>
#include <vtkm/interop/testing/TestingOpenGLInterop.h>
#include <vtkm/rendering/CanvasOSMesa.h>

//This sets up testing with the default device adapter and array container
#include <vtkm/cont/serial/DeviceAdapterSerial.h>

int UnitTestTransferOSMesa(int, char* [])
{
  //get osmesa canvas to construct a context for us
  vtkm::rendering::CanvasOSMesa canvas(1024, 1024);
  canvas.Initialize();
  canvas.Activate();

  //get glew to bind all the opengl functions
  glewInit();

  return vtkm::interop::testing::TestingOpenGLInterop<vtkm::cont::DeviceAdapterTagSerial>::Run();
}
