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

#include <string>
#include <memory>

#include "engine.h"
#include "VKdevice.h"

struct PipelineConfigInfo
{
    
};

class VK_Pipeline
{

public:

    VK_Pipeline(std::shared_ptr<VK_Device> device,
                const std::string& filePathVertexShader_SPV, 
                const std::string& filePathFragmentShader_SPV,
                const PipelineConfigInfo& spec);
    ~VK_Pipeline();

    VK_Pipeline(const VK_Pipeline&) = delete;
    void operator=(const VK_Pipeline&) = delete;

    static PipelineConfigInfo DefaultPipelineConfigInfo(uint width, uint height);

private:

    static std::vector<char> readFile(const std::string& filepath);
    void CreateGraphicsPipeline(const std::string& filePathVertexShader_SPV,
                                const std::string& filePathFragmentShader_SPV,
                                const PipelineConfigInfo& spec);
    void CreateShaderModule(const std::vector<char>& code, VkShaderModule* shaderModule);
private:

    std::shared_ptr<VK_Device> m_Device;
    VkPipeline m_GraphicsPipeline;
    VkShaderModule vertShaderModule;
    VkShaderModule fragShaderModule;

};
