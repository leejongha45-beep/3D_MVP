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
RWStructuredBuffer<float4> waveField;    // xyz=RGB파동, w=반사계수

[[vk::binding(3, 0)]]
RWStructuredBuffer<float3> waveVelocity; // RGB 스펙트럼별 속도

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

// 카메라 레이가 표면을 만나면 직전 셀의 파동 에너지 × 반사 스펙트럼
float3 queryWaveEnergy(float3 rayOrigin, float3 rayDirection)
{
    bool enteredGrid = false;
    float3 prevAmplitude = float3(0.0, 0.0, 0.0);

    float stepSize = 1.0;
    float maxDistance = float(GRID_SIZE) * 2.0;

    for (float t = 0.0; t < maxDistance; t += stepSize)
    {
        float3 samplePos = rayOrigin + rayDirection * t;
        int3 gridCoord = worldToGrid(samplePos);

        if (!isInGrid(gridCoord))
        {
            if (enteredGrid)
                break;
            continue;
        }
        enteredGrid = true;

        uint index = toIndex(uint3(gridCoord));
        float4 field = waveField[index];

        // 표면 셀을 만나면 직전 셀의 파동 에너지 × 반사계수
        if (field.w > 0.0)
        {
            float3 energy = prevAmplitude * prevAmplitude;
            return energy * field.w;
        }

        prevAmplitude = field.xyz;
    }

    return float3(0.0, 0.0, 0.0);
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

    // 카메라 방향으로 격자의 파동 에너지 수집
    float3 energy = queryWaveEnergy(rayOrigin, rayDirection);

    // 에너지 → 밝기 변환 (감마 보정)
    float3 brightness = pow(energy, float3(0.45, 0.45, 0.45));
    brightness = clamp(brightness, float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0));

    outputImage[texCoord] = float4(brightness, 1.0);
}
