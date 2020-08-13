/*===============================================================================
Copyright (c) 2020, PTC Inc. All rights reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other
countries.
===============================================================================*/

#include "Modelv3d.h"

#include "Log.h"


Modelv3d::Modelv3d(const std::vector<unsigned char>& data)
    : mTransparencyValue(1)
{
    mLightColor = new float[4]{ .5f, .5f, .5f, 1.0f };

    // Parse the data
    unsigned int location = 0; // current index in the data
    int ignoreInt;
    float ignoreFloat;

    static_assert(sizeof(int) == 4, "Modelv3d loading assumes integers are 4 bytes");
    static_assert(sizeof(float) == 4, "Modelv3d loading assumes floats are 4 bytes");

    unsigned int magicNumber = readUint(data, location);
    LOG("Modelv3d loader: magicNumber: %4x", magicNumber);

    float version = readFloat(data, location);
    LOG("Modelv3d loader: version: %7.5f", version);

    // Read vertices number
    mNumVertices = readUint(data, location);
    LOG("Modelv3d loader: nbVertices: %d", mNumVertices);

    // Read faces number
    mNumFaces = readUint(data, location);
    LOG("Modelv3d loader: nbFaces: %d", mNumFaces);

    // Read material number
    mNumMaterials = readUint(data, location);
    LOG("Modelv3d loader: nbMaterials: %d", mNumMaterials);
    mNumGroups = mNumMaterials;

    // Read vertices
    int numFloatsToRead = mNumFaces * 3 * 3; // 3 vertices per face, 3 values per vertex x, y, z
    mVertices = new float[numFloatsToRead];

    for (int i = 0; i < numFloatsToRead; ++i)
    {
        mVertices[i] = readFloat(data, location);
    }
    LOG("Modelv3d loader: First vertex (of %d): %12.6f %12.6f %12.6f", numFloatsToRead, mVertices[0], mVertices[1], mVertices[2]);

    // Read normals
    numFloatsToRead = mNumFaces * 3 * 3; // 3 vertices per face, 3 values per vertex x, y, z
    mNormals = new float[numFloatsToRead];
    for (int i = 0; i < numFloatsToRead; ++i)
    {
        mNormals[i] = readFloat(data, location);
    }
    LOG("Modelv3d loader: First normal (of %d): %12.6f %12.6f %12.6f", numFloatsToRead, mNormals[0], mNormals[1], mNormals[2]);

    // Read texture coordinates
    numFloatsToRead = mNumFaces * 3 * 2; // 3 vertices per face, 2 values per vertex u, v
    mTextureCoordinates = new float[numFloatsToRead];
    for (int i = 0; i < numFloatsToRead; ++i)
    {
        mTextureCoordinates[i] = readFloat(data, location);
    }
    LOG("Modelv3d loader: First texture coordinate (of %d): %12.6f %12.6f", numFloatsToRead, mTextureCoordinates[0], mTextureCoordinates[1]);

    // Read material per face and shininess
    numFloatsToRead = mNumFaces * 3 * 2; // 3 vertices per face, 2 values per vertex material, shininess
    mMaterialIndices = new float[numFloatsToRead];
    for (int i = 0; i < numFloatsToRead; ++i)
    {
        mMaterialIndices[i] = readFloat(data, location);
    }
    LOG("Modelv3d loader: First material and shininess: %12.6f %12.6f", mMaterialIndices[0], mMaterialIndices[1]);

    // Read material ambient color
    numFloatsToRead = mNumMaterials * 4; // 4 values per material r, g, b, a
    mGroupAmbientColors = new float[numFloatsToRead];
    for (int i = 0; i < numFloatsToRead; ++i)
    {
        mGroupAmbientColors[i] = readFloat(data, location);
    }
    LOG("Modelv3d loader: First ambient color: %12.6f %12.6f %12.6f %12.6f", mGroupAmbientColors[0], mGroupAmbientColors[1], mGroupAmbientColors[2], mGroupAmbientColors[3]);

    // Read material diffuse color
    numFloatsToRead = mNumMaterials * 4; // 4 values per material r, g, b, a
    mGroupDiffuseColors = new float[numFloatsToRead];
    for (int i = 0; i < numFloatsToRead; ++i)
    {
        mGroupDiffuseColors[i] = readFloat(data, location);
    }

    // Read material specular color
    numFloatsToRead = mNumMaterials * 4; // 4 values per material r, g, b, a
    mGroupSpecularColors = new float[numFloatsToRead];
    for (int i = 0; i < numFloatsToRead; ++i)
    {
        mGroupSpecularColors[i] = readFloat(data, location);
    }
    LOG("Modelv3d loader: First specular color: %12.6f %12.6f %12.6f %12.6f", mGroupSpecularColors[0], mGroupSpecularColors[1], mGroupSpecularColors[2], mGroupSpecularColors[3]);

    // Read material diffuse texture indexes (ignored)
    numFloatsToRead = mNumMaterials; // 1 index per material
    for (int i = 0; i < numFloatsToRead; ++i)
    {
        ignoreInt = readInt(data, location);
    }

    // Read material dissolve value (transparency) -- IGNORED
    numFloatsToRead = mNumMaterials; // 1 value per material
    for (int i = 0; i < numFloatsToRead; ++i)
    {
        ignoreFloat = readFloat(data, location);
    }

    // Read vertex range per group
    numFloatsToRead = mNumMaterials * 2; // 2 values per material
    mGroupVertexRange = new int[numFloatsToRead];
    for (int i = 0; i < numFloatsToRead; ++i)
    {
        mGroupVertexRange[i] = readInt(data, location);
    }
    LOG("Modelv3d loader: First material diffuse texture index:%d , %d", mGroupVertexRange[0], mGroupVertexRange[1]);

    unsigned int magicNumberEnd = readUint(data, location);
    LOG("Modelv3d loader: magicNumber (end): %4x", magicNumberEnd);

    if (magicNumber != magicNumberEnd)
    {
        // sanity check to see if we read properly the magic number at the end of the file
        LOG("Modelv3d loader: Error while reading the v3d data");
        clearData();
        mIsLoaded = false;
    }
    else
    {
        mIsLoaded = true;
    }
}


