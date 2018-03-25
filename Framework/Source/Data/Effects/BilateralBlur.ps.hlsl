texture2D gSrcTex;

SamplerState gSampler;

struct BlurPSIn
{
    float2 texC : TEXCOORD;
    float4 pos : SV_POSITION;
};

// TODO: rename.
Buffer<float> weights;

static const float PI = 3.14159265f;

#if (0)
float4 blur(float2 texC)
{
    const int2 offset = -(_KERNEL_WIDTH / 2) * dir;

    float4 c = float4(0,0,0,0);
    $for(i in Range(_KERNEL_WIDTH))
    {
        c += gSrcTex.SampleLevel(gSampler, texC, 0, offset + i*dir)*weights[i];
    }
    return c;
}
#endif

// TODO: Some stuff here can be precomputed.
float rangeGaussianCoefficient(float x)
{
    float p = -(x * x) / (2 * _RANGE_SIGMA_SQUARED);
    float e = exp(p);

    float a = 2 * PI * _RANGE_SIGMA_SQUARED;
    return e / a;
}

float4 blur(float2 texC)
{
    const int halfKernelWidth = _KERNEL_WIDTH / 2;
    const float4 centerColor = gSrcTex.SampleLevel(gSampler, texC, 0);

    float4 c = float4(0.0f, 0.0f, 0.0f, 0.0f);
    float normFactor = 0.0f;
    for (int i = -halfKernelWidth; i <= halfKernelWidth; ++i)
    {
        for (int j = -halfKernelWidth; j <= halfKernelWidth; ++j)
        {
            int2 offset = int2(i, j);
            float4 sampleColor = gSrcTex.SampleLevel(gSampler, texC, 0, offset);
            float colorDist = distance(centerColor, sampleColor);
            float rangeCoef = rangeGaussianCoefficient(colorDist);

            int linIdx = (i + halfKernelWidth) * _KERNEL_WIDTH + (j + halfKernelWidth);
            float spatialCoef = weights[linIdx];

            float weight = spatialCoef * rangeCoef;

            c += weight * sampleColor;
            normFactor += weight;
        }
    }

    c /= normFactor;
    return c;
}

float4 main(BlurPSIn pIn) : SV_TARGET0
{
    float4 fragColor = float4(1.f, 1.f, 1.f, 1.f);

    fragColor = blur(pIn.texC);

    return fragColor;
}