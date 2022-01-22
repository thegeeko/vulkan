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

#include <memory>
#include <vector>

#include "engine.h"
#include "renderer/renderer.h"

#include "VKdevice.h"
#include "VKswapChain.h"

namespace GfxRenderEngine
{
    class VK_Window;
    class VK_Renderer : public Renderer
    {

    public:

        VK_Renderer(VK_Window* window, std::shared_ptr<VK_Device> device);
        ~VK_Renderer() override;

        VK_Renderer(const VK_Renderer&) = delete;
        VK_Renderer& operator=(const VK_Renderer&) = delete;

        VkRenderPass GetSwapChainRenderPass() const { return m_SwapChain->GetRenderPass(); }
        bool FrameInProgress() const { return m_FrameInProgress; }

        VkCommandBuffer GetCurrentCommandBuffer() const;

        VkCommandBuffer BeginFrame();
        void EndFrame();
        void BeginSwapChainRenderPass(VkCommandBuffer commandBuffer);
        void EndSwapChainRenderPass(VkCommandBuffer commandBuffer);
        
        virtual void BeginScene() override;
        virtual void Submit() override;
        virtual void EndScene() override;

    private:
    
        void CreateCommandBuffers();
        void FreeCommandBuffers();
        void RecreateSwapChain();

    private:
    
        VK_Window* m_Window;
        std::shared_ptr<VK_Device> m_Device;

        std::unique_ptr<VK_SwapChain> m_SwapChain;
        std::vector<VkCommandBuffer> m_CommandBuffers;
        VkCommandBuffer m_CurrentCommandBuffer;

        uint m_CurrentImageIndex;
        int m_CurrentFrameIndex;
        bool m_FrameInProgress;

    };
}
