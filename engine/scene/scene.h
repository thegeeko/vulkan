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

#pragma once

#include "engine.h"
#include "entt.hpp"
#include "scene/entity.h"
#include "events/event.h"
#include "auxiliary/timestep.h"

namespace GfxRenderEngine
{

    class Scene
    {

    public:

        Scene() : m_IsRunning{false} {}
        virtual ~Scene() {}

        virtual void Start() = 0;
        virtual void Stop() = 0;
        virtual void OnUpdate(const Timestep& timestep) = 0;
        virtual void OnEvent(Event& event) = 0;
        virtual void OnResize() = 0;

        entt::entity CreateEntity();
        void DestroyEntity(entt::entity entity);

        bool IsFinished() const { return !m_IsRunning; }
    protected:

        entt::registry m_Registry;
        bool m_IsRunning;

    };
}
