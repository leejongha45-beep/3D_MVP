// QuantumWave Renderer - 파동 시뮬레이션

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
RWStructuredBuffer<float4> normalBuffer; // 표면 셀의 normal (xyz) + pad

[[vk::binding(8, 0)]]
RWStructuredBuffer<float3> reflectedFieldXYZ; // 반사 wave 필드

static const uint GRID_SIZE = 256;
static const uint GRID_TOTAL = GRID_SIZE * GRID_SIZE;

static const uint PLANE_COUNT = 3;

uint toIndex(uint planeIdx, uint2 coord)
{
    return planeIdx * GRID_TOTAL + coord.y * GRID_SIZE + coord.x;
}

bool isInBounds(int2 coord)
{
    return coord.x >= 0 && coord.x < int(GRID_SIZE)
        && coord.y >= 0 && coord.y < int(GRID_SIZE);
}

void simulatePlane(uint planeIdx, uint2 coord, float c, float damping, float time, float frequency, float2 lightProjected)
{
    uint index = toIndex(planeIdx, coord);
    float currentW = fieldW[index];

    // 표면 셀: 반사 wave 생성 (Step 2)
    if (currentW > 0.0)
    {
        float lpLenSurf = length(lightProjected);
        if (lpLenSurf > 0.0001)
        {
            float2 kDirSurf = normalize(lightProjected);
            // 입사 방향 역방향(빛 오는 쪽)의 이웃 셀에서 direct wave 읽음
            int2 srcCell = int2(round(float2(coord) - kDirSurf));
            if (isInBounds(srcCell))
            {
                uint srcIdx = toIndex(planeIdx, uint2(srcCell));
                if (fieldW[srcIdx] == 0.0)
                {
                    float3 incoming = fieldXYZ[srcIdx];
                    float reflectFactor = fieldW[index];
                    reflectedFieldXYZ[index] = incoming * reflectFactor;
                }
            }
        }
        return;
    }

    float lpLen = length(lightProjected);

    // Step 1: Direct advection with bilinear interpolation (회절 없음, 정확한 사선 이동)
    if (lpLen > 0.0001)
    {
        float2 kDir = normalize(lightProjected);
        float2 prevPos = float2(coord) - kDir * c;

        int2 p00 = int2(floor(prevPos));
        float2 frac = prevPos - float2(p00);

        int2 p10 = p00 + int2(1, 0);
        int2 p01 = p00 + int2(0, 1);
        int2 p11 = p00 + int2(1, 1);

        float3 v00 = float3(0.0), v10 = float3(0.0), v01 = float3(0.0), v11 = float3(0.0);
        bool blocked = false;

        if (isInBounds(p00)) {
            uint i = toIndex(planeIdx, uint2(p00));
            if (fieldW[i] > 0.0) blocked = true;
            else v00 = fieldXYZ[i];
        }
        if (isInBounds(p10)) {
            uint i = toIndex(planeIdx, uint2(p10));
            if (fieldW[i] > 0.0) blocked = true;
            else v10 = fieldXYZ[i];
        }
        if (isInBounds(p01)) {
            uint i = toIndex(planeIdx, uint2(p01));
            if (fieldW[i] > 0.0) blocked = true;
            else v01 = fieldXYZ[i];
        }
        if (isInBounds(p11)) {
            uint i = toIndex(planeIdx, uint2(p11));
            if (fieldW[i] > 0.0) blocked = true;
            else v11 = fieldXYZ[i];
        }

        if (blocked)
        {
            fieldXYZ[index] = float3(0.0, 0.0, 0.0);
        }
        else
        {
            float w00 = (1.0 - frac.x) * (1.0 - frac.y);
            float w10 = frac.x * (1.0 - frac.y);
            float w01 = (1.0 - frac.x) * frac.y;
            float w11 = frac.x * frac.y;
            fieldXYZ[index] = (v00 * w00 + v10 * w10 + v01 * w01 + v11 * w11) * damping;
        }
    }

    // Step 3: 반사 wave 전파 (normal 기반 gather)
    // 빈 셀이 주변 표면 셀을 스캔해서 reflect 방향이 나를 향하는 표면의 wave를 수신
    if (lpLen > 0.0001)
    {
        float2 inDir = normalize(lightProjected);
        const int REFLECT_RADIUS = 6;
        float3 totalReflect = float3(0.0, 0.0, 0.0);
        float totalWeight = 0.0;

        for (int dy = -REFLECT_RADIUS; dy <= REFLECT_RADIUS; ++dy)
        {
            for (int dx = -REFLECT_RADIUS; dx <= REFLECT_RADIUS; ++dx)
            {
                if (dx == 0 && dy == 0) continue;
                int2 nc = int2(coord) + int2(dx, dy);
                if (!isInBounds(nc)) continue;

                uint nIdx = toIndex(planeIdx, uint2(nc));
                if (fieldW[nIdx] <= 0.0) continue; // 표면 셀만

                // 표면 셀의 normal을 현재 평면에 투영
                float3 n3 = normalBuffer[nIdx].xyz;
                float2 n2;
                if (planeIdx == 0) n2 = float2(n3.x, n3.y);
                else if (planeIdx == 1) n2 = float2(n3.x, n3.z);
                else n2 = float2(n3.y, n3.z);

                float n2Len = length(n2);
                if (n2Len < 0.0001) continue;
                n2 /= n2Len;

                // 반사 방향
                float2 refDir = inDir - 2.0 * dot(inDir, n2) * n2;

                // 표면 셀에서 현재 빈 셀로 향하는 방향
                float2 toHere = float2(coord) - float2(nc);
                float distVal = length(toHere);
                if (distVal < 0.0001) continue;
                float2 dirToHere = toHere / distVal;

                // 반사 방향과 얼마나 정렬되어 있는가 (dot > 0.7 = 약 45도 이내)
                float align = dot(refDir, dirToHere);
                if (align > 0.7)
                {
                    // 거리 감쇠 + 정렬 가중치
                    float weight = align / (1.0 + distVal * 0.3);
                    totalReflect += reflectedFieldXYZ[nIdx] * weight;
                    totalWeight += weight;
                }
            }
        }

        // 매 step damping 곱해서 점진 감쇠, 수신 있으면 추가 누적
        reflectedFieldXYZ[index] = reflectedFieldXYZ[index] * damping;
        if (totalWeight > 0.0)
        {
            reflectedFieldXYZ[index] = reflectedFieldXYZ[index] + (totalReflect / totalWeight) * (1.0 - damping);
        }
    }

    // 평면파 주입: 격자 내부 source line에 inject (사선 wavefront 밀려옴)
    if (lpLen > 0.0001)
    {
        float2 kDir = normalize(lightProjected);
        float2 center = float2(float(GRID_SIZE) * 0.5);
        float2 sourcePos = center - kDir * (float(GRID_SIZE) * 0.4);
        float2 relPos = float2(coord) - sourcePos;
        float distToLine = abs(dot(relPos, kDir));

        if (distToLine < 1.0)
        {
            fieldXYZ[index] = float3(1.0, 0.0, 0.0);
        }
    }
}

[shader("compute")]
[numthreads(16, 16, 1)]
void main(uint3 dispatchThreadID : SV_DispatchThreadID)
{
    // z = 평면 인덱스 (0=xy, 1=xz, 2=yz)
    uint planeIdx = dispatchThreadID.z;
    uint2 coord = dispatchThreadID.xy;
    if (coord.x >= GRID_SIZE || coord.y >= GRID_SIZE || planeIdx >= PLANE_COUNT)
        return;

    float c = scene.light.propagationSpeed;
    float damping = scene.light.damping;
    float time = scene.light.time;
    float frequency = scene.light.frequency;
    // lightDir = 광원 방향 (source), inject 로직은 진행 방향 기준이라 부호 반전
    float3 lightDir = -scene.light.direction;

    // 평면별 태양 방향 투영
    float2 lightProjected;
    if (planeIdx == 0)
        lightProjected = float2(lightDir.x, lightDir.y);
    else if (planeIdx == 1)
        lightProjected = float2(lightDir.x, lightDir.z);
    else
        lightProjected = float2(lightDir.y, lightDir.z);

    simulatePlane(planeIdx, coord, c, damping, time, frequency, lightProjected);
}
