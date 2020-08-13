/*===============================================================================
Copyright (c) 2020, PTC Inc. All rights reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other
countries.
===============================================================================*/

#ifndef __MODELV3D_H__
#define __MODELV3D_H__

#include <vector>


/// Class for loading v3d model files
/// v3d is a proprietary binary format used to minimize the size of the
/// model files. For developers wishing to create their own models we
// recommend using OBJ format and a suitable open-source parser.
class Modelv3d
{
public:
    Modelv3d(const std::vector<unsigned char>& data);
    virtual ~Modelv3d();

    bool isLoaded() const { return mIsLoaded; }

    const int getNumFaces() const { return mNumFaces; }
    const int getNumVertices() const { return mNumVertices; }
    const float* getVertices() const { return mVertices; }

    const float* getTextureCoordinates() const { return mTextureCoordinates; }

private: // methods
    void clearData();
    static unsigned int readUint(const std::vector<unsigned char>& data, unsigned int& location);
    static int readInt(const std::vector<unsigned char>& data, unsigned int& location);
    static float readFloat(const std::vector<unsigned char>& data, unsigned int& location);

private: // data members
    bool mIsLoaded = false;

    unsigned int mNumVertices{ 0 };
    unsigned int mNumFaces{ 0 };
    unsigned int mNumGroups{ 0 };
    unsigned int mNumMaterials{ 0 };

    float* mVertices{ nullptr };
    float* mNormals{ nullptr };
    float* mTextureCoordinates{ nullptr };
    float* mMaterialIndices{ nullptr };
    float* mGroupAmbientColors{ nullptr };
    float* mGroupDiffuseColors{ nullptr };
    float* mGroupSpecularColors{ nullptr };
    int* mGroupVertexRange{ nullptr };

    float mTransparencyValue{ 0 };
    float* mLightColor{ nullptr };

};

#endif // __MODELV3D_H__
