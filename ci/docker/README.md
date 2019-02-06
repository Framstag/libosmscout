# Docker images for continuous integration

When you have Docker installed on your machine, 
you can check libosmscout build on various Linux 
distributions and configurations.

## Requirements

 - docker ( >= 18.04)
 - seccomp library ( >= 2.3.3)

Qt tools in our Debian and Archlinux docker images are using 
`statx` syscall. This syscall is not whitelisted in older 
docker and it makes our builds failing. This is fixed in 
docker upstream [#208](https://github.com/docker/for-linux/issues/208) 
already, but it also requires new seccomp library 
[#1755250](https://bugs.launchpad.net/ubuntu/+source/docker.io/+bug/1755250).
To workaround this issue, you need to run containers with `--privileged` switch.

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
