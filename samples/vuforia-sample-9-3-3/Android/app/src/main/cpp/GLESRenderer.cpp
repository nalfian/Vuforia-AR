/*===============================================================================
Copyright (c) 2020 PTC Inc. All Rights Reserved.

Vuforia is a trademark of PTC Inc., registered in the United States and other
countries.
===============================================================================*/

#include "GLESRenderer.h"

#include "GLESUtils.h"
#include "Shaders.h"

#include <MathUtils.h>
#include <Models.h>
#include <Vuforia/Tool.h>

#include <android/asset_manager.h>

bool GLESRenderer::init(AAssetManager* assetManager)
{
    // Setup for Video Background rendering
    mVbShaderProgramID =
        GLESUtils::createProgramFromBuffer(textureVertexShaderSrc, textureFragmentShaderSrc);
    mVbVertexPositionHandle =
        glGetAttribLocation(mVbShaderProgramID, "vertexPosition");
    mVbTextureCoordHandle =
        glGetAttribLocation(mVbShaderProgramID, "vertexTextureCoord");
    mVbMvpMatrixHandle =
        glGetUniformLocation(mVbShaderProgramID, "modelViewProjectionMatrix");
    mVbTexSampler2DHandle =
        glGetUniformLocation(mVbShaderProgramID, "texSampler2D");

    // Setup for augmentation rendering
    mUniformColorShaderProgramID =
        GLESUtils::createProgramFromBuffer(uniformColorVertexShaderSrc, uniformColorFragmentShaderSrc);
    mUniformColorVertexPositionHandle =
        glGetAttribLocation(mUniformColorShaderProgramID, "vertexPosition");
    mUniformColorMvpMatrixHandle =
        glGetUniformLocation(mUniformColorShaderProgramID, "modelViewProjectionMatrix");
    mUniformColorColorHandle =
        glGetUniformLocation(mUniformColorShaderProgramID, "uniformColor");

    // Setup for guide view rendering
    mTextureUniformColorShaderProgramID =
        GLESUtils::createProgramFromBuffer(textureColorVertexShaderSrc, textureColorFragmentShaderSrc);
    mTextureUniformColorVertexPositionHandle =
        glGetAttribLocation(mTextureUniformColorShaderProgramID, "vertexPosition");
    mTextureUniformColorTextureCoordHandle =
        glGetAttribLocation(mTextureUniformColorShaderProgramID, "vertexTextureCoord");
    mTextureUniformColorMvpMatrixHandle =
        glGetUniformLocation(mTextureUniformColorShaderProgramID, "modelViewProjectionMatrix");
    mTextureUniformColorTexSampler2DHandle =
        glGetUniformLocation(mTextureUniformColorShaderProgramID, "texSampler2D");
    mTextureUniformColorColorHandle =
        glGetUniformLocation(mTextureUniformColorShaderProgramID, "uniformColor");

    // Setup for axis rendering
    mVertexColorShaderProgramID =
        GLESUtils::createProgramFromBuffer(vertexColorVertexShaderSrc, vertexColorFragmentShaderSrc);
    mVertexColorVertexPositionHandle
        = glGetAttribLocation(mVertexColorShaderProgramID, "vertexPosition");
    mVertexColorColorHandle
        = glGetAttribLocation(mVertexColorShaderProgramID, "vertexColor");
    mVertexColorMvpMatrixHandle
        = glGetUniformLocation(mVertexColorShaderProgramID, "modelViewProjectionMatrix");

    mModelTargetGuideViewTextureUnit = -1;

    std::vector<unsigned char> data; // for reading model files

    // Load Astronaut model
    if (!readAsset(assetManager, "astronaut.v3d", data))
    {
        return false;
    }
    mAstronautModel = std::make_unique<Modelv3d>(data);
    if (!mAstronautModel->isLoaded())
    {
        return false;
    }
    data.clear();
    mAstronautTextureUnit = -1;

    // Load Lander model
    if (!readAsset(assetManager, "lander.v3d", data))
    {
        return false;
    }
    mLanderModel = std::make_unique<Modelv3d>(data);
    if (!mLanderModel->isLoaded())
    {
        return false;
    }
    data.clear();
    mLanderTextureUnit = -1;

    return true;
}


