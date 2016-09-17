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
#include <fstream>
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
    Vertex();
    Vertex(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}
    float x{0.f};
    float y{0.f};
    float z{0.f};
};

struct Face
{
    Face(){};
    vector<uint32_t> indices;
};

struct Mesh
{
    vector<Vertex> vertices{};
    vector<Face> faces{};
};

/*************/
// Filters the image to keep the doodle
void filterImage(cv::Mat& image)
{
    // Resize
    cv::Mat tmpMat;
    cv::resize(image, tmpMat, cv::Size(800, (800 * image.rows) / image.cols));
    image = tmpMat;

    // Edge detection
    cv::Mat edges(image.rows, image.cols, CV_8U);
    cv::Canny(image, edges, 15, 30, 3);

    // Edge filtering
    auto structElem = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::morphologyEx(edges, tmpMat, cv::MORPH_CLOSE, structElem, cv::Point(-1, -1), 2);
    cv::morphologyEx(tmpMat, edges, cv::MORPH_OPEN, structElem, cv::Point(-1, -1), 1);

    // Find and draw the biggest contour
    vector<vector<cv::Point>> contours;
    vector<cv::Vec4i> hierarchy;
    cv::findContours(edges, contours, hierarchy, cv::RETR_CCOMP, cv::CHAIN_APPROX_TC89_L1);

    tmpMat = cv::Mat::zeros(image.rows, image.cols, CV_8U);
    for (int i = 0; i >= 0; i = hierarchy[i][0])
    {
        if (cv::contourArea(contours[i]) < 512)
            continue;

        cv::Scalar color(255, 255, 255);
        drawContours(tmpMat, contours, i, color, -1);

        for (int j = hierarchy[i][2]; j >= 0; j = hierarchy[j][0])
        {
            if (cv::contourArea(contours[j]) < 512)
                continue;

            cv::Scalar color(0, 0, 0);
            drawContours(tmpMat, contours, j, color, -1);
        }
    }

    image = tmpMat;
}

/*************/
// Converts a binary image to a 2D mesh
// The conversion is based on an existing matrice of vertices, with a given resolution
int getNeighbourCount(const cv::Mat& image, int x, int y)
{
    int count = 0;
    if (image.at<uint8_t>(y, x) != 0)
        ++count;
    if (y + 1 < image.rows && image.at<uint8_t>(y + 1, x) != 0)
        ++count;
    if (y + 1 < image.rows && x + 1 < image.cols && image.at<uint8_t>(y + 1, x + 1) != 0)
        ++count;
    if (x + 1 < image.cols && image.at<uint8_t>(y, x) != 0)
        ++count;

    return count;
}

int findBottomVertex(const vector<Vertex>& line, float x)
{
    if (line.size() == 0)
        return -1;

    for (int i = 0; i < line.size(); ++i)
        if (abs(x - line[i].x) < 0.01)
            return i;

    return -1;
}

