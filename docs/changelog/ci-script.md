# Provide scripts to build Gitlab-ci workers locally

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
