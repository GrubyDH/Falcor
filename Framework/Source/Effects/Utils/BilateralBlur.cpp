#include "Framework.h"
#include "BilateralBlur.h"
#include "API/RenderContext.h"
#include "Graphics/FboHelper.h"
#include "Utils/Gui.h"
#define _USE_MATH_DEFINES
#include <math.h>

namespace Falcor
{
    static std::string kShaderFilename(appendShaderExtension("Effects/BilateralBlur.ps"));

    BilateralBlur::~BilateralBlur() = default;

    BilateralBlur::BilateralBlur(uint32_t kernelWidth, float spatialSigma, float rangeSigma) : mKernelWidth(kernelWidth), mSpatialSigma(spatialSigma), mRangeSigma(rangeSigma)
    {
        Sampler::Desc samplerDesc;
        samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Point).setAddressingMode(Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp, Sampler::AddressMode::Clamp);
        mpSampler = Sampler::create(samplerDesc);
    }

    BilateralBlur::UniquePtr BilateralBlur::create(uint32_t kernelWidth, float spatialSigma, float rangeSigma)
    {
        BilateralBlur* pBlur = new BilateralBlur(kernelWidth, spatialSigma, rangeSigma);
        return BilateralBlur::UniquePtr(pBlur);
    }

    void BilateralBlur::renderUI(Gui* pGui, const char* uiGroup)
    {
        if (uiGroup == nullptr || pGui->beginGroup(uiGroup))
        {
            if (pGui->addIntVar("Kernel Width", (int&)mKernelWidth, 1, 15, 2))
            {
                setKernelWidth(mKernelWidth);
            }
            if (pGui->addFloatVar("Spatial Sigma", mSpatialSigma, 0.001f))
            {
                setSpatialSigma(mSpatialSigma);
            }
            if (pGui->addFloatVar("Range Sigma", mRangeSigma, 0.001f))
            {
                setRangeSigma(mRangeSigma);
            }
            if (uiGroup) pGui->endGroup();
        }
    }

    void BilateralBlur::setKernelWidth(uint32_t kernelWidth)
    {
        mKernelWidth = kernelWidth | 1; // Make sure the kernel width is an odd number
        mDirty = true; 
    }

    float getCoefficient(float sigma, float x)
    {
        float sigmaSquared = sigma * sigma;
        float p = -(x*x) / (2 * sigmaSquared);
        float e = exp(p);

        float a = 2 * (float)M_PI * sigmaSquared;
        return e / a;
    }

    void BilateralBlur::updateKernel()
    {
        std::vector<std::vector<float> > weights(mKernelWidth, std::vector<float>(mKernelWidth));
        float sum = 0.0f;

        glm::vec2 centerPos(static_cast<float>(mKernelWidth / 2), static_cast<float>(mKernelWidth / 2));
        // TODO: This is symmetric array in both directions. Only top-left corner could be computed.
        for (uint32_t i = 0; i < mKernelWidth; ++i)
        {
            for (uint32_t j = 0; j < mKernelWidth; ++j)
            {
                glm::vec2 samplePos(static_cast<float>(i), static_cast<float>(j));
                weights[i][j] = getCoefficient(mSpatialSigma, glm::distance(centerPos, samplePos));
                sum += weights[i][j];
            }
        }

        TypedBuffer<float>::SharedPtr pBuf = TypedBuffer<float>::create(mKernelWidth * mKernelWidth, Resource::BindFlags::ShaderResource);

        for (uint32_t i = 0; i < mKernelWidth; ++i)
        {
            for (uint32_t j = 0; j < mKernelWidth; ++j)
            {
                float w = weights[i][j] / sum;
                uint32_t linIdx = i * mKernelWidth + j;
                pBuf[linIdx] = w;
            }
        }

        mpVars->setTypedBuffer("weights", pBuf);
    }

    void BilateralBlur::createProgram()
    {
        Program::DefineList defines;
        defines.add("_KERNEL_WIDTH", std::to_string(mKernelWidth));
        defines.add("_RANGE_SIGMA_SQUARED", std::to_string(mRangeSigma * mRangeSigma));

        mpBlur = FullScreenPass::create(kShaderFilename, defines, true, true);

        ProgramReflection::SharedConstPtr pReflector = mpBlur->getProgram()->getActiveVersion()->getReflector();
        mpVars = GraphicsVars::create(pReflector);

        mBindLocations.sampler = pReflector->getDefaultParameterBlock()->getResourceBinding("gSampler");
        mBindLocations.srcTexture = pReflector->getDefaultParameterBlock()->getResourceBinding("gSrcTex");

        updateKernel();
        mDirty = false;
    }

    void BilateralBlur::execute(RenderContext* pRenderContext, Texture::SharedPtr pSrc, Fbo::SharedPtr pDst)
    {
        if (mDirty)
        {
            createProgram();
        }

        uint32_t arraySize = pSrc->getArraySize();
        GraphicsState::Viewport vp;
        vp.originX = 0;
        vp.originY = 0;
        vp.height = (float)pDst->getHeight();
        vp.width = (float)pDst->getWidth();
        vp.minDepth = 0;
        vp.maxDepth = 1;

        GraphicsState* pState = pRenderContext->getGraphicsState().get();
        for(uint32_t i = 0; i < arraySize; i++)
        {
            pState->pushViewport(i, vp);
        }

        // Blur pass
        ParameterBlock* pDefaultBlock = mpVars->getDefaultBlock().get();
        pDefaultBlock->setSampler(mBindLocations.sampler, 0, mpSampler);
        pDefaultBlock->setSrv(mBindLocations.srcTexture, 0, pSrc->getSRV());

        pState->pushFbo(pDst);
        pRenderContext->pushGraphicsVars(mpVars);
        mpBlur->execute(pRenderContext);


        pState->popFbo();
        for(uint32_t i = 0; i < arraySize; i++)
        {
            pState->popViewport(i);
        }

        pRenderContext->popGraphicsVars();
    }
}
