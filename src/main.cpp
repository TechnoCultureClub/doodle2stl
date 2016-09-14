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

#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <opencv/cv.hpp>

using namespace std;

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
// Filters the image to keep the doodle
void filterImage(cv::Mat& image, int subdiv)
{
    // Resize
    cv::Mat tmpMat;
    cv::resize(image, tmpMat, cv::Size(800, (800 * image.rows) / image.cols));
    image = tmpMat;

    // Edge detection
    cv::Mat edges(image.rows, image.cols, CV_8U);
    cv::Canny(image, edges, 20, 40, 3);

    // Edge filtering
    auto structElem = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(edges, tmpMat, cv::MORPH_CLOSE, structElem, cv::Point(-1, -1), 2);
    cv::morphologyEx(tmpMat, edges, cv::MORPH_OPEN, structElem, cv::Point(-1, -1), 1);

    image = edges;
}

/*************/
// Converts a binary image to a 2D mesh
// The conversion is based on an existing matrice of vertices, with a given resolution
int binaryToMesh(const cv::Mat& image, Mesh& out, int resolution)
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
        else
        {
            ++i;
        }
    }

    // Load the image
    auto image = cv::imread(filename, cv::IMREAD_GRAYSCALE);

    // Apply some filters on it
    filterImage(image, subdivision);

    // Save the image for debug
    cv::imwrite("debug.png", image);

    // Create the base 2D mesh from the binary image
    Mesh mesh2D;
    auto nbrFaces = binaryToMesh(image, mesh2D, resolution);

    return 0;
}
