//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================
#include <cstdio>
#include <cuda.h>
#include <cuda_runtime.h>
int main()
{
  int count = 0;
  if (cudaSuccess != cudaGetDeviceCount(&count))
    return 1;
  if (count == 0)
    return 1;

  int prev_arch = 0;
  for (int device = 0; device < count; ++device)
  {
    cudaDeviceProp prop;
    if (cudaSuccess == cudaGetDeviceProperties(&prop, device))
    {
      int arch = (prop.major * 10) + prop.minor;
      int compute_level = arch;
      //arch 21 has no equivalent compute level.
      if (compute_level == 21)
      {
        compute_level = 20;
      }

      //handle multiple cards of the same architecture
      if (arch == prev_arch)
      {
        continue;
      }
      prev_arch = arch;

      //we need to print out a semi-colon as this needs to be output
      //as a CMake list which is separated by semicolons
      printf("--generate-code=arch=compute_%d,code=sm_%d;", compute_level, arch);
    }
  }
  return 0;
}
