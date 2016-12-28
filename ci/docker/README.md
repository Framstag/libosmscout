# Docker images for continuous integration

When you have Docker installed on your machine, 
you can check libosmscout build on various Linux 
distributions and configurations.

## Build docker images

For build all available images, you can just execute 
`buildAll.sh` script in this directory:

```
./ci/docker/buildAll.sh
```

## Run build in docker containers

Build in docker containers can be started by `runAll.sh`
script in this directory. Without argument, it will
checkout SourceForge git repository, master branch.
To override it, you can specify repository url and branch
as a arguments:

```
./ci/docker/runAll.sh \
  https://github.com/Framstag/libosmscout.git \
  master
```