void GLESRenderer::deinit()
{
    if (mModelTargetGuideViewTextureUnit != -1)
    {
        GLESUtils::destroyTexture(mModelTargetGuideViewTextureUnit);
        mModelTargetGuideViewTextureUnit = -1;
    }
    if (mAstronautTextureUnit != -1)
    {
        GLESUtils::destroyTexture(mAstronautTextureUnit);
        mAstronautTextureUnit = -1;
    }
    if (mLanderTextureUnit != -1)
    {
        GLESUtils::destroyTexture(mLanderTextureUnit);
        mLanderTextureUnit = -1;
    }
}


void GLESRenderer::setAstronautTexture(int width, int height, unsigned char* bytes)
{
    createTexture(width, height, bytes, mAstronautTextureUnit);
}


void GLESRenderer::setLanderTexture(int width, int height, unsigned char* bytes)
{
    createTexture(width, height, bytes, mLanderTextureUnit);
}


void GLESRenderer::renderVideoBackground(
    Vuforia::Matrix44F& projectionMatrix,
    const float* vertices, const float* textureCoordinates,
    const int numTriangles, const unsigned short* indices,
    int textureUnit)
{
    GLboolean depthTest = GL_FALSE;
    GLboolean cullTest = GL_FALSE;

    glGetBooleanv(GL_DEPTH_TEST, &depthTest);
    glGetBooleanv(GL_CULL_FACE, &cullTest);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    // Load the shader and upload the vertex/texcoord/index data
    glUseProgram(mVbShaderProgramID);
    glVertexAttribPointer(static_cast<GLuint>(mVbVertexPositionHandle), 3, GL_FLOAT,
                          GL_FALSE, 0, vertices);
    glVertexAttribPointer(static_cast<GLuint>(mVbTextureCoordHandle), 2, GL_FLOAT,
                          GL_FALSE, 0, textureCoordinates);

    glUniform1i(mVbTexSampler2DHandle, textureUnit);

    // Render the video background with the custom shader
    // First, we enable the vertex arrays
    glEnableVertexAttribArray(static_cast<GLuint>(mVbVertexPositionHandle));
    glEnableVertexAttribArray(static_cast<GLuint>(mVbTextureCoordHandle));

    // Pass the projection matrix to OpenGL
    glUniformMatrix4fv(mVbMvpMatrixHandle, 1, GL_FALSE, projectionMatrix.data);

    // Then, we issue the render call
    glDrawElements(GL_TRIANGLES, numTriangles * 3, GL_UNSIGNED_SHORT,
                   indices);

    // Finally, we disable the vertex arrays
    glDisableVertexAttribArray(static_cast<GLuint>(mVbVertexPositionHandle));
    glDisableVertexAttribArray(static_cast<GLuint>(mVbTextureCoordHandle));

    if(depthTest)
        glEnable(GL_DEPTH_TEST);

    if(cullTest)
        glEnable(GL_CULL_FACE);

    GLESUtils::checkGlError("Render video background");
}


void GLESRenderer::renderWorldOrigin(Vuforia::Matrix44F& projectionMatrix, Vuforia::Matrix44F& modelViewMatrix)
{
    Vuforia::Vec3F axis10cmSize = Vuforia::Vec3F(0.1f, 0.1f, 0.1f);
    renderAxis(projectionMatrix, modelViewMatrix, axis10cmSize, 4.0f);
    Vuforia::Vec4F cubeColor(0.8, 0.8, 0.8, 1.0);
    renderCube(projectionMatrix, modelViewMatrix, 0.015f, cubeColor);
}