int binaryToMesh(const cv::Mat& image, Mesh& mesh, int resolution, float height)
{
    // Reduce image resolution
    cv::Mat map;
    cv::resize(image, map, cv::Size(resolution, (resolution * image.rows) / image.cols));
    cv::threshold(map, map, 128, 255, cv::THRESH_BINARY);

    // First pass: add vertices where needed
    vector<vector<Vertex>> vertices;
    int vertexCount = 0;
    for (int y = 0; y < map.rows - 1; ++y)
    {
        vertices.push_back(vector<Vertex>());
        for (int x = 0; x < map.cols - 1; ++x)
        {
            if (getNeighbourCount(map, x, y) >= 2)
            {
                vertices[y].push_back(Vertex(x, y, 0.f));
                ++vertexCount;
            }
        }
    }

    // Second pass: create faces for the bottom part
    uint32_t totalIndex = 0;
    vector<int> verticesFaceCount(vertexCount, 0);
    for (int y = 0; y < vertices.size(); ++y)
    {
        if (vertices[y].size() < 2)
        {
            totalIndex += vertices[y].size();
            continue;
        }

        for (int x = 0; x < vertices[y].size() - 1; ++x)
        {
            auto& vertex = vertices[y][x];
            auto& next = vertices[y][x + 1];

            auto bottomIndex = findBottomVertex(vertices[y + 1], vertex.x);
            if (bottomIndex != -1)
            {
                auto& bottom = vertices[y + 1][bottomIndex];
                auto bottomNextIndex = -1;
                if (bottomIndex < vertices[y + 1].size() - 1)
                {
                    auto& bottomNext = vertices[y + 1][bottomIndex + 1];
                    if (bottomNext.x - bottom.x < 1.01)
                        bottomNextIndex = bottomIndex + 1;
                }

                if (next.x - vertex.x < 1.01)
                {
                    if (bottomNextIndex >= 0)
                    {
                        Face face;
                        face.indices.push_back(totalIndex + x);
                        face.indices.push_back(totalIndex + x + 1);
                        face.indices.push_back(totalIndex + vertices[y].size() + bottomIndex + 1);
                        face.indices.push_back(totalIndex + vertices[y].size() + bottomIndex);
                        mesh.faces.push_back(face);

                        verticesFaceCount[totalIndex + x] += 1;
                        verticesFaceCount[totalIndex + x + 1] += 1;
                        verticesFaceCount[totalIndex + vertices[y].size() + bottomIndex + 1] += 1;
                        verticesFaceCount[totalIndex + vertices[y].size() + bottomIndex] += 1;
                    }
                    else
                    {
                        Face face;
                        face.indices.push_back(totalIndex + x);
                        face.indices.push_back(totalIndex + x + 1);
                        face.indices.push_back(totalIndex + vertices[y].size() + bottomIndex);
                        mesh.faces.push_back(face);

                        verticesFaceCount[totalIndex + x] += 1;
                        verticesFaceCount[totalIndex + x + 1] += 1;
                        verticesFaceCount[totalIndex + vertices[y].size() + bottomIndex] += 1;
                    }
                }
                else
                {
                    if (bottomNextIndex >= 0)
                    {
                        Face face;
                        face.indices.push_back(totalIndex + x);
                        face.indices.push_back(totalIndex + vertices[y].size() + bottomIndex + 1);
                        face.indices.push_back(totalIndex + vertices[y].size() + bottomIndex);
                        mesh.faces.push_back(face);

                        verticesFaceCount[totalIndex + x] += 1;
                        verticesFaceCount[totalIndex + vertices[y].size() + bottomIndex] += 1;
                        verticesFaceCount[totalIndex + vertices[y].size() + bottomIndex + 1] += 1;
                    }
                }
            }
            else
            {
                auto bottomNextIndex = findBottomVertex(vertices[y + 1], vertex.x + 1.f);
                if (bottomNextIndex != -1 && next.x - vertex.x < 1.01)
                {
                    Face face;
                    face.indices.push_back(totalIndex + x);
                    face.indices.push_back(totalIndex + x + 1);
                    face.indices.push_back(totalIndex + vertices[y].size() + bottomNextIndex);
                    mesh.faces.push_back(face);

                    verticesFaceCount[totalIndex + x] += 1;
                    verticesFaceCount[totalIndex + x + 1] += 1;
                    verticesFaceCount[totalIndex + vertices[y].size() + bottomIndex + 1] += 1;
                }
            }
        }

        totalIndex += vertices[y].size();
    }

    // Third pass: fill mesh.vertices
    for (int y = 0; y < vertices.size(); ++y)
    {
        if (vertices[y].size() == 0)
            continue;

        for (int x = 0; x < vertices[y].size(); ++x)
            mesh.vertices.push_back(vertices[y][x]);
    }

    // Fourth pass: go through all faces, and extrude them along outter edges
    int faceCount = mesh.faces.size();
    for (int i = 0; i < faceCount; ++i)
    {
        vector<int> outter;
        for (auto idx : mesh.faces[i].indices)
        {
            if (verticesFaceCount[idx] < 4)
                outter.push_back(idx);
            else
                outter.push_back(-1);
        }

        for (int j = 0; j < outter.size(); ++j)
        {
            auto curr = outter[j];
            auto next = outter[(j + 1) % outter.size()];
            if (curr >= 0 && next >= 0)
            {
                Face face;
                face.indices.push_back(curr);
                face.indices.push_back(next);

                mesh.vertices.push_back(mesh.vertices[next]);
                mesh.vertices[mesh.vertices.size() - 1].z = height;
                face.indices.push_back(mesh.vertices.size() - 1);

                mesh.vertices.push_back(mesh.vertices[curr]);
                mesh.vertices[mesh.vertices.size() - 1].z = height;
                face.indices.push_back(mesh.vertices.size() - 1);

                mesh.faces.push_back(face);
            }
        }
    }

    // Fifth pass: fill the top
    for (int i = 0; i < faceCount; ++i)
    {
        Face face;

        for (int j = 0; j < mesh.faces[i].indices.size(); ++j)
        {
            auto vertex = mesh.vertices[mesh.faces[i].indices[j]];
            vertex.z = height;
            mesh.vertices.push_back(vertex);
            face.indices.push_back(mesh.vertices.size() - 1);
        }

        mesh.faces.push_back(face);
    }

    return mesh.faces.size();
}

