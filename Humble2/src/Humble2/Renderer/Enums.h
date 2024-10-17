#pragma once

namespace HBL2
{
	enum class CommandBufferType
	{
		MAIN = 0,
		OFFSCREEN = 1,
		UI = 2,
	};

	enum class ShaderStage
	{
		VERTEX = 0x1,
		FRAGMENT = 0x2,
		COMPUTE = 0x4,
	};

	enum class BufferBindingType
	{
		UNIFORM = 0,
		UNIFORM_DYNAMIC_OFFSET = 1,
		STORAGE = 2,
		READ_ONLY_STORAGE = 3,
	};

	enum class VertexFormat
	{
		FLOAT32 = 0,
		FLOAT32x2 = 1,
		FLOAT32x3 = 2,
		FLOAT32x4 = 3,
		INT32 = 4,
		INT32x2 = 5,
		INT32x3 = 6,
		INT32x4 = 7,
		UINT32 = 8,
		UINT32x2 = 9,
		UINT32x3 = 10,
		UINT32x4 = 11,
	};

	enum class Compare
	{
		LESS = 1,
		LESS_OR_EQUAL = 2,
		GREATER = 3,
		GREATER_OR_EQUAL = 4,
		EQUAL = 5,
		NOT_EQUAL = 6,
		ALAWAYS = 7,
		NEVER = 8,
	};

	enum class Wrap
	{
		CLAMP = 1,
	};

	enum class Format
	{
		RGB32_FLOAT = 1,
		D32_FLOAT = 2,
		RGBA16_FLOAT = 3,
		RGBA8_UNORM = 4,
		RG16_FLOAT = 5,
		RGBA8_RGB = 6
	};

	enum class MemoryUsage
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
	
	enum class TextureType
	{
		D1 = 0,
		D2 = 1,
		D3 = 2,
	};

	enum class TextureAspect
	{
		NONE = 0,
		COLOR = 1,
		DEPTH = 2,
		STENCIL = 3,
	};

	enum class TextureUsage
	{
		COPY_SRC = 1,
		COPY_DST = 2,
		TEXTURE_BINDING = 4,
		STORAGE_BINDING = 8,
		RENDER_ATTACHMENT = 16,
		SAMPLED = 32,
		DEPTH_STENCIL = 64,
	};

	enum class TextureLayout
	{
		UNDEFINED = 0,
		COPY_SRC = 1,
		COPY_DST = 2,
		RENDER_ATTACHMENT = 4,
		DEPTH_STENCIL = 8,
		PRESENT = 16,
	};

	enum class BlendOperation
	{
		ADD = 0,
		MUL = 1,
		SUB = 2,
	};

	enum class BlendFactor
	{
		SRC_ALPHA = 0,
		ONE_MINUS_SRC_ALPHA = 1,
	};

	enum class StoreOperation
	{
		STORE = 1,
		DONT_CARE = 2,
	};

	enum class LoadOperation
	{
		CLEAR = 0,
		LOAD = 1,
		DONT_CARE = 2,
	};
}