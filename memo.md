# Memo

## Build
1. Build & install:
    ```shell
    mv ~/kde/usr/lib/x86_64-linux-gnu/_libwayland-client.so.0 ~/kde/usr/lib/x86_64-linux-gnu/libwayland-client.so.0
    /usr/bin/cmake --build ~/kde/build/kalgebra --config Debug --target all -j 16
    mv ~/kde/usr/lib/x86_64-linux-gnu/libwayland-client.so.0 ~/kde/usr/lib/x86_64-linux-gnu/_libwayland-client.so.0
    /usr/bin/cmake --build ~/kde/build/kalgebra --config Debug --target install -j 16
    ```

2. Verify viability:

    ```shell
    LD_LIBRARY_PATH=~/aqt/6.5.0/gcc_64/lib /tmp/kalgebra/bin/kalgebra
    ```

3. Packaging:

    ```shell
    ...
    ```

## Execution

```shell
LD_LIBRARY_PATH=/home/user/6.5.0/gcc_64/lib /tmp/kalgebra/bin/kalgebra
```
