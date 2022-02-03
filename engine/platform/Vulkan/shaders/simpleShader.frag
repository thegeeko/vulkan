#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPositionWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragUV;
layout(location = 4) flat in int  fragTextureSlot;

struct PointLight
{
    vec4 m_Position;  // ignore w
    vec4 m_Color;     // w is intensity
};

layout(set = 0, binding = 0) uniform GlobalUniformBuffer
{
    mat4 m_Projection;
    mat4 m_View;

    // point light
    vec4 m_AmbientLightColor;
    PointLight m_PointLights[10];
    int m_NumberOfActiveLights;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D tex1;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform Push
{
    mat4 m_ModelMatrix;
    mat4 m_NormalMatrix;
} push;

void main()
{

    vec3 diffusedLightColor = ubo.m_AmbientLightColor.xyz * ubo.m_AmbientLightColor.w;
    vec3 surfaceNormal = normalize(fragNormalWorld);

    for (int i = 0; i < ubo.m_NumberOfActiveLights; i++)
    {
        PointLight light = ubo.m_PointLights[i];
        vec3 directionToLight = light.m_Position.xyz - fragPositionWorld;
        float attenuation = 1.0 / dot(directionToLight, directionToLight);
        float cosAngleOfIncidence = max(dot(surfaceNormal, normalize(directionToLight)), 0);
        vec3 intensity = light.m_Color.xyz * light.m_Color.w * attenuation;
        diffusedLightColor += intensity * cosAngleOfIncidence;
    }

    if (fragTextureSlot > 0)
    {
        // {0.0, 1.0} - {1.0, 1.0}
        // |        /            |
        // {0.0, 0.0} - {1.0, 0.0}

        vec3 color = texture(tex1,fragUV).xyz;
        outColor = vec4(color,1.0f);
    }
    else
    {
        outColor.x = diffusedLightColor.x * fragColor.x;
        outColor.y = diffusedLightColor.y * fragColor.y;
        outColor.z = diffusedLightColor.z * fragColor.z;
    }

    outColor.w = 1.0;

}
