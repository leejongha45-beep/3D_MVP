// QuantumWave Renderer - 렌더링 Compute Shader (2D Shadow Map 쿼리)

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
RWStructuredBuffer<float4> waveField;

[[vk::binding(3, 0)]]
RWStructuredBuffer<float4> waveVelocity; // xyz=RGB속도, w=roughness

static const uint GRID_SIZE = 1024;

int2 worldToGrid(float3 worldPos)
{
    return int2(
        int(worldPos.x + float(GRID_SIZE) / 2.0),
        int(worldPos.z + float(GRID_SIZE) / 2.0)
    );
}

bool isInGrid(int2 coord)
{
    return coord.x >= 0 && coord.x < int(GRID_SIZE)
        && coord.y >= 0 && coord.y < int(GRID_SIZE);
}

uint toIndex(uint2 coord)
{
    return coord.y * GRID_SIZE + coord.x;
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

    // NDC 좌표
    float2 uv = (float2(texCoord) / float2(width, height)) * 2.0 - 1.0;
    uv.y = -uv.y;
    float aspectRatio = float(width) / float(height);
    uv.x *= aspectRatio;

    // 카메라 레이 생성
    float3 camForward = scene.camera.forward;
    float3 camRight = normalize(cross(camForward, scene.camera.up));
    float3 camUp = cross(camRight, camForward);

    float fovScale = tan(radians(scene.camera.fov) * 0.5);
    float3 rayOrigin = scene.camera.position;
    float3 rayDirection = normalize(camForward + uv.x * fovScale * camRight + uv.y * fovScale * camUp);

    // 바닥(y = planeY)과의 교차점
    float planeY = scene.plane.position.y;
    float4 color = float4(0.0, 0.0, 0.0, 1.0);

    if (abs(rayDirection.y) > 0.0001)
    {
        float t = (planeY - rayOrigin.y) / rayDirection.y;
        if (t > 0.0)
        {
            float3 hitPoint = rayOrigin + rayDirection * t;

            // 히트 지점을 2D 격자 좌표로 변환
            int2 gridCoord = worldToGrid(hitPoint);

            if (isInGrid(gridCoord))
            {
                uint index = toIndex(uint2(gridCoord));
                float4 field = waveField[index];

                float3 brightness = abs(field.xyz);
                brightness = clamp(brightness, float3(0.0, 0.0, 0.0), float3(1.0, 1.0, 1.0));
                color = float4(brightness, 1.0);
            }
        }
    }

    outputImage[texCoord] = color;
}
