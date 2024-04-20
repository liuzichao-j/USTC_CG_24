#include "light.h"

#include "Utils/Logging/Logging.h"
#include "pxr/base/gf/plane.h"
#include "pxr/base/gf/ray.h"
#include "pxr/base/gf/rotation.h"
#include "pxr/base/gf/vec2f.h"
#include "pxr/imaging/glf/simpleLight.h"
#include "pxr/imaging/hd/changeTracker.h"
#include "pxr/imaging/hd/rprimCollection.h"
#include "pxr/imaging/hd/sceneDelegate.h"
#include "pxr/imaging/hio/image.h"
#include "renderParam.h"
#include "texture.h"
#include "utils/math.hpp"
#include "utils/sampling.hpp"

USTC_CG_NAMESPACE_OPEN_SCOPE
using namespace pxr;
void Hd_USTC_CG_Light::Sync(
    HdSceneDelegate* sceneDelegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits)
{
    static_cast<Hd_USTC_CG_RenderParam*>(renderParam)->AcquireSceneForEdit();

    TRACE_FUNCTION();
    HF_MALLOC_TAG_FUNCTION();

    TF_UNUSED(renderParam);

    if (!TF_VERIFY(sceneDelegate != nullptr)) {
        return;
    }

    const SdfPath& id = GetId();

    // Change tracking
    HdDirtyBits bits = *dirtyBits;

    // Transform
    if (bits & DirtyTransform) {
        _params[HdTokens->transform] = VtValue(sceneDelegate->GetTransform(id));
    }

    // Lighting Params
    if (bits & DirtyParams) {
        HdChangeTracker& changeTracker = sceneDelegate->GetRenderIndex().GetChangeTracker();

        // Remove old dependencies
        VtValue val = Get(HdTokens->filters);
        if (val.IsHolding<SdfPathVector>()) {
            auto lightFilterPaths = val.UncheckedGet<SdfPathVector>();
            for (const SdfPath& filterPath : lightFilterPaths) {
                changeTracker.RemoveSprimSprimDependency(filterPath, id);
            }
        }

        if (_lightType == HdPrimTypeTokens->simpleLight) {
            _params[HdLightTokens->params] = sceneDelegate->Get(id, HdLightTokens->params);
        }

        // Add new dependencies
        val = Get(HdTokens->filters);
        if (val.IsHolding<SdfPathVector>()) {
            auto lightFilterPaths = val.UncheckedGet<SdfPathVector>();
            for (const SdfPath& filterPath : lightFilterPaths) {
                changeTracker.AddSprimSprimDependency(filterPath, id);
            }
        }
    }

    *dirtyBits = Clean;
}

HdDirtyBits Hd_USTC_CG_Light::GetInitialDirtyBitsMask() const
{
    if (_lightType == HdPrimTypeTokens->simpleLight ||
        _lightType == HdPrimTypeTokens->distantLight) {
        return AllDirty;
    }
    else {
        return (DirtyParams | DirtyTransform);
    }
}

bool Hd_USTC_CG_Light::IsDomeLight()
{
    return _lightType == HdPrimTypeTokens->domeLight;
}

void Hd_USTC_CG_Light::Finalize(HdRenderParam* renderParam)
{
    static_cast<Hd_USTC_CG_RenderParam*>(renderParam)->AcquireSceneForEdit();

    HdLight::Finalize(renderParam);
}

VtValue Hd_USTC_CG_Light::Get(const TfToken& token) const
{
    VtValue val;
    TfMapLookup(_params, token, &val);
    return val;
}

