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
    vector<uint8_t> data{};
    int width{0};
    int height{0};
    int channels{0};
};

/*************/
struct Roi
{
    int x{0};
    int y{0};
    int w{0};
    int h{0};
};

/*************/
struct Vertex
{
    float x{0.f};
    float y{0.f};
    float z{0.f};
};

struct Mesh
{
    vector<Vertex> vertices;
    vector<uint32_t> faces;
};

/*************/
// Load the image from the name, stores it in out, returns true if success
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
// Saves the image given in parameter, returns true if success
bool saveImage(const string& filename, const Image& image)
{
    auto result = stbi_write_tga(filename.c_str(), image.width, image.height, image.channels, image.data.data());
    return result != 0 ? false : true;
}

/*************/
// Filter only a portion of the image
void filterROI(Image& image, Roi roi)
{
    cout << "------------" << endl;
    int xshift = roi.x;
    int yshift = roi.y;
    int xmax = min(image.width, roi.x + roi.w);
    int ymax = min(image.height, roi.y + roi.h);

    // Search the mean value on all three channels
    vector<float> meanColor(image.channels);
    for (auto& c : meanColor)
        c = 0;

    for (int y = yshift; y < ymax; ++y)
        for (int x = xshift; x < xmax; ++x)
            for (int c = 0; c < image.channels; ++c)
                meanColor[c] += image.data[(x + y * image.width) * image.channels + c];

    for (auto& c : meanColor)
        c = c / (roi.w * roi.h);

    cout << "Mean color: ";
    for (auto& c : meanColor)
        cout << c << " ";
    cout << endl;

    // Compute the stddev
    vector<float> stddevColor(image.channels);
    for (int y = yshift; y < ymax; ++y)
        for (int x = xshift; x < xmax; ++x)
            for (int c = 0; c < image.channels; ++c)
                stddevColor[c] += pow((float)image.data[(x + y * image.width) * image.channels + c] - meanColor[c], 2.f);

    for (auto& c : stddevColor)
        c = sqrt(c / (roi.w * roi.h));

    cout << "Stddev color: ";
    for (auto& c : stddevColor)
        cout << c << " ";
    cout << endl;

    // Find pixels higher by at least 2 stddev from the mean on one channel, set them to black
    // The other ones will be white
    for (int y = yshift; y < ymax; ++y)
        for (int x = xshift; x < xmax; ++x)
        {
            bool isHigher = false;
            for (int c = 0; c < image.channels; ++c)
            {
                if (image.data[(x + y * image.width) * image.channels + c] > meanColor[c] - stddevColor[c] / 2)
                    isHigher = true;
            }

            // If stddev is really small, consider all pixels as white
            bool isStddevSmall = true;
            for (auto& c : stddevColor)
                if (c > 10)
                    isStddevSmall = false;
            if (isStddevSmall)
                isHigher = true;

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
// Filters the image to keep the doodle
void filterImage(Image& image, int subdiv)
{
    Roi roi;
    roi.w = image.width / subdiv;
    roi.h = image.height / subdiv;

    for (int y = 0; y < image.height; y += roi.h)
    {
        auto localRoi = roi;
        localRoi.y = y;
        localRoi.h = min(roi.h, image.height - y);

        for (int x = 0; x < image.width; x += roi.w)
        {
            localRoi.x = x;
            localRoi.w = min(roi.w, image.width - x);
            filterROI(image, localRoi);
        }
    }
}

/*************/
// Converts a binary image to a 2D mesh
// The conversion is based on an existing matrice of vertices, with a given resolution
int binaryToMesh(const Image& image, Mesh& out, int resolution)
{
}

/*************/
int main(int argc, char** argv)
{
    string filename{""};
    int resolution{128};
    int subdivision{2};

    // Fill the parameters
    for (int i = 1; i < argc;)
    {
        if (i == argc - 1)
        {
            filename = string(argv[i]);
            ++i;
        }
        else if (i < argc - 1 && (string(argv[i]) == "-r" || string(argv[i]) == "--resolution"))
        {
            resolution = stoi(string(argv[i + 1]));
            if (resolution == 0)
                resolution = 128;
            i += 2;
        }
        else if (i < argc - 1 && (string(argv[i]) == "-s" || string(argv[i]) == "--subdiv"))
        {
            subdivision = std::max(1, stoi(string(argv[i + 1])));
            i += 2;
        }
    }

    // Load the image
    Image image;
    if (!loadImage(filename, image))
        return 1;

    // Apply some filters on it
    filterImage(image, subdivision);

    // Save the image for debug
    saveImage("debug.tga", image);

    // Create the base 2D mesh from the binary image
    Mesh mesh2D;
    auto nbrFaces = binaryToMesh(image, mesh2D, resolution);

    return 0;
}