/*************/
bool writeSTL(const string& filename, const Mesh& mesh)
{
    ofstream file(filename, ios::binary);
    if (!file.is_open())
        return false;

    file << "solid doodle" << endl;
    for (auto& f : mesh.faces)
    {
        // if (f.indices.size() != 3)
        //    continue;
        if (f.indices.size() == 3)
        {
            file << "facet normal 0.0 0.0 0.0" << endl;
            file << "  outer loop" << endl;
            for (auto i : f.indices)
            {
                auto& vertex = mesh.vertices[i];
                file << "    vertex " << vertex.x << " " << vertex.y << " " << vertex.z << endl;
            }
            file << "  endloop" << endl;
            file << "endfacet" << endl;
        }
        else if (f.indices.size() != 3)
        {
            file << "facet normal 0.0 0.0 0.0" << endl;
            file << "  outer loop" << endl;
            for (int i = 0; i < 3; ++i)
            {
                auto& vertex = mesh.vertices[f.indices[i]];
                file << "    vertex " << vertex.x << " " << vertex.y << " " << vertex.z << endl;
            }
            file << "  endloop" << endl;
            file << "endfacet" << endl;

            file << "facet normal 0.0 0.0 0.0" << endl;
            file << "  outer loop" << endl;
            for (int i = 2; i != 1;)
            {
                i = i % 4;
                auto& vertex = mesh.vertices[f.indices[i]];
                file << "    vertex " << vertex.x << " " << vertex.y << " " << vertex.z << endl;
                ++i;
            }
            file << "  endloop" << endl;
            file << "endfacet" << endl;
        }
    }
    file << "endsolid doodle" << endl;
    file.close();
}

/*************/
int main(int argc, char** argv)
{
    string filename{""};
    int resolution{128};
    float height{1.f};

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
        else if (i < argc - 1 && (string(argv[i]) == "-h" || string(argv[i]) == "--height"))
        {
            height = stof(string(argv[i + 1]));
            if (height <= 1.f)
                height = 1.f;
            i += 2;
        }
        else
        {
            ++i;
        }
    }

    // Load the image, and resize it
    auto image = cv::imread(filename, cv::IMREAD_GRAYSCALE);
    cv::Mat resized;
    cv::resize(image, resized, cv::Size(2048, (2048 * image.rows) / image.cols));
    image = resized;

    // Apply some filters on it
    filterImage(image);

    // Save the image for debug
    cv::imwrite("debug.png", image);

    // Create the base 2D mesh from the binary image
    Mesh mesh2D;
    auto nbrFaces = binaryToMesh(image, mesh2D, resolution, height);
    cout << "Created " << nbrFaces << " faces" << endl;
    auto successWrite = writeSTL("test.stl", mesh2D);

    return 0;
}
