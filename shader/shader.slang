// QuantumWave Renderer - Compute Shader (레이마칭 렌더링)

struct CameraData
{
    float3 position;
    float _pad0;
    float3 target;
    float _pad1;
    float3 up;
    float _pad2;
    float fov;
};

struct PlaneData
{
    float3 position;
    float _pad0;
    float3 normal;
    float _pad1;
    float3 color;
    float size;
};

struct LightData
{
    float3 direction;
    float _pad0;
    float3 color;
    float intensity;
    float ambient;
};

struct SceneData
{
    CameraData camera;
    PlaneData plane;
    LightData light;
};

[[vk::binding(0, 0)]]
RWTexture2D<float4> outputImage;

[[vk::binding(1, 0)]]
ConstantBuffer<SceneData> scene;

// 레이-AABB 교차 검사
bool rayBoxIntersect(float3 rayOrigin, float3 rayDir, float3 boxMin, float3 boxMax, out float tNear, out float tFar, out float3 normal)
{
    float3 invDir = 1.0 / rayDir;
    float3 t0 = (boxMin - rayOrigin) * invDir;
    float3 t1 = (boxMax - rayOrigin) * invDir;

    float3 tMin = min(t0, t1);
    float3 tMax = max(t0, t1);

    tNear = max(max(tMin.x, tMin.y), tMin.z);
    tFar = min(min(tMax.x, tMax.y), tMax.z);

    if (tNear > tFar || tFar < 0.0)
        return false;

    normal = float3(0, 0, 0);
    if (tMin.x > tMin.y && tMin.x > tMin.z)
        normal = float3(-sign(rayDir.x), 0, 0);
    else if (tMin.y > tMin.z)
        normal = float3(0, -sign(rayDir.y), 0);
    else
        normal = float3(0, 0, -sign(rayDir.z));

    return true;
}

// 레이-평면 교차 검사
bool rayPlaneIntersect(float3 rayOrigin, float3 rayDir, float3 planePos, float3 planeNormal, float planeSize, out float t)
{
    float denom = dot(planeNormal, rayDir);
    if (abs(denom) < 0.0001)
        return false;

    t = dot(planePos - rayOrigin, planeNormal) / denom;
    if (t < 0.0)
        return false;

    float3 hitPoint = rayOrigin + rayDir * t;
    float3 localPos = hitPoint - planePos;
    float halfSize = planeSize * 0.5;

    if (abs(localPos.x) > halfSize || abs(localPos.z) > halfSize)
        return false;

    return true;
}

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 texCoord = dispatchThreadID.xy;

    uint width, height;
    outputImage.GetDimensions(width, height);

    if (texCoord.x >= width || texCoord.y >= height)
        return;

    // NDC 좌표 (-1 ~ 1)
    float2 uv = (float2(texCoord) / float2(width, height)) * 2.0 - 1.0;
    float aspectRatio = float(width) / float(height);
    uv.x *= aspectRatio;

    // 카메라 (Uniform Buffer에서 읽기)
    float3 forward = normalize(scene.camera.target - scene.camera.position);
    float3 right = normalize(cross(forward, scene.camera.up));
    float3 up = cross(right, forward);

    float fovScale = tan(radians(scene.camera.fov) * 0.5);
    float3 rayDirection = normalize(forward + uv.x * fovScale * right + uv.y * fovScale * up);

    // 배경색
    float4 color = float4(0.05, 0.05, 0.08, 1.0);
    float closestT = 1e30;

    // 레이-큐브 교차 (임시 하드코딩)
    float3 boxMin = float3(-0.5, -0.5, -0.5);
    float3 boxMax = float3(0.5, 0.5, 0.5);

    float tNear, tFar;
    float3 normal;
    if (rayBoxIntersect(scene.camera.position, rayDirection, boxMin, boxMax, tNear, tFar, normal))
    {
        if (tNear < closestT)
        {
            closestT = tNear;
            float diffuse = max(dot(normal, scene.light.direction), 0.0);
            float3 baseColor = float3(0.8, 0.3, 0.2);
            color = float4(baseColor * (scene.light.ambient + diffuse * scene.light.intensity) * scene.light.color, 1.0);
        }
    }

    // 레이-플레인 교차
    float tPlane;
    if (rayPlaneIntersect(scene.camera.position, rayDirection, scene.plane.position, scene.plane.normal, scene.plane.size, tPlane))
    {
        if (tPlane < closestT)
        {
            closestT = tPlane;
            float diffuse = max(dot(scene.plane.normal, scene.light.direction), 0.0);
            color = float4(scene.plane.color * (scene.light.ambient + diffuse * scene.light.intensity) * scene.light.color, 1.0);
        }
    }

    outputImage[texCoord] = color;
}
