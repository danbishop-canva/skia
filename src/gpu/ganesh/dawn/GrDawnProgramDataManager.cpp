/*
 * Copyright 2019 Google Inc.
 *
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "src/gpu/ganesh/dawn/GrDawnProgramDataManager.h"

#include "src/gpu/ganesh/dawn/GrDawnBuffer.h"
#include "src/gpu/ganesh/dawn/GrDawnGpu.h"

GrDawnProgramDataManager::GrDawnProgramDataManager(const UniformInfoArray& uniforms,
                                                   uint32_t uniformBufferSize)
    : GrUniformDataManager(uniforms.count(), uniformBufferSize) {
    memset(fUniformData.get(), 0, uniformBufferSize);
    // We must add uniforms in same order is the UniformInfoArray so that UniformHandles already
    // owned by other objects will still match up here.
    int i = 0;
    for (const auto& uniformInfo : uniforms.items()) {
        Uniform& uniform = fUniforms[i];
        SkDEBUGCODE(
            uniform.fArrayCount = uniformInfo.fVariable.getArrayCount();
            uniform.fType = uniformInfo.fVariable.getType();
        )
        uniform.fOffset = uniformInfo.fUBOOffset;
        ++i;
    }
}

static wgpu::BindGroupEntry make_bind_group_entry(uint32_t binding, const wgpu::Buffer& buffer,
                                                  uint32_t offset, uint32_t size) {
    wgpu::BindGroupEntry result;
    result.binding = binding;
    result.buffer = buffer;
    result.offset = offset;
    result.size = size;
    result.sampler = nullptr;
    result.textureView = nullptr;
    return result;
}

wgpu::BindGroup GrDawnProgramDataManager::uploadUniformBuffers(GrDawnGpu* gpu,
                                                               wgpu::BindGroupLayout layout) {
    if (fUniformsDirty && 0 != fUniformSize) {
        std::vector<wgpu::BindGroupEntry> bindings;

        GrRingBuffer::Slice slice = gpu->allocateUniformRingBufferSlice(fUniformSize);
        GrDawnBuffer* buffer = static_cast<GrDawnBuffer*>(slice.fBuffer);
        gpu->queue().WriteBuffer(buffer->get(), slice.fOffset, fUniformData.get(), fUniformSize);
        bindings.push_back(make_bind_group_entry(GrSPIRVUniformHandler::kUniformBinding,
                                                 buffer->get(), slice.fOffset,
                                                 fUniformSize));
        wgpu::BindGroupDescriptor descriptor;
        descriptor.layout = layout;
        descriptor.entryCount = bindings.size();
        descriptor.entries = bindings.data();
        fBindGroup = gpu->device().CreateBindGroup(&descriptor);
        fUniformsDirty = false;
    }
    return fBindGroup;
}
