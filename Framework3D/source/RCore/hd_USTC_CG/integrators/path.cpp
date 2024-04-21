#include "path.h"

#include <random>

#include "surfaceInteraction.h"
#include "utils/sampling.hpp"
USTC_CG_NAMESPACE_OPEN_SCOPE
using namespace pxr;

VtValue PathIntegrator::Li(const GfRay& ray, std::default_random_engine& random)
{
    std::uniform_real_distribution<float> uniform_dist(
        0.0f, 1.0f - std::numeric_limits<float>::epsilon());
    std::function<float()> uniform_float = std::bind(uniform_dist, random);

    auto color = EstimateOutGoingRadiance(ray, uniform_float, 0);

    return VtValue(GfVec3f(color[0], color[1], color[2]));
}

GfVec3f PathIntegrator::EstimateOutGoingRadiance(
    const GfRay& ray,
    const std::function<float()>& uniform_float,
    int recursion_depth)
{
    if (recursion_depth >= 50) {
        return {};
    }

    SurfaceInteraction si;
    if (!Intersect(ray, si)) {
        // ray intersects nothing
        if (recursion_depth == 0) {
            return IntersectDomeLight(ray);
            // use dome light for infinite far. This will automatically decide if there is a dome
            // light.
            // Only use for the first bounce. It is for easy running.
        }

        return GfVec3f{ 0, 0, 0 };
    }

    // ray intersects something, stored in si

    // This can be customized : Do we want to see the lights? (Other than dome lights?)
    if (recursion_depth == 0 && IntersectDomeLight(ray) == GfVec3f(0.f)) {
        GfVec3f intersecPos;
        auto light_color = IntersectLights(ray, intersecPos);
        if (light_color != GfVec3f(0.f)) {
            return light_color;
        }
    }

    // Flip the normal if opposite
    if (GfDot(si.shadingNormal, ray.GetDirection()) > 0) {
        si.flipNormal();
        si.PrepareTransforms();
    }

    GfVec3f color{ 0 };
    GfVec3f directLight = EstimateDirectLight(si, uniform_float);

    // HW7_TODO: Estimate global lighting here.
    GfVec3f globalLight = GfVec3f(0.f);
    const float russian_roulette = 1.0;
    if (uniform_float() > russian_roulette) {
        // Introduce Russian Roulette by randomly terminate the path
        return directLight;
    }
    // float distance = (si.position - ray.GetStartPoint()).GetLength();
    // float distance = 1.0f;
    float sample_pos_pdf;
    // GfVec3f wi = UniformSampleHemiSphere(GfVec2f(uniform_float(), uniform_float()),
    // sample_pos_pdf); 
    // randomly choose input direction with cosine weight and sample_pos_pdf

    GfVec3f wi;
    GfVec3f wo = si.WorldToTangent(si.wo);
    wi = UniformSampleHemiSphere(GfVec2f(uniform_float(), uniform_float()), sample_pos_pdf);
    auto brdfVal = si.Eval(wi);
    // Sample BRDF
    // auto brdfVal = si.material->Sample(wo, wi, sample_pos_pdf, si.texcoord, uniform_float);
    // recursively estimate the outgoing radiance
    auto L = EstimateOutGoingRadiance(
        GfRay(si.position, si.TangentToWorld(wi)), uniform_float, recursion_depth + 1);
    globalLight =
        GfCompMult(brdfVal, L) * GfDot(si.shadingNormal, wi) / sample_pos_pdf / russian_roulette;

    color = directLight + globalLight;

    return color;
}

USTC_CG_NAMESPACE_CLOSE_SCOPE
