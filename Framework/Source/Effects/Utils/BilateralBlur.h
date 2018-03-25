#pragma once
#include "API/FBO.h"
#include "Graphics/FullScreenPass.h"
#include "API/Sampler.h"
#include "Graphics/Program/ProgramVars.h"
#include <memory>

namespace Falcor
{
    class RenderContext;
    class Texture;
    class Fbo;
    class Gui;

    class BilateralBlur
    {
    public:
        using UniquePtr = std::unique_ptr<BilateralBlur>;

        ~BilateralBlur();

        /** Create a new object
            \param[in] kernelSize Number of samples taken along each axis
            \param[in] spatialSigma Gaussian distribution sigma value used to calculate sample weights based on the spatial distance to the center position.
            \param[in] rangeSigma Gaussian distribution sigma value used to calculate sample weights based on the difference in pixel values.
        */
        static UniquePtr create(uint32_t kernelWidth = 5, float spatialSigma = 2.5f, float rangeSigma = 0.2f);

        /** Apply bilateral blur by rendering one texture into another.
            \param pRenderContext Render context to use
            \param pSrc The source texture
            \param pDst The destination texture
        */
        void execute(RenderContext* pRenderContext, Texture::SharedPtr pSrc, Fbo::SharedPtr pDst);

        /** Set the kernel width. Controls the number of texels which will be sampled when blurring each pixel.
            Values smaller than twice the spatial sigma are ineffective.
        */
        void setKernelWidth(uint32_t kernelWidth);

        uint32_t getKernelWidth() const { return mKernelWidth; }

        /** Set the spatial sigma. Higher values will result in a blurrier image.
            Values greater than half the kernel width are ineffective.
        */
        void setSpatialSigma(float sigma) { mSpatialSigma = sigma; mDirty = true; }

        float getSpatialSigma() const { return mSpatialSigma; }

        /** Set the range sigma. Higher values will result in more Gaussian-like blur.
        */
        void setRangeSigma(float sigma) { mRangeSigma = sigma; mDirty = true; }

        float getRangeSigma() const { return mRangeSigma; }

        /** Render UI controls for blur settings.
            \param[in] pGui GUI instance to render UI elements with
            \param[in] uiGroup Optional name. If specified, UI elements will be rendered within a named group
        */
        void renderUI(Gui* pGui, const char* uiGroup = nullptr);

    private:
        BilateralBlur(uint32_t kernelWidth, float spatialSigma, float rangeSigma);
        uint32_t mKernelWidth;
        float mSpatialSigma;
        float mRangeSigma;
        uint32_t vpMask;
        void createProgram();
        void updateKernel();

        FullScreenPass::UniquePtr mpBlur;
        Sampler::SharedPtr mpSampler;
        bool mDirty = true;
        GraphicsVars::SharedPtr mpVars;

        struct
        {
            ParameterBlockReflection::BindLocation sampler;
            ParameterBlockReflection::BindLocation srcTexture;
        } mBindLocations;
    };
}
