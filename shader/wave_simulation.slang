// QuantumWave Renderer - 3평면 파동 시뮬레이션 (xz, xy, yz)

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
RWStructuredBuffer<float4> waveField;    // 3평면 × 1024x1024

[[vk::binding(3, 0)]]
RWStructuredBuffer<float4> waveVelocity;

static const uint GRID_SIZE = 1024;
static const uint GRID_TOTAL = GRID_SIZE * GRID_SIZE;

uint toIndex(uint planeIdx, uint2 coord)
{
    return planeIdx * GRID_TOTAL + coord.y * GRID_SIZE + coord.x;
}

bool isInBounds(int2 coord)
{
    return coord.x >= 0 && coord.x < int(GRID_SIZE)
        && coord.y >= 0 && coord.y < int(GRID_SIZE);
}

void simulatePlane(uint planeIdx, uint2 coord, float c, float damping, float time, float frequency)
{
    if (coord.x >= GRID_SIZE || coord.y >= GRID_SIZE)
        return;

    uint index = toIndex(planeIdx, coord);
    float4 currentField = waveField[index];
    float3 currentWave = currentField.xyz;

    // 4방향 이웃 평균
    float3 neighborSum = float3(0.0, 0.0, 0.0);
    int neighborCount = 0;

    int2 offsets[4] = {
        int2(1, 0), int2(-1, 0),
        int2(0, 1), int2(0, -1)
    };

    for (int i = 0; i < 4; ++i)
    {
        int2 neighborCoord = int2(coord) + offsets[i];
        if (isInBounds(neighborCoord))
        {
            neighborSum += waveField[toIndex(planeIdx, uint2(neighborCoord))].xyz;
            neighborCount++;
        }
    }

    float3 neighborAvg = neighborSum / float(neighborCount);

    float roughnessVal = waveVelocity[index].w;
    float3 velocity = waveVelocity[index].xyz;
    velocity += c * (neighborAvg - currentWave);
    velocity *= damping;

    // 표면 셀이면 roughness에 따라 감쇠
    if (currentField.w > 0.0)
    {
        float reflectFactor = 1.0 - roughnessVal * 0.8;
        velocity *= reflectFactor;
    }

    waveVelocity[index] = float4(velocity, roughnessVal);
    waveField[index] = float4(currentWave + velocity, currentField.w);

    // 태양광 평면파 주입: 각 평면의 상단(y=0)에서
    if (coord.y == 0)
    {
        float phase = time * frequency;
        float wave = sin(phase) * 0.02;
        waveField[index] = float4(currentWave + velocity + float3(wave, wave, wave), currentField.w);
    }
}

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    uint2 coord = dispatchThreadID.xy;
    float c = scene.light.propagationSpeed;
    float damping = scene.light.damping;
    float time = scene.light.time;
    float frequency = scene.light.frequency;

    // 3평면 시뮬레이션 (z축이 높이)
    simulatePlane(0, coord, c, damping, time, frequency); // xy (위에서 아래)
    simulatePlane(1, coord, c, damping, time, frequency); // xz (앞에서 뒤)
    simulatePlane(2, coord, c, damping, time, frequency); // yz (옆에서)
}
