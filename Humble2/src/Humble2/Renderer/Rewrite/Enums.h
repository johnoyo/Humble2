#pragma once

namespace HBL2
{
	enum class ShaderStage
	{
		VERTEX = 0,
		FRAGMENT = 1,
		COMPUTE = 2,
	};

	enum class BufferBindingType
	{
		UNIFORM = 0,
		STORAGE = 1,
		READ_ONLY_STORAGE = 2,
	};

	enum class VertexFormat
	{
		FLOAT32 = 0,
		FLOAT32x2 = 1,
		FLOAT32x3 = 2,
		FLOAT32x4 = 3,
	};

	enum class Compare
	{
		LESS = 1,
		LESS_OR_EQUAL = 2,
		GREATER = 3,
		GREATER_OR_EQUAL = 4,
	};

	enum class Format
	{
		RGB32_FLOAT = 1,
		D32_FLOAT = 2,
		RGBA16_FLOAT = 3,
		RGBA8_UNORM = 4,
		RG16_FLOAT = 5,
	};

	enum class Memory
	{
		CPU_CPU = 0,
		GPU_CPU = 1,
		CPU_GPU = 2,
	};

	enum class BufferUsage
	{
		MAP_READ = 1,
		MAP_WRITE = 2,
		COPY_SRC = 4,
		COPY_DST = 8,
		INDEX = 16,
		VERTEX = 32,
		UNIFORM = 64,
		STORAGE = 128,
		INDIRECT = 256,
		QUERY_RESOLVE = 512,
	};

	enum class BufferUsageHint
	{
		STATIC = 1,
		DYNAMIC = 2,
	};

	enum class TextureUsage
	{
		COPY_SRC = 1,
		COPY_DST = 2,
		TEXTURE_BINDING = 4,
		STORAGE_BINDING = 8,
		RENDER_ATTACHMENT = 16,
	};
}