void GLESRenderer::renderImageTarget(Vuforia::Matrix44F& projectionMatrix,
                                     Vuforia::Matrix44F& modelViewMatrix,
                                     Vuforia::Matrix44F& scaledModelViewMatrix)
{
    Vuforia::Matrix44F scaledModelViewProjectionMatrix;
    MathUtils::multiplyMatrix(projectionMatrix, scaledModelViewMatrix, scaledModelViewProjectionMatrix);


    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float stateLineWidth;
    glGetFloatv(GL_LINE_WIDTH, &stateLineWidth);

    glUseProgram(mUniformColorShaderProgramID);

    glVertexAttribPointer(mUniformColorVertexPositionHandle, 3, GL_FLOAT, GL_TRUE, 0,
                          (const GLvoid *) &squareVertices[0]);

    glEnableVertexAttribArray(mUniformColorVertexPositionHandle);

    glUniformMatrix4fv(mUniformColorMvpMatrixHandle, 1, GL_FALSE,
                       &scaledModelViewProjectionMatrix.data[0]);

    // Draw translucent solid overlay
    // Color RGBA
    glUniform4f(mUniformColorColorHandle, 1.0, 0.0, 0.0, 0.1);
    glDrawElements(GL_TRIANGLES, NUM_SQUARE_INDEX, GL_UNSIGNED_SHORT,
                   (const GLvoid *) &squareIndices[0]);

    // Draw solid outline
    glUniform4f(mUniformColorColorHandle, 1.0, 0.0, 0.0, 1.0);
    glLineWidth(4.0f);
    glDrawElements(GL_LINES, NUM_SQUARE_WIREFRAME_INDEX, GL_UNSIGNED_SHORT,
                   (const GLvoid *) &squareWireframeIndices[0]);

    glDisableVertexAttribArray(mUniformColorVertexPositionHandle);

    GLESUtils::checkGlError("Render Image Target");

    glLineWidth(stateLineWidth);

    glDisable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    Vuforia::Vec3F axis2cmSize = Vuforia::Vec3F(0.02f, 0.02f, 0.02f);
    renderAxis(projectionMatrix, modelViewMatrix, axis2cmSize, 4.0f);

    Vuforia::Matrix44F modelViewProjectionMatrix;
    Vuforia::Matrix44F adjustedModelViewMatrix;
    adjustedModelViewMatrix = MathUtils::Matrix44FRotate(90, { 1.0f, 0.f, 0.f }, modelViewMatrix); // Stand up
    MathUtils::translateMatrix({ -0.03f, 0, -0.02f }, adjustedModelViewMatrix); // Move to center
    MathUtils::multiplyMatrix(projectionMatrix, adjustedModelViewMatrix, modelViewProjectionMatrix);
    renderModel(modelViewProjectionMatrix,
        mAstronautModel->getNumVertices(), mAstronautModel->getVertices(), mAstronautModel->getTextureCoordinates(),
        mAstronautTextureUnit);
}


void GLESRenderer::renderModelTarget(Vuforia::Matrix44F& projectionMatrix,
                                     Vuforia::Matrix44F& modelViewMatrix,
                                     Vuforia::Matrix44F& /*scaledModelViewMatrix*/)
{
    Vuforia::Matrix44F modelViewProjectionMatrix;
    MathUtils::multiplyMatrix(projectionMatrix, modelViewMatrix, modelViewProjectionMatrix);

    renderModel(modelViewProjectionMatrix,
        mLanderModel->getNumVertices(), mLanderModel->getVertices(), mLanderModel->getTextureCoordinates(),
        mLanderTextureUnit);

    Vuforia::Vec3F axis10cmSize = Vuforia::Vec3F(0.1f, 0.1f, 0.1f);
    renderAxis(projectionMatrix, modelViewMatrix, axis10cmSize, 4.0f);
}


