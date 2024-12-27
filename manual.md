# Manual

## Build
1. Configure CMake

2. Build & install:

    ```shell
    mv ~/kde/usr/lib/x86_64-linux-gnu/_libwayland-client.so.0 ~/kde/usr/lib/x86_64-linux-gnu/libwayland-client.so.0
    cmake --build ~/kde/build/kalgebra --config Debug --target all -j 16
    rm -rf /tmp/kalgebra-kai
    mv ~/kde/usr/lib/x86_64-linux-gnu/libwayland-client.so.0 ~/kde/usr/lib/x86_64-linux-gnu/_libwayland-client.so.0
    cmake --build ~/kde/build/kalgebra --config Debug --target install -j 16
    ```

3. Verify viability:

    ```shell
    readelf -d /tmp/kalgebra-kai/app/bin/kalgebra /tmp/kalgebra-kai/app/lib/* | grep RUNPATH
    LD_LIBRARY_PATH=~/aqt/6.5.0/gcc_64/lib /tmp/kalgebra-kai/app/bin/kalgebra
    ```

4. Packaging:

    ```shell
    mkdir /tmp/kalgebra-kai/DEBIAN
    cp src/control /tmp/kalgebra-kai/DEBIAN/control
    cd /tmp; dpkg-deb --build kalgebra-kai; cd -
    ```

## Execution
1. Download `aqt` and install `QT 6.5.0`:

    ```shell
    pip install aqtinstall
    aqt install-qt linux desktop 6.5.0 gcc_64 -m all -O ~
    ```

2. Run Kalgebra with `LD_LIBRARY_PATH` specified:

    ```shell
    sudo dpkg -i /tmp/kalgebra-kai.deb
    LD_LIBRARY_PATH=~/6.5.0/gcc_64/lib /app/bin/kalgebra
    ```