Color Hd_USTC_CG_Sphere_Light::Sample(
    const GfVec3f& pos,
    GfVec3f& dir,
    GfVec3f& sampled_light_pos,
    float& sample_light_pdf,
    const std::function<float()>& uniform_float)
{
    auto distanceVec = position - pos;

    // x, y, z direction. z = -distanceVec
    auto basis = constructONB(-distanceVec.GetNormalized());

    auto distance = distanceVec.GetLength();

    // A sphere light is treated as all points on the surface spreads energy uniformly:
    float sample_pos_pdf;
    // First we sample a point on the hemi sphere whose radius is 1:
    // On known two uniform random variables, we set them as: (theta, r_xy ^ 2), and the pdf is
    // sqrt(1 - r_xy ^ 2) / pi
    auto sampledDir =
        CosineWeightedDirection(GfVec2f(uniform_float(), uniform_float()), sample_pos_pdf);
    // Using the basis to transform the direction to the world space, with z = normal.
    auto worldSampledDir = basis * sampledDir;

    auto sampledPosOnSurface = worldSampledDir * radius + position;
    sampled_light_pos = sampledPosOnSurface;
    // Light position is on the hemisphere. Assume the light mostly emits light on the radius
    // direction.

    // Then we can decide the direction.
    dir = (sampledPosOnSurface - pos).GetNormalized();

    // and the pdf (with the measure of solid angle):
    float cosVal = GfDot(-dir, worldSampledDir.GetNormalized());

    sample_light_pdf = sample_pos_pdf / radius / radius * cosVal * distance * distance;
    // The pdf is the product of pdf of the position and the pdf of the direction.

    // Finally we calculate the radiance
    if (cosVal < 0) {
        return Color{ 0 };
    }
    return irradiance * cosVal / M_PI;
    // irradiance means the color of the light (also the power).
}

Color Hd_USTC_CG_Sphere_Light::Intersect(const GfRay& ray, float& depth)
{
    double distance;
    // Intersects the ray with a sphere. 
    if (ray.Intersect(GfRange3d{ position - GfVec3d{ radius }, position + GfVec3d{ radius } })) {
        if (ray.Intersect(position, radius, &distance)) {
            depth = distance;

            // Return the radiance of the light intersected by the ray.
            return irradiance / M_PI;
        }
    }
    depth = std::numeric_limits<float>::infinity();
    return { 0, 0, 0 };
}

void Hd_USTC_CG_Sphere_Light::Sync(
    HdSceneDelegate* sceneDelegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits)
{
    Hd_USTC_CG_Light::Sync(sceneDelegate, renderParam, dirtyBits);
    auto id = GetId();

    radius = sceneDelegate->GetLightParamValue(id, HdLightTokens->radius).Get<float>();

    auto diffuse = sceneDelegate->GetLightParamValue(id, HdLightTokens->diffuse).Get<float>();
    power = sceneDelegate->GetLightParamValue(id, HdLightTokens->color).Get<GfVec3f>() * diffuse;
    // power = color * diffuse

    auto transform = Get(HdTokens->transform).GetWithDefault<GfMatrix4d>();

    GfVec3d p = transform.ExtractTranslation();
    position = GfVec3f(p[0], p[1], p[2]);

    area = 4 * M_PI * radius * radius;

    irradiance = power / area;
    // irradiance means power per unit area on the sphere surface.
}

Color Hd_USTC_CG_Dome_Light::Sample(
    const GfVec3f& pos,
    GfVec3f& dir,
    GfVec3f& sampled_light_pos,
    float& sample_light_pdf,
    const std::function<float()>& uniform_float)
{
    // Uniformly sample a point on the sphere with radius 1. 
    dir = UniformSampleSphere(GfVec2f{ uniform_float(), uniform_float() }, sample_light_pdf);
    // Assume light is from the infinite distance.
    sampled_light_pos = dir * std::numeric_limits<float>::max() / 100.f;

    return Le(dir);
    // Color of given texture on dir. 
}

Color Hd_USTC_CG_Dome_Light::Intersect(const GfRay& ray, float& depth)
{
    depth = std::numeric_limits<float>::max() / 100.f;  // max is smaller than infinity, lol

    return Le(GfVec3f(ray.GetDirection()));
    // Color of given texture on ray direction.
}

