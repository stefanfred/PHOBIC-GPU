# PHOBIC-GPU

A minimal perfect hash function construction technique on the GPU, based on the CPU implementation 
[PHOBIC](https://github.com/jermp/pthash/tree/phobic).

If you use PHOBIC in an academic context please cite our paper *ToDo*.

### Requirements
PHOBIC-GPU requires Vulkan.


### Benchmark Usage
Clone (with submodules) this repo and add run the following from the root folder.

```
mkdir out    
cd out
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j
```

You can now run ```./BENCHMARK``` and supply your own configuration.


### Library Usage

Clone (with submodules) this repo and add the following to your `CMakeLists.txt`.

```
add_subdirectory(path/to/PHOBIC-GPU)
target_link_libraries(YourTarget PRIVATE PHOBIC-GPU)
```

An example on how to use the library can be found in ```example.cpp```

### Contact

Feel free to [contact](mailto:hermann@kit.edu) me if you have any questions.

### License

This code is licensed under the [GPLv3](/LICENSE).
