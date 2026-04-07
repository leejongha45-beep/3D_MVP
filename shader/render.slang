// QuantumWave Renderer - 렌더링

struct CameraData
{
    float3 position;
    float _pad0;
    float3 forward;
    float _pad1;
    float3 up;
    float fov;
    float3 _pad2;
};

struct PlaneData
{
    float3 position;
    float _pad3;
    float3 normal;
    float _pad4;
    float3 color;
    float size;
    float3 _pad5;
};

struct LightData
{
    float3 direction;
    float frequency;
    float propagationSpeed;
    float damping;
    float time;
    float _pad0;
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

[[vk::binding(2, 0)]]
RWStructuredBuffer<float3> fieldXYZ;

[[vk::binding(3, 0)]]
RWStructuredBuffer<float> fieldW;

[[vk::binding(4, 0)]]
RWStructuredBuffer<float3> velocityXYZ;

[[vk::binding(5, 0)]]
RWStructuredBuffer<float> velocityW;

[[vk::binding(6, 0)]]
RWStructuredBuffer<float> accumCounter;

[[vk::binding(7, 0)]]
RWStructuredBuffer<float4> normalBuffer;

[[vk::binding(8, 0)]]
RWStructuredBuffer<float3> reflectedFieldXYZ;

static const uint GRID_SIZE = 256;
static const uint GRID_TOTAL = GRID_SIZE * GRID_SIZE;
static const float CELL_SIZE = 1.0;

static const int half_grid = int(GRID_SIZE) / 2;

uint toIndex(uint planeIdx, uint2 coord)
{
    return planeIdx * GRID_TOTAL + coord.y * GRID_SIZE + coord.x;
}

bool isInGrid(int2 coord)
{
    return coord.x >= 0 && coord.x < int(GRID_SIZE)
        && coord.y >= 0 && coord.y < int(GRID_SIZE);
}

// 레이-AABB 교차 + 법선
bool rayBoxIntersect(float3 rayOrigin, float3 rayDir, float3 boxMin, float3 boxMax, out float tHit, out float3 normal)
{
    tHit = 0.0;
    normal = float3(0, 0, 0);

    float3 invDir = 1.0 / rayDir;
    float3 t0 = (boxMin - rayOrigin) * invDir;
    float3 t1 = (boxMax - rayOrigin) * invDir;

    float3 tMin = min(t0, t1);
    float3 tMax = max(t0, t1);

    float tNear = max(max(tMin.x, tMin.y), tMin.z);
    float tFar = min(min(tMax.x, tMax.y), tMax.z);

    tHit = tNear;

    if (tNear > tFar || tFar < 0.0)
        return false;

    if (tMin.x > tMin.y && tMin.x > tMin.z)
        normal = float3(-sign(rayDir.x), 0, 0);
    else if (tMin.y > tMin.z)
        normal = float3(0, -sign(rayDir.y), 0);
    else
        normal = float3(0, 0, -sign(rayDir.z));

    return true;
}

// 3평면에서 direct + reflected wave 합산
float3 queryEnergy(float3 worldPos)
{
    float3 energy = float3(0.0, 0.0, 0.0);

    // xy 평면
    int2 xyCoord = int2(int(worldPos.x / CELL_SIZE) + half_grid, int(worldPos.y / CELL_SIZE) + half_grid);
    if (isInGrid(xyCoord))
    {
        uint idx = toIndex(0, uint2(xyCoord));
        float3 direct = abs(fieldXYZ[idx]);           // 직사광
        float3 reflected = abs(reflectedFieldXYZ[idx]); // 반사광
        energy += direct + reflected;
    }

    // xz 평면
    int2 xzCoord = int2(int(worldPos.x / CELL_SIZE) + half_grid, int(worldPos.z / CELL_SIZE) + half_grid);
    if (isInGrid(xzCoord))
    {
        uint idx = toIndex(1, uint2(xzCoord));
        float3 direct = abs(fieldXYZ[idx]);
        float3 reflected = abs(reflectedFieldXYZ[idx]);
        energy += direct + reflected;
    }

    // yz 평면
    int2 yzCoord = int2(int(worldPos.y / CELL_SIZE) + half_grid, int(worldPos.z / CELL_SIZE) + half_grid);
    if (isInGrid(yzCoord))
    {
        uint idx = toIndex(2, uint2(yzCoord));
        float3 direct = abs(fieldXYZ[idx]);
        float3 reflected = abs(reflectedFieldXYZ[idx]);
        energy += direct + reflected;
    }

    return energy / 3.0;
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

    float2 uv = (float2(texCoord) / float2(width, height)) * 2.0 - 1.0;
    uv.y = -uv.y;
    float aspectRatio = float(width) / float(height);
    uv.x *= aspectRatio;

    float3 camForward = scene.camera.forward;
    float3 camRight = normalize(cross(camForward, scene.camera.up));
    float3 camUp = cross(camRight, camForward);

    float fovScale = tan(radians(scene.camera.fov) * 0.5);
    float3 rayOrigin = scene.camera.position;
    float3 rayDirection = normalize(camForward + uv.x * fovScale * camRight + uv.y * fovScale * camUp);

    float4 color = float4(0.0, 0.0, 0.0, 1.0);
    float closestT = 1e30;
    float3 hitPos = float3(0, 0, 0);

    // 큐브 교차 (하드코딩)
    static const int CUBE_COUNT = 5;
    float3 cubePositions[CUBE_COUNT] = {
        float3(-20.0, 0.0, 5.0),
        float3(-10.0, 15.0, 5.0),
        float3(0.0, -10.0, 5.0),
        float3(15.0, 5.0, 5.0),
        float3(10.0, -20.0, 5.0)
    };
    float3 cubeSizes[CUBE_COUNT] = {
        float3(8.0, 8.0, 40.0),
        float3(6.0, 6.0, 50.0),
        float3(10.0, 10.0, 35.0),
        float3(7.0, 7.0, 45.0),
        float3(9.0, 9.0, 30.0)
    };

    for (int i = 0; i < CUBE_COUNT; ++i)
    {
        float3 boxMin = float3(
            cubePositions[i].x - cubeSizes[i].x * 0.5,
            cubePositions[i].y - cubeSizes[i].y * 0.5,
            cubePositions[i].z);
        float3 boxMax = float3(
            cubePositions[i].x + cubeSizes[i].x * 0.5,
            cubePositions[i].y + cubeSizes[i].y * 0.5,
            cubePositions[i].z + cubeSizes[i].z);

        float tHit;
        float3 normal;
        if (rayBoxIntersect(rayOrigin, rayDirection, boxMin, boxMax, tHit, normal))
        {
            if (tHit > 0.0 && tHit < closestT)
            {
                closestT = tHit;
                hitPos = rayOrigin + rayDirection * tHit;
            }
        }
    }

    // 바닥 교차 (z축 높이)
    float planeZ = scene.plane.position.z;
    if (abs(rayDirection.z) > 0.0001)
    {
        float t = (planeZ - rayOrigin.z) / rayDirection.z;
        if (t > 0.0 && t < closestT)
        {
            closestT = t;
            hitPos = rayOrigin + rayDirection * t;
        }
    }

    // 히트 지점에서 direct + reflected 에너지 쿼리
    if (closestT < 1e30)
    {
        float3 energy = queryEnergy(hitPos);
        float intensity = saturate(length(energy));
        color = float4(intensity, intensity, intensity, 1.0);
    }

    outputImage[texCoord] = color;
}
