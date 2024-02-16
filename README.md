# PTHash-GPU

A minimal perfect hash function on the GPU, based on [PTHash](https://github.com/jermp/pthash).

### Library Usage

Clone (with submodules) this repo and add the following to your `CMakeLists.txt`.

```
add_subdirectory(path/to/GpuPTHash)
target_link_libraries(YourTarget PRIVATE GpuPTHash)
```

### License

This code is licensed under the [GPLv3](/LICENSE).