void GLESRenderer::renderModelTargetGuideView(Vuforia::Matrix44F& projectionMatrix,
                                              Vuforia::Matrix44F& modelViewMatrix,
                                              const Vuforia::Image *image)
{
    Vuforia::Matrix44F modelViewProjectionMatrix;
    MathUtils::multiplyMatrix(projectionMatrix, modelViewMatrix, modelViewProjectionMatrix);


    glDisable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glActiveTexture(GL_TEXTURE0);

    if (mModelTargetGuideViewTextureUnit == -1)
    {
        mModelTargetGuideViewTextureUnit = GLESUtils::createTexture(image);
    }
    glBindTexture(GL_TEXTURE_2D, mModelTargetGuideViewTextureUnit);

    glEnableVertexAttribArray(mTextureUniformColorVertexPositionHandle);
    glVertexAttribPointer(mTextureUniformColorVertexPositionHandle, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)&squareVertices[0]);

    glEnableVertexAttribArray(mTextureUniformColorTextureCoordHandle);
    glVertexAttribPointer(mTextureUniformColorTextureCoordHandle, 2, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)&squareTexCoords[0]);

    glUseProgram(mTextureUniformColorShaderProgramID);
    glUniformMatrix4fv(mTextureUniformColorMvpMatrixHandle, 1, GL_FALSE, (GLfloat*)modelViewProjectionMatrix.data);
    glUniform4f(mTextureUniformColorColorHandle, 1.0f, 1.0f, 1.0f, 0.7f);
    glUniform1i(mTextureUniformColorTexSampler2DHandle, 0); //texture unit, not handle

    // Draw
    glDrawElements(GL_TRIANGLES, NUM_SQUARE_INDEX, GL_UNSIGNED_SHORT, (const GLvoid*)&squareIndices[0]);

    //disable input data structures
    glDisableVertexAttribArray(mTextureUniformColorTextureCoordHandle);
    glDisableVertexAttribArray(mTextureUniformColorVertexPositionHandle);
    glUseProgram(0);

    glBindTexture(GL_TEXTURE_2D, 0);

    GLESUtils::checkGlError("Render guide view");

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}


void GLESRenderer::createTexture(int width, int height, unsigned char* bytes, int& textureId)
{
    if (textureId != -1)
    {
        GLESUtils::destroyTexture(textureId);
        textureId = -1;
    }
    textureId = GLESUtils::createTexture(width, height, bytes);
}


void GLESRenderer::renderCube(const Vuforia::Matrix44F& projectionMatrix, const Vuforia::Matrix44F& modelViewMatrix,
                              float scale, const Vuforia::Vec4F& color)
{
    Vuforia::Matrix44F scaledModelViewMatrix;
    Vuforia::Matrix44F modelViewProjectionMatrix;
    Vuforia::Vec3F scaleVec(scale, scale, scale);

    scaledModelViewMatrix = MathUtils::Matrix44FScale(scaleVec, modelViewMatrix);
    MathUtils::multiplyMatrix(projectionMatrix, scaledModelViewMatrix, modelViewProjectionMatrix);

    ///////////////////////////////////////////////////////////////
    // Render with const ambient diffuse light uniform color shader
    glEnable(GL_DEPTH_TEST);
    glUseProgram(mUniformColorShaderProgramID);

    glEnableVertexAttribArray(mUniformColorVertexPositionHandle);

    glVertexAttribPointer(mUniformColorVertexPositionHandle, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)&cubeVertices[0]);

    glUniformMatrix4fv(mUniformColorMvpMatrixHandle, 1, GL_FALSE, (GLfloat*)modelViewProjectionMatrix.data);
    glUniform4f(mUniformColorColorHandle, color.data[0], color.data[1], color.data[2], color.data[3]);

    // Draw
    glDrawElements(GL_TRIANGLES, NUM_CUBE_INDEX, GL_UNSIGNED_SHORT, (const GLvoid*)&cubeIndices[0]);

    //disable input data structures
    glDisableVertexAttribArray(mUniformColorVertexPositionHandle);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);

    GLESUtils::checkGlError("Render cube");
    ///////////////////////////////////////////////////////
}