void Hd_USTC_CG_Dome_Light::_PrepareDomeLight(SdfPath const& id, HdSceneDelegate* sceneDelegate)
{
    const VtValue v = sceneDelegate->GetLightParamValue(id, HdLightTokens->textureFile);
    if (!v.IsEmpty()) {
        if (v.IsHolding<SdfAssetPath>()) {
            textureFileName = v.UncheckedGet<SdfAssetPath>();
            texture = std::make_unique<Texture2D>(textureFileName);
            if (!texture->isValid()) {
                texture = nullptr;
            }

            logging("Attempting to load file " + textureFileName.GetAssetPath(), Warning);
        }
        else {
            texture = nullptr;
        }
    }
    auto diffuse = sceneDelegate->GetLightParamValue(id, HdLightTokens->diffuse).Get<float>();
    radiance = sceneDelegate->GetLightParamValue(id, HdLightTokens->color).Get<GfVec3f>() * diffuse;
}

void Hd_USTC_CG_Dome_Light::Sync(
    HdSceneDelegate* sceneDelegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits)
{
    Hd_USTC_CG_Light::Sync(sceneDelegate, renderParam, dirtyBits);

    auto id = GetId();
    _PrepareDomeLight(id, sceneDelegate);
}

/**
 * @brief Evaluate the radiance of the dome light at the given direction.
 * @param dir The direction to evaluate the radiance.
 * @return The radiance of the dome light at the given direction.
 */
Color Hd_USTC_CG_Dome_Light::Le(const GfVec3f& dir)
{
    if (texture != nullptr) {
        // dir should be normalized.
        auto uv = GfVec2f((M_PI + std::atan2(dir[1], dir[0])) / 2.0 / M_PI, 0.5 - dir[2] * 0.5);
        // uv means the coordinate of the texture. u = (pi + atan2(y, x)) / 2pi means to map the
        // angle to [0, 1]. v = 0.5 - z / 2 means to map the z to [0, 1].

        auto value = texture->Evaluate(uv);
        // Get texture value on the given uv coordinate.

        if (texture->component_conut() >= 3) {
            return GfCompMult(Color{ value[0], value[1], value[2] }, radiance);
            // Get the color. Texture * radiance.
        }
    }
    else {
        return radiance;
        // No texture, return the color directly.
    }
}

void Hd_USTC_CG_Dome_Light::Finalize(HdRenderParam* renderParam)
{
    texture = nullptr;
    Hd_USTC_CG_Light::Finalize(renderParam);
}

// HW7_TODO: write the following, you should refer to the sphere light.

void Hd_USTC_CG_Distant_Light::Sync(
    HdSceneDelegate* sceneDelegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits)
{
    Hd_USTC_CG_Light::Sync(sceneDelegate, renderParam, dirtyBits);
    auto id = GetId();
    angle = sceneDelegate->GetLightParamValue(id, HdLightTokens->angle).Get<float>();
    angle = std::clamp(angle, 0.03f, 89.9f) * M_PI / 180.0f;
    // The light is limited in 3D angle. 

    auto diffuse = sceneDelegate->GetLightParamValue(id, HdLightTokens->diffuse).Get<float>();
    radiance = sceneDelegate->GetLightParamValue(id, HdLightTokens->color).Get<GfVec3f>() *
               diffuse / (1 - cos(angle)) / 2.0 / M_PI;

    auto transform = Get(HdTokens->transform).GetWithDefault<GfMatrix4d>();

    direction = transform.TransformDir(GfVec3f(0, 0, -1)).GetNormalized();
}

Color Hd_USTC_CG_Distant_Light::Sample(
    const GfVec3f& pos,
    GfVec3f& dir,
    GfVec3f& sampled_light_pos,
    float& sample_light_pdf,
    const std::function<float()>& uniform_float)
{
    float theta = uniform_float() * angle;
    float phi = uniform_float() * 2 * M_PI;

    auto sampled_dir = GfVec3f(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));

    auto basis = constructONB(-direction);
    // - means to see from the object's view. 

    dir = basis * sampled_dir;
    sampled_light_pos = pos + dir * std::numeric_limits<float>::max() / 100.f;
    // To infinity distance.

    sample_light_pdf = 1.0f / sin(theta) / (2.0f * M_PI * angle);

    return radiance;
}

