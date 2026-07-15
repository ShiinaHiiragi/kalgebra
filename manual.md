# Manual

## Build
1. Change version in `src/main.cpp` and `src/control`

2. Configure CMake

    ```shell
    cmake -S ~/Downloads/kalgebra -B ~/kde/build/kalgebra -G Ninja \
      -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
      -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_PREFIX_PATH="/home/ichinoe/kde/usr;/home/ichinoe/6.5.0/gcc_64" \
      -DCMAKE_INSTALL_PREFIX=/tmp/kalgebra-kai/app
    ln -sf ~/kde/build/kalgebra/compile_commands.json ~/Downloads/kalgebra/compile_commands.json
    ```

3. Build & install:

    ```shell
    cmake --build ~/kde/build/kalgebra --config Debug --target all -j 16
    rm -rf /tmp/kalgebra-kai
    cmake --build ~/kde/build/kalgebra --config Debug --target install -j 16
    ```

    use cmake >= 4.4 to avoid the problem of libwayland duplication

4. Verify viability:

    ```shell
    readelf -d /tmp/kalgebra-kai/app/bin/kalgebra /tmp/kalgebra-kai/app/lib/* | grep RUNPATH
    LD_LIBRARY_PATH=~/6.5.0/gcc_64/lib /tmp/kalgebra-kai/app/bin/kalgebra
    ```

5. Packaging:

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