void GLESRenderer::renderAxis(const Vuforia::Matrix44F& projectionMatrix, const Vuforia::Matrix44F& modelViewMatrix,
                              const Vuforia::Vec3F& scale,
                              float lineWidth)
{
    Vuforia::Matrix44F scaledModelViewMatrix;
    Vuforia::Matrix44F modelViewProjectionMatrix;

    scaledModelViewMatrix = MathUtils::Matrix44FScale(scale, modelViewMatrix);
    MathUtils::multiplyMatrix(projectionMatrix, scaledModelViewMatrix, modelViewProjectionMatrix);

    ///////////////////////////////////////////////////////
    // Render with vertex color shader
    glEnable(GL_DEPTH_TEST);
    glUseProgram(mVertexColorShaderProgramID);

    glEnableVertexAttribArray(mVertexColorVertexPositionHandle);
    glVertexAttribPointer(mVertexColorVertexPositionHandle, 3, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)&axisVertices[0]);

    glEnableVertexAttribArray(mVertexColorColorHandle);
    glVertexAttribPointer(mVertexColorColorHandle, 4, GL_FLOAT, GL_FALSE, 0, (const GLvoid*)&axisColors[0]);

    glUniformMatrix4fv(mVertexColorMvpMatrixHandle, 1, GL_FALSE, (GLfloat*)modelViewProjectionMatrix.data);

    // Draw
    float stateLineWidth;
    glGetFloatv(GL_LINE_WIDTH, &stateLineWidth);

    glLineWidth(lineWidth);

    glDrawElements(GL_LINES, NUM_AXIS_INDEX, GL_UNSIGNED_SHORT, (const GLvoid*)&axisIndices[0]);

    //disable input data structures
    glDisableVertexAttribArray(mVertexColorVertexPositionHandle);
    glDisableVertexAttribArray(mVertexColorColorHandle);
    glUseProgram(0);
    glDisable(GL_DEPTH_TEST);

    glLineWidth(stateLineWidth);

    GLESUtils::checkGlError("Render axis");
    ///////////////////////////////////////////////////////
}


void GLESRenderer::renderModel(Vuforia::Matrix44F modelViewProjectionMatrix,
    const int numVertices, const float* vertices, const float* textureCoordinates,
    GLint textureId)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glUseProgram(mTextureUniformColorShaderProgramID);

    glEnableVertexAttribArray(mTextureUniformColorVertexPositionHandle);
    glVertexAttribPointer(mTextureUniformColorVertexPositionHandle, 3, GL_FLOAT, GL_FALSE, 0,
                          (const GLvoid *) vertices);

    glEnableVertexAttribArray(mTextureUniformColorTextureCoordHandle);
    glVertexAttribPointer(mTextureUniformColorTextureCoordHandle, 2, GL_FLOAT, GL_FALSE, 0,
                          (const GLvoid *) textureCoordinates);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glUniformMatrix4fv(mTextureUniformColorMvpMatrixHandle, 1, GL_FALSE,
                       (GLfloat *) modelViewProjectionMatrix.data);
    glUniform4f(mTextureUniformColorColorHandle, 1.0f, 1.0f, 1.0f, 1.0f);
    glUniform1i(mTextureUniformColorTexSampler2DHandle, 0); //texture unit, not handle

    // Draw
    glDrawArrays(GL_TRIANGLES, 0, numVertices);

    //disable input data structures
    glDisableVertexAttribArray(mTextureUniformColorTextureCoordHandle);
    glDisableVertexAttribArray(mTextureUniformColorVertexPositionHandle);
    glUseProgram(0);

    glBindTexture(GL_TEXTURE_2D, 0);

    GLESUtils::checkGlError("Render model");

    glDisable(GL_BLEND);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
}


bool GLESRenderer::readAsset(AAssetManager* assetManager, const char* filename, std::vector<unsigned char>& data)
{
    LOG("Reading asset %s", filename);
    AAsset* asset = AAssetManager_open(assetManager, filename, AASSET_MODE_STREAMING);
    if (asset == nullptr)
    {
        LOG("Error opening asset file %s", filename);
        return false;
    }
    auto assetSize = AAsset_getLength(asset);
    data.reserve(assetSize);
    char buf[BUFSIZ];
    int nb_read = 0;
    while ((nb_read = AAsset_read(asset, buf, BUFSIZ)) > 0)
    {
        std::copy(&buf[0], &buf[BUFSIZ], std::back_inserter(data));
    }
    AAsset_close(asset);
    if (nb_read < 0)
    {
        LOG("Error reading asset file %s", filename);
        return false;
    }
    return true;
}

