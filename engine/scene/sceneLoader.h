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
#include "scene/scene.h"
#include "yaml-cpp/yaml.h"

namespace GfxRenderEngine
{
    class SceneLoader
    {

    public:

        enum State
        {
            UNKOWN,
            DESCRIPTION_FILE_FOUND,
            DESCRIPTION_FILE_NOT_FOUND,
            LOAD_SUCCESSFUL,
            SAVE_SUCCESSFUL,
            LOAD_FAILED,
            SAVE_FAILED
        };

    public:

        SceneLoader(Scene& scene);
        ~SceneLoader() {}

        SceneLoader::State GetState() const { return m_State; }
        void PrintState2Console();

        void Deserialize();
        void Serialize();

    private:

        YAML::Node m_YAMLNode;
        Scene& m_Scene;
        SceneLoader::State m_State;

    };
}
