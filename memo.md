# Memo

## Build
1. Configure CMake
2. Build & install:

    ```shell
    mv ~/kde/usr/lib/x86_64-linux-gnu/_libwayland-client.so.0 ~/kde/usr/lib/x86_64-linux-gnu/libwayland-client.so.0
    cmake --build ~/kde/build/kalgebra --config Debug --target all -j 16
    mv ~/kde/usr/lib/x86_64-linux-gnu/libwayland-client.so.0 ~/kde/usr/lib/x86_64-linux-gnu/_libwayland-client.so.0
    cmake --build ~/kde/build/kalgebra --config Debug --target install -j 16
    ```

3. Verify viability:

    ```shell
    readelf -d /tmp/kalgebra/bin/kalgebra /tmp/kalgebra/lib/* | grep RUNPATH
    LD_LIBRARY_PATH=~/aqt/6.5.0/gcc_64/lib /tmp/kalgebra/bin/kalgebra
    ```

4. Packaging:

    ```shell
    mkdir /tmp/kalgebra-kai/DEBIAN
    cp src/control /tmp/kalgebra-kai/DEBIAN/control
    cd /tmp; dpkg-deb --build kalgebra-kai; cd -
    mv /tmp/kalgebra-kai.deb .
    ```

## Execution

```shell
LD_LIBRARY_PATH=/home/user/6.5.0/gcc_64/lib /tmp/kalgebra/bin/kalgebra
```
