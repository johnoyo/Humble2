#pragma once

#include <glm\glm.hpp>

namespace HBL
{
	class Texture;
	class Buffer;
	class Shader;
	class BindGroup;
	class BindGroupLayout;
	class RenderPass;
	class RenderPassLayout;

	struct TextureDescriptor
	{
		const char* debugName;
		glm::vec3 dimensions;
		uint32_t format;
		uint32_t internalFormat;
	};

	struct BufferDescriptor
	{
		const char* debugName;
		void* data = 0;
		uint32_t byteSize = 0;
		uint32_t usage = 0;
		uint32_t memory = 0;
	};

	struct FramebufferDescriptor
	{
		const char* debugName;
	};

	struct BindGroupLayoutDescriptor
	{
		const char* debugName;
		struct TextureBinding
		{
			uint32_t slot = 0;
			uint32_t visibility = 0;
		};
		std::initializer_list<TextureBinding> textureBindings;

		struct BufferBinding
		{
			uint32_t slot = 0;
			uint32_t visibility = 0;
			uint32_t type = 0;
		};
		std::initializer_list<BufferBinding> bufferBindings;
	};

	struct BindGroupDescriptor
	{
		const char* debugName;
		BindGroupLayoutDescriptor layout;
		std::initializer_list<Handle<Texture>> textures;
		struct Buffer
		{
			Handle<HBL::Buffer> buffer;
			uint32_t byteOffset = 0;
		};
		std::initializer_list<Handle<Buffer>> buffers;
	};

	struct ShaderDescriptor
	{
		const char* debugName;
		struct ShaderStage
		{
			const char* code = nullptr;
			const char* entryPoint;
		};
		ShaderStage VS;
		ShaderStage FS;
		std::initializer_list<BindGroupDescriptor> bindGroups;
		struct RenderPipeline
		{
			struct VertexBufferBinding
			{
				struct Attribute
				{
					uint32_t byteOffset = 0;
					uint32_t format = 347567;
				};

				uint32_t byteStride = 12;
				std::initializer_list<Attribute> attributes;
			};
			bool depthTest = true;
			std::initializer_list<VertexBufferBinding> vertexBufferBindings;
		};
		RenderPipeline renderPipeline;
		Handle<RenderPassLayout> renderPassLayout;
	};

	struct RenderPassDescriptor
	{
		const char* debugName;
	};

	struct RenderPassLayoutDescriptor
	{
		const char* debugName;
	};

	struct MeshDescriptor
	{
		const char* debugName;
		uint32_t indexOffset;
		uint32_t indexCount;
		uint32_t vertexOffset;
		uint32_t vertexCount;
		Handle<Buffer> indexBuffer;
		std::initializer_list<Handle<Buffer>> vertexBuffers;
	};

	struct MaterialDescriptor
	{
		const char* debugName;
		Handle<Shader> shader;
		Handle<BindGroup> bindGroup;
	};
}
