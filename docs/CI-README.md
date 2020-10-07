
Gitlab CI
===============

# High level view
1. Kitware Gitlab CI
    - Why pipelines
    - Gitlab runner tags

2. How to use docker builders locally
    - Setting up docker
    - Setting up nvidia runtime
    - Running docker images

3. How to Add/Update Kitware Gitlab CI
    - How to add a new builder
    - How to add a new tester
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

Pipelines are the top-level component of continuous integration. For VTK-m the pipeline contains build and test stages, with the possibilty of adding subsequent stages such as coverage, or memory checking.

Decomposing the build and test into separate components comes with some significant benifits for VTK-m developers.
The most impactful change is that we now have the ability to compile VTK-m on dedicated 'compilation' machines and
test on machines with less memory or an older CPU improving turnaround time. Additionally since we are heavily
leveraging docker, VTK-m build stages can be better load balanced across the set of builders as we don't have
a tight coupling between a machine and build configuration.

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
        This is currently used for CUDA `build` requests
    - cuda-rt
        Used to state that the runner is required to have the CUDA runtime environment.
        This is required to `build` and `test` VTK-m when using CUDA 
    - maxwell
    - pascal
    - turing
        Only used on a `test` stage to signify which GPU hardware is required to
        run the VTK-m tests

# How to use docker builders locally

When diagnosing issues from the docker builders it can be useful to iterate locally on a 
solution.

If you haven't set up docker locally we recommend following the official getting started guide:
    - https://docs.docker.com/get-started/


## Setting up nvidia runtime

To properly test VTK-m inside docker containers when the CUDA backend is enabled you will need
to have installed the nvidia-container-runtime ( https://github.com/NVIDIA/nvidia-container-runtime )
and be using a recent version of docker ( we recommend docker-ce )


Once nvidia-container-runtime is installed you will want the default-runtime be `nvidia` so
that `docker run` will automatically support gpus. The easiest way to do so is to add
the following to your `/etc/docker/daemon.json`

```
{
 "default-runtime": "nvidia",
    "runtimes": {
        "nvidia": {
            "path": "/usr/bin/nvidia-container-runtime",
            "runtimeArgs": []
        }
    },
}
```

## Running docker images

To simplify reproducing docker based CI workers locally, VTK-m has python program that handles all the
work automatically for you.

The program is located in `[Utilities/CI/reproduce_ci_env.py ]` and requires python3 and pyyaml. 

To use the program is really easy! The following two commands will create the `build:rhel8` gitlab-ci
worker as a docker image and setup a container just as how gitlab-ci would be before the actual
compilation of VTK-m. Instead of doing the compilation, instead you will be given an interactive shell. 

```
./reproduce_ci_env.py create rhel8
./reproduce_ci_env.py run rhel8
```

To compile VTK-m from the the interactive shell you would do the following:
```
> src]# cd build/
> build]# cmake --build .
```

# How to Add/Update Kitware Gitlab CI

Adding new build or test stages is necessary when a given combination of compiler, platform,
and VTK-m options isn't already captured by existing builders. Each definition is composed via 3 components; tags, variables, and extends.

Tags are used to by gitlab-ci to match a given build to a set of possible execution locations.
Therefore we encode information such as we require docker or the linux kernel into tags.
The full set of VTK-m tags each meaning are found under the `runner tags` section of the document.

Extends is used to compose the execution enviornment of the builder. Basically this means
setting up the correct build/test enviornment and specifying the CMake scripts that need
to be executed. So a linux docker based builder would extend the docker image they want,
plus `.cmake_build_linux`. A MacOS builder would extend `.cmake_build_macos`.

Variables control stage specific information such as runtime enviornment variables,
or VTK-m CMake options.

## How to add a new builder

Each builder definition is placed inside the respective OS `yml` file located in
`.gitlab/ci/`. Therefore if you are adding a builder that will run on Ubuntu 20.04 it
would go into `.gitlab/ci/ubuntu2004.yml`.

Variables are used to control the following components:

    - Compiler
    - VTK-m CMake Options
    - Static / Shared
    - Release / Debug / MinSizeRel

An example defitinon of a builder would look like:
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
    - .only-default
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

Each test definition is placed inside the respective OS `yml` file located in
`.gitlab/ci/`. Therefore if you are adding a builder that will run on Ubuntu 20.04 it
would go into `.gitlab/ci/ubuntu2004.yml`.

The primary difference between tests and build definitions are that tests have
the dependencies and needs sections. These are required as by default
gitlab-ci will not run any test stage before ALL the build stages have
completed.

Variables for testers are currently only used for the following things:
    - Allowing OpenMPI to run as root

An example defitinon of a tester would look like:
```yml
test:ubuntu2004_$<compiler>:
  tags:
    - test
    - cuda-rt
    - turing
    - vtkm
    - docker
    - linux
  extends:
    - .ubuntu2004_cuda
    - .cmake_test_linux
    - .only-default
  dependencies:
    - build:ubuntu2004_$<compiler>
  needs:
    - build:ubuntu2004_$<compiler>
```

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
