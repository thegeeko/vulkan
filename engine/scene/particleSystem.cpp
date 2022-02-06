/* Engine Copyright (c) 2022 Engine Development Team 
   https://github.com/beaumanvienna/gfxRenderEngine

   Permission is hereby granted, free of charge, to any person
   obtaining a copy of this software and associated documentation files
   (the "Software"), to deal in the Software without restriction,
   including without limitation the rights to use, copy, modify, merge,
   publish, distribute, sublicense, and/or sell copies of the Software,
   and to permit persons to whom the Software is furnished to do so,
   subject to the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. */

#include "core.h"
#include "auxiliary/random.h"
#include "scene/particleSystem.h"

namespace GfxRenderEngine
{
    ParticleSystem::ParticleSystem(uint poolSize)
        : m_ParticlePool{poolSize}, m_PoolIndex{0}
    {
        ASSERT(poolSize);
    }

    void ParticleSystem::Emit(const ParticleSystem::Specification& spec, const ParticleSystem::Specification& variation)
    {
        Particle& particle = m_ParticlePool[m_PoolIndex];

        particle.m_Velocity          = spec.m_Velocity + variation.m_Velocity.x * EngineCore::RandomPlusMinusOne();
        particle.m_Acceleration      = spec.m_Acceleration;

        particle.m_RotationSpeed     = spec.m_RotationSpeed;

        particle.m_StartColor        = spec.m_StartColor;
        particle.m_FinalColor        = spec.m_FinalColor;

        particle.m_StartSize         = spec.m_StartSize;
        particle.m_FinalSize         = spec.m_FinalSize;

        particle.m_LifeTime          = spec.m_LifeTime;
        particle.m_RemainingLifeTime = particle.m_LifeTime;

        particle.m_Enabled = true;

        m_PoolIndex = (m_PoolIndex + 1) % m_ParticlePool.size();

        Builder builder{};
        particle.m_Entity = m_Registry.create();

        builder.LoadParticle(spec.m_StartColor);
        auto model = Engine::m_Engine->LoadModel(builder);
        MeshComponent mesh{"particle", model};
        m_Registry.emplace<MeshComponent>(particle.m_Entity, mesh);

        TransformComponent transform{};
        transform.m_Translation = glm::vec3{spec.m_Position.x, spec.m_Position.y, 3.0f};
        transform.m_Scale = glm::vec3{1.0f} * particle.m_StartSize;
        transform.m_Rotation = glm::vec3{0.0f, 0.0f, spec.m_Rotation};
        m_Registry.emplace<TransformComponent>(particle.m_Entity, transform);
    }

    void ParticleSystem::OnUpdate(Timestep timestep)
    {
        for (auto& particle : m_ParticlePool)
        {
            if (!particle.m_Enabled)
            {
                continue;
            }

            if (particle.m_RemainingLifeTime <= 0.0ms)
            {
                particle.m_Enabled = false;
                continue;
            }

            auto& transform = m_Registry.get<TransformComponent>(particle.m_Entity);
            particle.m_Velocity += particle.m_Acceleration * static_cast<float>(timestep);
            transform.m_Translation.x += particle.m_Velocity.x * static_cast<float>(timestep);
            transform.m_Translation.y += particle.m_Velocity.y * static_cast<float>(timestep);

            transform.m_Rotation.z += particle.m_RotationSpeed * static_cast<float>(timestep);
            particle.m_RemainingLifeTime -= timestep;

            auto remainingLifeTime = static_cast<float>(particle.m_RemainingLifeTime);
            auto lifeTime = static_cast<float>(particle.m_LifeTime);
            auto normalizedRemainingLifeTime = remainingLifeTime / lifeTime;

            glm::vec4 color = glm::lerp(particle.m_FinalColor, particle.m_StartColor, normalizedRemainingLifeTime);

            float size = glm::lerp(particle.m_FinalSize, particle.m_StartSize, normalizedRemainingLifeTime);
            transform.m_Scale.x = size;
            transform.m_Scale.y = size;

            auto& mesh = m_Registry.get<MeshComponent>(particle.m_Entity);
        }
    }
}
