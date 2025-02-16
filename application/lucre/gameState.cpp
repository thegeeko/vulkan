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

#include "gameState.h"

#include "core.h"
#include "scenes/splashScene.h"
#include "scenes/mainScene.h"
#include "scenes/settingsScene.h"

namespace LucreApp
{

    GameState::GameState()
        : m_State{SPLASH}, m_InputIdle{false},
          m_UserInputEnabled{false}
    {
    }

    void GameState::Start()
    {
        m_Scenes.emplace(State::SPLASH, std::make_unique<SplashScene>("application/lucre/sceneDescriptions/splash.scene"));
        m_Scenes.emplace(State::MAIN, std::make_unique<MainScene>("application/lucre/sceneDescriptions/main.scene"));
        m_Scenes.emplace(State::SETTINGS, std::make_unique<SettingsScene>("application/lucre/sceneDescriptions/settings.scene"));

        SetState(State::SPLASH);
    }

    void GameState::Stop()
    {
        GetScene().Stop();
    }

    Scene* GameState::OnUpdate()
    {
        switch(m_State)
        {
            case State::SPLASH:
            {
                if (GetScene().IsFinished())
                {
                    SetState(State::MAIN);
                }
                break;
            }
            case State::MAIN:
            {
                if (GetScene().IsFinished())
                {
                    // game over
                    Engine::m_Engine->Shutdown();
                }
                break;
            }
            case State::SETTINGS:
            {
                if (GetScene().IsFinished())
                {
                    SetState(State::MAIN);
                }
                break;
            }
        }
        return m_Scenes[m_State].get();
    }

    void GameState::SetState(State state)
    {
        GetScene().Stop();
        m_State = state;
        GetScene().Start();
    }

    Scene& GameState::GetScene()
    {
        auto& scene_ptr = m_Scenes[m_State];
        return *scene_ptr;
    }

    void GameState::Load()
    {
    }

    void GameState::Save()
    {
    }

    void GameState::EnableUserInput(bool enable)
    {
        m_UserInputEnabled = enable;
    }
}
