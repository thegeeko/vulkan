#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPositionWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragUV;
layout(location = 4) flat in int fragDiffuseTextureSlot;
layout(location = 5) in float fragAmplification;
layout(location = 6) flat in int fragUnlit;
layout(location = 7) in vec3  toCameraDirection;
layout(location = 8) flat in int fragNormalTextureSlot;

layout(location = 9) in vec3  fragTangentViewPos;
layout(location = 10) in vec3 fragTangentFragPos;
layout(location = 11) in vec3 fragTangentLightPos[10];

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
layout(set = 0, binding = 3) uniform sampler2D tex3;
layout(set = 0, binding = 4) uniform sampler2D tex4;

layout (location = 0) out vec4 outColor;

layout(push_constant) uniform Push
{
    mat4 m_ModelMatrix;
    mat4 m_NormalMatrix;
} push;

void main()
{

    vec3 ambientLightColor = ubo.m_AmbientLightColor.xyz * ubo.m_AmbientLightColor.w;

    // ---------- lighting ----------
    vec3 diffusedLightColor = vec3(0.0);
    vec3 surfaceNormal;

    // blinn phong: theta between N and H
    vec3 specularLightColor = vec3(0.0, 0.0, 0.0);

    for (int i = 0; i < ubo.m_NumberOfActiveLights; i++)
    {
        if (fragNormalTextureSlot == 4)
        {
            PointLight light = ubo.m_PointLights[i];

            // normal in tangent space
            surfaceNormal = normalize(texture(tex4,fragUV).xyz * 2 - vec3(1.0, 1.0, 1.0));
            vec3 directionToLight     = fragTangentLightPos[i] - fragTangentFragPos;
            float distanceToLight     = length(directionToLight);
            float attenuation = 1.0 / (distanceToLight * distanceToLight);
            
            // ---------- diffused ----------
            float cosAngleOfIncidence = max(dot(surfaceNormal, normalize(directionToLight)), 0.0);
            vec3 intensity = light.m_Color.xyz * light.m_Color.w * attenuation;
            diffusedLightColor += intensity * cosAngleOfIncidence;
            
            // ---------- specular ----------
            if (cosAngleOfIncidence != 0.0)
            {
                vec3 incidenceVector      = - normalize(directionToLight);
                vec3 directionToCamera    = normalize(toCameraDirection);
                vec3 reflectedLightDir    = reflect(incidenceVector, surfaceNormal);
            
                // phong
                //float specularFactor      = max(dot(reflectedLightDir, directionToCamera),0.0);
                // blinn phong
                vec3 halfwayDirection     = normalize(-incidenceVector + directionToCamera);
                float specularFactor      = max(dot(surfaceNormal, halfwayDirection),0.0);
            
                float specularReflection  = pow(specularFactor, 128);
                vec3  intensity = light.m_Color.xyz * light.m_Color.w * attenuation;
                specularLightColor += intensity * specularReflection;
            }
        }
        else
        {
            PointLight light = ubo.m_PointLights[i];

            // normal in world space
            surfaceNormal = normalize(fragNormalWorld);
            vec3 directionToLight     = light.m_Position.xyz - fragPositionWorld;
            float distanceToLight     = length(directionToLight);
            float attenuation = 1.0 / (distanceToLight * distanceToLight);

            // ---------- diffused ----------
            float cosAngleOfIncidence = max(dot(surfaceNormal, normalize(directionToLight)), 0.0);
            vec3 intensity = light.m_Color.xyz * light.m_Color.w * attenuation;
            diffusedLightColor += intensity * cosAngleOfIncidence;

            // ---------- specular ----------
            if (cosAngleOfIncidence != 0.0)
            {
                vec3 incidenceVector      = - normalize(directionToLight);
                vec3 directionToCamera    = normalize(toCameraDirection);
                vec3 reflectedLightDir    = reflect(incidenceVector, surfaceNormal);

                // phong
                //float specularFactor      = max(dot(reflectedLightDir, directionToCamera),0.0);
                // blinn phong
                vec3 halfwayDirection     = normalize(-incidenceVector + directionToCamera);
                float specularFactor      = max(dot(surfaceNormal, halfwayDirection),0.0);

                float specularReflection  = pow(specularFactor, 128);
                vec3  intensity = light.m_Color.xyz * light.m_Color.w * attenuation;
                specularLightColor += intensity * specularReflection;
            }
        }
    }
    // ------------------------------

    vec3 pixelColor;
    float alpha = 1.0;
    if (fragDiffuseTextureSlot > 0)
    {
        // {0.0, 1.0} - {1.0, 1.0}
        // |        /            |
        // {0.0, 0.0} - {1.0, 0.0}

        if (fragDiffuseTextureSlot == 1)
        {
            alpha = texture(tex1,fragUV).w;
            pixelColor = texture(tex1,fragUV).xyz;
        }
        if (fragDiffuseTextureSlot == 3)
        {
            alpha = texture(tex3,fragUV).w;
            pixelColor = texture(tex3,fragUV).xyz;
        }
        if (alpha == 0) discard;
        if (fragUnlit != 0)
        {
            diffusedLightColor = vec3(1.0, 1.0, 1.0);
            specularLightColor = vec3(0.0, 0.0, 0.0);
        }
        pixelColor *= fragAmplification;
    }
    else
    {
        pixelColor = fragColor.xyz;
    }

    outColor.xyz = ambientLightColor*pixelColor.xyz + (diffusedLightColor  * pixelColor.xyz) + specularLightColor;
    
    // reinhard tone mapping
    outColor.xyz = outColor.xyz / (outColor.xyz + vec3(1.0));

    outColor.w = alpha;

}
