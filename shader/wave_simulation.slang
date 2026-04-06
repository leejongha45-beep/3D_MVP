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
RWStructuredBuffer<float> waveField;

[[vk::binding(3, 0)]]
RWStructuredBuffer<float> waveVelocity;

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
    float current = waveField[index];
    float c = scene.light.propagationSpeed;
    float damping = scene.light.damping;

    // 6방향 이웃 평균
    float neighborSum = 0.0;
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
            neighborSum += waveField[toIndex(uint3(neighborCoord))];
            neighborCount++;
        }
    }

    float neighborAvg = neighborSum / float(neighborCount);

    // 파동 방정식: velocity += c * (avg - current); field += velocity;
    float velocity = waveVelocity[index];
    velocity += c * (neighborAvg - current);
    velocity *= damping;

    waveVelocity[index] = velocity;
    waveField[index] = current + velocity;

    // 태양광 평면파 주입: 격자 상단(y=0)에서 아래 방향으로
    if (coord.y == 0)
    {
        // sourcePhase는 프레임마다 변하지 않으므로 격자 위치 기반으로 위상 생성
        float phase = float(coord.x + coord.z) * scene.light.frequency;
        waveField[index] += sin(phase) * scene.light.frequency;
    }

    // 경계 흡수: 격자 가장자리에서 에너지 감쇠
    if (coord.x == 0 || coord.x == GRID_SIZE - 1 ||
        coord.y == GRID_SIZE - 1 ||
        coord.z == 0 || coord.z == GRID_SIZE - 1)
    {
        waveField[index] *= 0.9;
        waveVelocity[index] *= 0.9;
    }
}