Color Hd_USTC_CG_Distant_Light::Intersect(const GfRay& ray, float& depth)
{
    depth = std::numeric_limits<float>::max() / 100.f;

    if (GfDot(ray.GetDirection().GetNormalized(), -direction) > cos(angle)) {
        return radiance;
    }
    return Color(0);
}

Color Hd_USTC_CG_Rect_Light::Sample(
    const GfVec3f& pos,
    GfVec3f& dir,
    GfVec3f& sampled_light_pos,
    float& sample_light_pdf,
    const std::function<float()>& uniform_float)
{
    float x = uniform_float();
    float y = uniform_float();

    sampled_light_pos = corner0 + (corner2 - corner0) * x + (corner1 - corner0) * y;

    dir = sampled_light_pos - pos;
    auto distance = dir.GetLength();
    dir = dir.GetNormalized();
    auto normal = GfCross(corner2 - corner0, corner1 - corner0).GetNormalized();
    float cosVal = GfDot(-dir, normal);

    sample_light_pdf = 2.0f * distance * distance / (width * height);
    if(cosVal < 0) {
        return Color{ 0 };
    }
    return irradiance * cosVal / M_PI;
}

Color Hd_USTC_CG_Rect_Light::Intersect(const GfRay& ray, float& depth)
{
    double distance;
    bool frontFacing;
    GfPlane plane(corner0, corner1, corner2);
    if (ray.Intersect(plane, &distance, &frontFacing)) {
        GfVec3d p = ray.GetPoint(distance);
        GfVec3d v0 = corner1 - corner0;
        GfVec3d v1 = corner2 - corner0;
        GfVec3d v2 = p - corner0;
        
        double dot00 = GfDot(v0, v0);
        double dot01 = GfDot(v0, v1);
        double dot02 = GfDot(v0, v2);
        double dot11 = GfDot(v1, v1);
        double dot12 = GfDot(v1, v2);
        double invDenom = 1 / (dot00 * dot11 - dot01 * dot01);
        double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
        double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

        if (u >= 0 && v >= 0 && u <= 1 && v <= 1) {
            depth = distance;
            return irradiance / M_PI;
        }
    }
    depth = std::numeric_limits<float>::infinity();
    return { 0, 0, 0 };
}

void Hd_USTC_CG_Rect_Light::Sync(
    HdSceneDelegate* sceneDelegate,
    HdRenderParam* renderParam,
    HdDirtyBits* dirtyBits)
{
    Hd_USTC_CG_Light::Sync(sceneDelegate, renderParam, dirtyBits);

    auto transform = Get(HdTokens->transform).GetWithDefault<GfMatrix4d>();

    auto id = GetId();
    width = sceneDelegate->GetLightParamValue(id, HdLightTokens->width).Get<float>();
    height = sceneDelegate->GetLightParamValue(id, HdLightTokens->height).Get<float>();

    corner0 = transform.TransformAffine(GfVec3f(-0.5 * width, -0.5 * height, 0));
    corner1 = transform.TransformAffine(GfVec3f(-0.5 * width, 0.5 * height, 0));
    corner2 = transform.TransformAffine(GfVec3f(0.5 * width, -0.5 * height, 0));
    corner3 = transform.TransformAffine(GfVec3f(0.5 * width, 0.5 * height, 0));

    auto diffuse = sceneDelegate->GetLightParamValue(id, HdLightTokens->diffuse).Get<float>();
    power = sceneDelegate->GetLightParamValue(id, HdLightTokens->color).Get<GfVec3f>() * diffuse;

    // HW7_TODO: calculate irradiance

    area = width * height;
    irradiance = power / area;
}

USTC_CG_NAMESPACE_CLOSE_SCOPE
