#pragma once

#include <vector>
#include <iostream>
#include <fstream>
#include <cstring>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        std::string error = "failed to open file: " + filename;
        throw std::runtime_error(error);
    }

    size_t fileSize = (size_t)file.tellg();

    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    return buffer;
}

static void writeImageNormalized(int height, int width, float max, std::vector<float> imageArray) {
    FILE* imageFile;

    imageFile = fopen("image.pgm", "wb");
    if (imageFile == NULL) {
        perror("ERROR: Cannot open output file");
        exit(EXIT_FAILURE);
    }

    fprintf(imageFile, "P5\n");           // P5 filetype
    fprintf(imageFile, "%d %d\n", width, height);   // dimensions
    fprintf(imageFile, "255\n");          // Max pixel

    /* Now write a greyscale ramp */
    for (int i = 0; i < height * width; i++) {
        fputc(imageArray[i]/max * 255, imageFile);
    }
    fclose(imageFile);
}


