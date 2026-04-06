// QuantumWave Renderer - 파동 시뮬레이션 Compute Shader
// 파인만 경로 적분 기반: velocity += c * (neighbor_avg - current); field += velocity;

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
RWStructuredBuffer<float4> waveField;    // xyz=RGB파동, w=반사계수

[[vk::binding(3, 0)]]
RWStructuredBuffer<float3> waveVelocity; // RGB 스펙트럼별 속도

static const uint GRID_SIZE = 128;

// 3D 좌표 → 1D 인덱스
uint toIndex(uint3 coord)
{
    return coord.z * GRID_SIZE * GRID_SIZE + coord.y * GRID_SIZE + coord.x;
}

// 격자 범위 내인지 확인
bool isInBounds(int3 coord)
{
    return coord.x >= 0 && coord.x < int(GRID_SIZE)
        && coord.y >= 0 && coord.y < int(GRID_SIZE)
        && coord.z >= 0 && coord.z < int(GRID_SIZE);
}

[shader("compute")]
[numthreads(8, 8, 8)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint3 coord = dispatchThreadID;

    if (coord.x >= GRID_SIZE || coord.y >= GRID_SIZE || coord.z >= GRID_SIZE)
        return;

    uint index = toIndex(coord);
    float4 currentField = waveField[index];
    float3 currentWave = currentField.xyz;
    float reflectCoeff = currentField.w;
    float c = scene.light.propagationSpeed;
    float damping = scene.light.damping;

    // 벽(오브젝트) 셀이면 반사 처리
    if (reflectCoeff > 0.0)
    {
        // 반사 계수만큼 파동 반전 (나머지는 흡수)
        waveField[index] = float4(-currentWave * reflectCoeff, reflectCoeff);
        waveVelocity[index] = -waveVelocity[index] * reflectCoeff;
        return;
    }

    // 6방향 이웃 평균
    float3 neighborSum = float3(0.0, 0.0, 0.0);
    int neighborCount = 0;

    int3 offsets[6] = {
        int3(1, 0, 0), int3(-1, 0, 0),
        int3(0, 1, 0), int3(0, -1, 0),
        int3(0, 0, 1), int3(0, 0, -1)
    };

    for (int i = 0; i < 6; ++i)
    {
        int3 neighborCoord = int3(coord) + offsets[i];
        if (isInBounds(neighborCoord))
        {
            neighborSum += waveField[toIndex(uint3(neighborCoord))].xyz;
            neighborCount++;
        }
    }

    float3 neighborAvg = neighborSum / float(neighborCount);

    // 파동 방정식: velocity += c * (avg - current); field += velocity;
    float3 velocity = waveVelocity[index];
    velocity += c * (neighborAvg - currentWave);
    velocity *= damping;

    waveVelocity[index] = velocity;
    waveField[index] = float4(currentWave + velocity, 0.0);

    // 태양광 평면파 주입: 격자 상단(y=0)에서 아래 방향으로 (백색광 = RGB 동일)
    if (coord.y == 0)
    {
        float phase = float(coord.x + coord.z) * scene.light.frequency;
        float wave = sin(phase) * scene.light.frequency;
        waveField[index] = float4(currentWave + velocity + float3(wave, wave, wave), 0.0);
    }

    // 경계 흡수: 격자 가장자리에서 에너지 감쇠
    if (coord.x == 0 || coord.x == GRID_SIZE - 1 ||
        coord.y == GRID_SIZE - 1 ||
        coord.z == 0 || coord.z == GRID_SIZE - 1)
    {
        waveField[index] = float4(waveField[index].xyz * 0.9, waveField[index].w);
        waveVelocity[index] *= 0.9;
    }
}
