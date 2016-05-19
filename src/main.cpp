//
// Copyright (C) 2016 Emmanuel Durand
//
// This file is part of doodle2stl.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// Splash is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this software.  If not, see <http://www.gnu.org/licenses/>.
//

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include "./stb/stb_image.h"
#include "./stb/stb_image_write.h"

using namespace std;

/*************/
struct Image
{
    vector<uint8_t> data {};
    int width {0};
    int height {0};
    int channels {0};
};

/*************/
bool loadImage(const string& filename, Image& out)
{
    auto rawImage = stbi_load(filename.c_str(), &out.width, &out.height, &out.channels, 0);
    if (!rawImage)
    {
        cerr << "Error while trying to load image " << filename << "." << endl;
        return false;
    }
    
    auto imgSize = out.width * out.height * out.channels;
    out.data.resize(imgSize);
    memcpy(out.data.data(), rawImage, imgSize);
    stbi_image_free(rawImage);

    return true;
}

/*************/
bool saveImage(const string& filename, const Image& image)
{
    auto result = stbi_write_tga(filename.c_str(), image.width, image.height, image.channels, image.data.data());
    return result != 0 ? false : true;
}

/*************/
void filterImage(Image& image)
{
    // Search the mean value on all three channels
    vector<float> meanColor(image.channels);
    for (auto& c : meanColor)
        c = 0;

    for (int y = 0; y < image.height; ++y)
        for (int x = 0; x < image.width; ++x)
            for (int c = 0; c < image.channels; ++c)
                meanColor[c] += image.data[(x + y * image.width) * image.channels + c];

    for (auto& c : meanColor)
        c = c / (image.width * image.height * image.channels);

    // Compute the stddev
    vector<float> stddevColor(image.channels);
    for (int y = 0; y < image.height; ++y)
        for (int x = 0; x < image.width; ++x)
            for (int c = 0; c < image.channels; ++c)
                stddevColor[c] += pow((float)image.data[(x + y * image.width) * image.channels + c] - meanColor[c], 2.f);

    for (auto& c : stddevColor)
        c = sqrt(c / (image.width * image.height * image.channels));

    // Find pixels higher by at least 2 stddev from the mean on one channel, set them to black
    // The other ones will be white
    for (int y = 0; y < image.height; ++y)
        for (int x = 0; x < image.width; ++x)
        {
            bool isHigher = false;
            for (int c = 0; c < image.channels; ++c)
            {
                if (image.data[(x + y * image.width) * image.channels + c] > meanColor[c] + stddevColor[c])
                    isHigher = true;
            }

            if (isHigher)
            {
                for (int c = 0; c < image.channels; ++c)
                    image.data[(x + y * image.width) * image.channels + c] = 255;
            }
            else
            {
                for (int c = 0; c < image.channels; ++c)
                    image.data[(x + y * image.width) * image.channels + c] = 0;
            }
        }
}

/*************/
int main(int argc, char** argv)
{
    string filename {""};

    // Fill the parameters
    for (int i = 1; i < argc;)
    {
        if (i == argc - 1)
        {
            filename = string(argv[i]);
            ++i;
        }
    }

    // Load the image
    Image image;
    if (!loadImage(filename, image))
    {
        return 1;
    }

    // Apply some filters on it
    filterImage(image);

    // Save the image for debug
    saveImage("debug.tga", image);

    return 0;
}
