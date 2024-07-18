#pragma once

#include "Types.h"
#include "Handle.h"

namespace HBL2
{
	class Renderer
	{
	public:
		static inline Renderer* Instance;

		virtual ~Renderer() = default;

		virtual void Initialize() = 0;
		virtual void BeginFrame() = 0;
		virtual void SetPipeline(Handle<Material> material) = 0;
		virtual void SetBuffers(Handle<Mesh> mesh) = 0;
		virtual void SetBufferData(Handle<Buffer> buffer, void* newData) = 0;
		virtual void SetBindGroups(Handle<Material> material) = 0;
		virtual void Draw(Handle<Mesh> mesh, Handle<Material> material) = 0;
		virtual void DrawIndexed(Handle<Mesh> mesh, Handle<Material> material) = 0;
		virtual void EndFrame() = 0;
		virtual void Clean() = 0;

		virtual void ResizeFrameBuffer(uint32_t width, uint32_t height) = 0;
		virtual void* GetDepthAttachment() = 0;
		virtual void* GetColorAttachment() = 0;

		Handle<FrameBuffer> FrameBufferHandle;
	};
}