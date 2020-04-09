#How to setup machine to use CI scripts#

#OSX and Unix#


# Requirements #

- Docker
- Python3
-- PyYAML

The CI scripts require python3 and the PyYAML package.

Generally the best way to setup this environment is to create a python
virtual env so you don't pollute your system. This means getting pip
the python package manager, and virtual env which allow for isolation
of a projects python dependencies.

```
sudo easy_install pip
sudo pip install virtualenv
```

Next we need to create a new virtual env of python. I personally
like to setup this in `vtkm/Utilities/CI/env`.

```
mkdir env
virtualenv env
```

Now all we have to do is setup the requirements:

```
./env/bin/pip install -r requirements.txt
```

