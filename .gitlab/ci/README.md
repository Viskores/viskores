# CI Docker images

These images build base images for use in the CI infrastructure.

## Adding or Updating images

Adding/Updating an image to be used for CI infrastructure requires the following process.

1. Start a new git branch
2. Update the associated `Dockerfile`
3. Locally build and verify the docker image
4. 
4. Push the docker image to dockerhub
5. Open a Merge Request to 


### Manual building

After updating the `Dockerfile` (and associated scripts), it's a standard image
build sequence:

```sh
cd $name
docker build -t kitware/vtkm/ci-vtkm-$name-$YYMMDD .
docker push kitware/vtkm/ci-vtkm-$name-$YYMMDD
```

For example to rebuild the `rhe8` `cuda10.2` image we would issue:
```sh
sudo docker build -t kitware/vtkm/ci-vtkm-rhe8-cuda10.2-$YYMMDD .
sudo docker build -t kitware/vtkm/ci-vtkm-rhe8-cuda10.2-$YYMMDD .
```
