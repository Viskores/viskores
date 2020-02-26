
Gitlab CI
===============

# High level view
1. Kitware Gitlab CI
    - Why pipelines
    - Existing build and test pipelines
    - Gitlab runner tags

2. How to use docker builders locally
    - Setting up docker
    - Setting up nvidia runtime
    - Running docker images

3. How to Add/Update Kitware Gitlab CI
    - How to add a new builder
    - How to add a new tester
    - Docker image structure
    - How to update an existing docker image

4. ECP OSTI CI
    - Issues

# Kitware Gitlab CI

GitLab CI/CD allows for software development through continous integration, delivery, and deployment.
VTK-m uses continuous integration to verify every merge request, by running a pipeline of scripts to build, test,
the code changes across a wide range of hardware and configurations before merging them into master.

This workflow allow everyone to easily catch build failures, bugs, and errors before VTK-m is deployed in a
production enviornment. Making sure VTK-m is a robust library provides not only confidence to our users
but to every VTK-m developer. When the system is working developers can be confident that failures
seen during CI are related to the specific changes they have made.

GitLab CI/CD is configured by a file called `.gitlab-ci.yml` located at the root of the VTK-m repository.
The scripts set in this file are executed by the [GitLab Runners](https://docs.gitlab.com/runner/) associated with VTK-m.

## Why pipelines
How this separates our the build and test components of the pipeline

Pipelines are the top-level component of continuous integration, delivery, and deployment.

Pipelines comprise:

Jobs that define what to run. For example, code compilation or test runs.
Stages that define when and how to run. For example, that tests run only after code compilation.
Multiple jobs in the same stage are executed by Runners in parallel, if there are enough concurrent Runners.

If all the jobs in a stage:

Succeed, the pipeline moves on to the next stage.
Fail, the next stage is not (usually) executed and the pipeline ends early.

## Existing build and test pipelines
## Gitlab runner tags

Current gitlab runner tags for VTK-m are:

    - build
        Signifies that this is will be doing compilation
    - test
        Signifies that this is will be running tests
    - vtkm
        Allows us to make sure VTK-m ci is only run on VTK-m allocated hardware
    - docker
        Used to state that the gitlab-runner must support docker based ci
    - linux
        Used to state that we require a linux based gitlab-runner
    - large-memory
        Used to state that this step will require a machine that has lots of memory.
        This is currently used for cuda `build` requests
    - cuda-rt
        Used to state that the runner is required to have the cuda runtime enviornment.
        This isn't required to `build` VTK-m, only `test`
    - maxwell
    - pascal
    - turing
        Only used on a `test` stage to signifiy which GPU hardware is required to
        run the VTK-m tests

# How to use docker builders locally
## Setting up docker
## Setting up nvidia runtime
## Running docker images


# How to Add/Update Kitware Gitlab CI

## How to add a new builder

Adding builders is necessary when a given combination of compiler, platform,
and VTK-m options isn't already captured by existing builders.

Each builder definition is placed inside the respective OS `yml` file located in
`.gitlab/ci/`. Therefore if you are adding a builder that will run on Ubuntu 20.04 it
would go into `.gitlab/ci/ubuntu2004.yml`.

As each builder tests a given set of flags, we need to encode them in the yml definition.
This information is encoded via 3 ways; tags, variables, and extends.

Tags are used to by gitlab-ci to match a given build to a set of possible execution locations.
Therefore we encode information such as we require docker or the linux kernel into tags.
The full set of VTK-m tags each meaning are found under the `Builder tags` section of the document.

Extends is used to compose the actual execution component of the builder with any information.
So a linux docker based builder would extend the docker image they want, plus `.cmake_build_linux`. A MacOS builder would extend `.cmake_build_macos`.


Variables are used to
The defitinon of the builder would look roughly like
```yml
build:ubuntu2004_$<compiler>:
  tags:
    - build
    - vtkm
    - docker
    - linux
  extends:
    - .ubuntu2004
    - .cmake_build_linux
  variables:
    CC: "$<c-compiler-command>"
    CXX: "$<cxx-compiler-command>"
    CMAKE_BUILD_TYPE: "Debug|Release|MinSizeRel"
    VTKM_SETTINGS: "tbb+openmp+mpi"
```

If this builder requires a new docker image a coupe of extra steps are required

1. Add the docker image to the proper folder under `.gitlab/ci/docker`. Images
are laid out with the primary folder being the OS and the secondary folder the
primary device adapter it adds. We currently consider `openmp` and `tbb` to
be small enough to be part of any image.

2. Make sure image is part of the `update_all.sh` script, following the convention
of `platform_device`.

3. Update the `.gitlab-ci.yml` comments to list what compiler(s), device adapters,
and other relevant libraries the image has.

4. Verify the image is part of the `.gitlab-ci.yml` file and uses the docker image
pattern, as seen below. This is important as `.docker_image` makes sure we
have consistent paths across all builds to allow us to cache compilation object
files.

```yml
.$<platform>_$<device>: &$<platform>_$<device>
  image: "kitware/vtkm:ci-$<platform>_$<device>-$<YYYYMMDD>"
  extends:
    - .docker_image
```

## How to add a new tester
## Docker image structure

## How to update an existing docker image

Updating an image to be used for CI infrastructure can be done by anyone that
has permissions to the kitware/vtkm dockerhub project, as that is where
images are stored.

Each modification of the docker image requires a new name so that existing open
merge requests can safely trigger pipelines without inadverntly using the
updated images which might break their build.

Therefore the workflow to update images is
1. Start a new git branch
2. Update the associated `Dockerfile`
3. Locally build the docker image
4. Push the docker image to dockerhub
5. Open a Merge Request


To simplify step 3 and 4 of the process, VTK-m has a script (`update_all.sh`) that automates
these stages. This script is required to be run from the `.gitlab/ci/docker` directory, and
needs to have the date string passed to it. An example of running the script:

```sh
sudo docker login --username=<docker_hub_name>
cd .gitlab/ci/docker
sudo ./update_all.sh 20201230
```

# ECP OSTI CI

`.gitlab-ci-ecp.yml` allows for VTK-m to run CI on provided by ECP at NMC.

To have this work properly you will need to make sure that the gitlab repository
has been updated to this non-standard yaml file location
( "Settings" -> "CI/CD" -> "General pipelines" -> "Custom CI configuration path").

The ECP CI is setup to verify VTK-m mainly on Power9 hardware as that currently is
missing from VTK-m standard CI infrastructure.

Currently we verify Power9 support with `cuda` and `openmp` builders. The `cuda` builder
is setup to use the default cuda SDK on the machine and the required `c++` compiler which
currently is `gcc-4.8.5`. The `openmp` builder is setup to use the newest `c++` compiler provided
on the machine so that we maximimze compiler coverage.

## Issues
Currently these builders don't report back to the VTK-m CDash instance.

