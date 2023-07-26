#!/bin/sh

##=============================================================================
##
##  Copyright (c) Kitware, Inc.
##  All rights reserved.
##  See LICENSE.txt for details.
##
##  This software is distributed WITHOUT ANY WARRANTY; without even
##  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
##  PURPOSE.  See the above copyright notice for more information.
##
##=============================================================================

set -e
set -x

# data is expected to be a string of the form YYYYMMDD
readonly date="$1"

cd centos7/cuda10.2
sudo docker build -t kitware/vtkm:ci-centos7_cuda10.2-$date .
cd ../..

cd centos8/base
sudo docker build -t kitware/vtkm:ci-centos8-$date .
cd ../..

cd rhel8/cuda10.2
sudo docker build -t kitware/vtkm:ci-rhel8_cuda10.2-$date .
cd ../..

cd ubuntu1604/base
sudo docker build -t kitware/vtkm:ci-ubuntu1604-$date .
cd ../..

cd ubuntu1604/cuda9.2
sudo docker build -t kitware/vtkm:ci-ubuntu1604_cuda9.2-$date .
cd ../..

cd ubuntu1804/base
sudo docker build -t kitware/vtkm:ci-ubuntu1804-$date .
cd ../..

cd ubuntu1804/cuda11.1
sudo docker build -t kitware/vtkm:ci-ubuntu1804_cuda11.1-$date .
cd ../..

cd ubuntu1804/kokkos-cuda
sudo docker build -t kitware/vtkm:ci-ubuntu1804_cuda11_kokkos-$date .
cd ../..

cd ubuntu2004/doxygen/
sudo docker build -t kitware/vtkm:ci-doxygen-$date .
cd ../..

cd ubuntu2004/kokkos
sudo docker build -t kitware/vtkm:ci-ubuntu2004_kokkos-$date .
cd ../..

# sudo docker login --username=<docker_hub_name>
sudo docker push kitware/vtkm
sudo docker system prune