Modelv3d::~Modelv3d()
{
    delete[] mLightColor;
    clearData();
}


void Modelv3d::clearData()
{
    mIsLoaded = false;

    mNumVertices = 0;
    mNumFaces = 0;
    mNumGroups = 0;
    mNumMaterials = 0;

    delete[] mVertices;
    mVertices = nullptr;
    delete[] mNormals;
    mNormals = nullptr;
    delete[] mTextureCoordinates;
    mTextureCoordinates = nullptr;
    delete[] mMaterialIndices;
    mMaterialIndices = nullptr;
    delete[] mGroupAmbientColors;
    mGroupAmbientColors = nullptr;
    delete[] mGroupDiffuseColors;
    mGroupDiffuseColors = nullptr;
    delete[] mGroupSpecularColors;
    mGroupSpecularColors = nullptr;
    delete[] mGroupVertexRange;
    mGroupVertexRange = nullptr;
}


int Modelv3d::readInt(const std::vector<unsigned char>& data, unsigned int& location)
{
    int result;
    // Reverse byte order
    unsigned char reversed[] = { data[location + 3], data[location + 2], data[location + 1], data[location] };
    // Copy to result
    std::copy(reinterpret_cast<const char*>(&reversed[0]),
        reinterpret_cast<const char*>(&reversed[4]),
        reinterpret_cast<char*>(&result));
    location += 4;
    return result;
}


unsigned int Modelv3d::readUint(const std::vector<unsigned char>& data, unsigned int& location)
{
    unsigned int result;
    // Reverse byte order
    unsigned char reversed[] = { data[location + 3], data[location + 2], data[location + 1], data[location] };
    // Copy to result
    std::copy(reinterpret_cast<const char*>(&reversed[0]),
        reinterpret_cast<const char*>(&reversed[4]),
        reinterpret_cast<char*>(&result));
    location += 4;
    return result;
}


float Modelv3d::readFloat(const std::vector<unsigned char>& data, unsigned int& location)
{
    float result;
    // Reverse byte order
    unsigned char reversed[] = { data[location + 3], data[location + 2], data[location + 1], data[location] };
    // Copy to result
    std::copy(reinterpret_cast<const char*>(&reversed[0]),
        reinterpret_cast<const char*>(&reversed[4]),
        reinterpret_cast<char*>(&result));
    location += 4;
    return result;
}
