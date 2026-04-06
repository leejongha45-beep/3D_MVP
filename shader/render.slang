// QuantumWave Renderer - 렌더링 Compute Shader
// 카메라 레이를 쏘고 파동 에너지 격자에서 밝기를 샘플링

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
    float2 _pad0;
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
RWStructuredBuffer<float> waveField;

[[vk::binding(3, 0)]]
RWStructuredBuffer<float> waveVelocity;

static const uint GRID_SIZE = 128;

// 3D 좌표 → 1D 인덱스
uint toIndex(uint3 coord)
{
    return coord.z * GRID_SIZE * GRID_SIZE + coord.y * GRID_SIZE + coord.x;
}

// 월드 좌표 → 격자 좌표 변환 (격자 중심이 월드 원점)
int3 worldToGrid(float3 worldPos)
{
    float3 gridPos = worldPos + float3(GRID_SIZE / 2, 0, GRID_SIZE / 2);
    return int3(gridPos);
}

// 격자 범위 내인지 확인
bool isInGrid(int3 coord)
{
    return coord.x >= 0 && coord.x < int(GRID_SIZE)
        && coord.y >= 0 && coord.y < int(GRID_SIZE)
        && coord.z >= 0 && coord.z < int(GRID_SIZE);
}

// 레이-플레인 교차
bool rayPlaneIntersect(float3 rayOrigin, float3 rayDir, float3 planePos, float3 planeNormal, float planeSize, out float t, out float3 hitPoint)
{
    float denom = dot(planeNormal, rayDir);
    if (abs(denom) < 0.0001)
        return false;

    t = dot(planePos - rayOrigin, planeNormal) / denom;
    if (t < 0.0)
        return false;

    hitPoint = rayOrigin + rayDir * t;
    float3 localPos = hitPoint - planePos;
    float halfSize = planeSize * 0.5;

    if (abs(localPos.x) > halfSize || abs(localPos.z) > halfSize)
        return false;

    return true;
}

// 파동 에너지 샘플링: 해당 격자 위치의 field^2 값
float sampleWaveEnergy(int3 gridCoord)
{
    if (!isInGrid(gridCoord))
        return 0.0;

    // y축 전체를 따라 에너지 누적 (수직 방향 광자 경로)
    float energy = 0.0;
    for (int y = 0; y < int(GRID_SIZE); ++y)
    {
        uint index = toIndex(uint3(gridCoord.x, y, gridCoord.z));
        float field = waveField[index];
        energy += field * field;
    }

    return energy;
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
    uv.y = -uv.y;
    float aspectRatio = float(width) / float(height);
    uv.x *= aspectRatio;

    // 카메라 레이 생성
    float3 forward = scene.camera.forward;
    float3 right = normalize(cross(forward, scene.camera.up));
    float3 up = cross(right, forward);

    float fovScale = tan(radians(scene.camera.fov) * 0.5);
    float3 rayOrigin = scene.camera.position;
    float3 rayDirection = normalize(forward + uv.x * fovScale * right + uv.y * fovScale * up);

    // 배경색
    float4 color = float4(0.0, 0.0, 0.0, 1.0);

    // Uniform Buffer 플레인 값으로 교차 테스트
    float denom = dot(scene.plane.normal, rayDirection);
    if (abs(denom) > 0.0001)
    {
        float t = dot(scene.plane.position - rayOrigin, scene.plane.normal) / denom;
        if (t > 0.0)
        {
            float3 hp = rayOrigin + rayDirection * t;
            float halfSize = scene.plane.size * 0.5;
            if (abs(hp.x - scene.plane.position.x) <= halfSize && abs(hp.z - scene.plane.position.z) <= halfSize)
            {
                color = float4(1.0, 1.0, 1.0, 1.0);
            }
        }
    }

    outputImage[texCoord] = color;
}
