# How to build docker image

```sh
docker build --build-arg https_proxy=https://proxy.com:port --build-arg http_proxy=http://proxy.com:port -t libpmemobj-cpp:debian-unstable -f ./Dockerfile.debian-unstable .
```

# How to use docker image

To run build and tests on local machine on docker:

```sh
docker run --network=bridge --shm-size=4G -v /your/workspace/path/:/opt/workspace:Z -w /opt/workspace/ -e CC=clang-9 -e CXX=clang++-9 -e PKG_CONFIG_PATH=/opt/pmdk/lib/pkgconfig -it libpmemobj-cpp:debian-unstable /bin/bash
```

To get strace working, add to docker commandline

```sh
 --cap-add SYS_PTRACE
```

