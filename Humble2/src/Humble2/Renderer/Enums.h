#pragma once

namespace HBL2
{
	enum class RenderPassEvent
	{
		BeforeRendering = 0,
		BeforeRenderingShadows,
		AfterRenderingShadows,
		BeforeRenderingPrePasses,
		AfterRenderingPrePasses,
        BeforeRenderingGeometry,
        AfterRenderingGeometry,
        BeforeRenderingOpaques,
        AfterRenderingOpaques,
		BeforeRenderingSkybox,
		AfterRenderingSkybox,
		BeforeRenderingTransparents,
		AfterRenderingTransparents,
		BeforeRenderingPostProcess,
		AfterRenderingPostProcess,
		BeforeDebugRendering,
		AfterDebugRendering,
		BeforePresenting,
		AfterRendering,
	};

	enum class RenderPassStage
	{
		Shadow,
		PrePass,
		Opaque,
		Skybox,
		Transparent,
		PostProcess,
		Present,
		UserInterface,
	};

	enum class CommandBufferType
	{
		MAIN = 0,
		UI = 1,
	};

	enum class PipelineStage
	{
		TOP_OF_PIPE = 0x00000001,
		DRAW_INDIRECT = 0x00000002,
		VERTEX_INPUT = 0x00000004,
		VERTEX_SHADER = 0x00000008,
		TESSELLATION_CONTROL_SHADER = 0x00000010,
		TESSELLATION_EVALUATION_SHADER = 0x00000020,
		GEOMETRY_SHADER = 0x00000040,
		FRAGMENT_SHADER = 0x00000080,
		EARLY_FRAGMENT_TESTS = 0x00000100,
		LATE_FRAGMENT_TESTS = 0x00000200,
		COLOR_ATTACHMENT_OUTPUT = 0x00000400,
		COMPUTE_SHADER = 0x00000800,
		TRANSFER = 0x00001000,
		BOTTOM_OF_PIPE = 0x00002000,
		HOST = 0x00004000,
		ALL_GRAPHICS = 0x00008000,
		ALL_COMMANDS = 0x00010000,
		NONE = 0,
	};

	enum class ShaderType
	{
		RASTERIZATION = 0,
		COMPUTE = 1,
	};

	enum class ShaderStage
	{
		NONE = 0,
		VERTEX = 1,
		FRAGMENT = 2,
		COMPUTE = 4,
	};

	enum class BufferBindingType
	{
		UNIFORM = 0,
		UNIFORM_DYNAMIC_OFFSET = 1,
		STORAGE = 2,
		READ_ONLY_STORAGE = 3,
		STORAGE_IMAGE = 4,
	};

	enum class TextureBindingType
	{
		IMAGE_SAMPLER = 0,
		STORAGE_IMAGE = 1,
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
		NONE,
	};

	enum class TextureFilter
	{
		NEAREST = 0,
		LINEAR = 1,
		CUBIC = 2,
	};

	enum class Compare
	{
		LESS = 0,
		LESS_OR_EQUAL,
		GREATER,
		GREATER_OR_EQUAL,
		EQUAL,
		NOT_EQUAL,
		ALAWAYS,
		NEVER,
	};

	enum class Wrap
	{
		REPEAT = 0,
		REPEAT_MIRRORED = 1,
		CLAMP_TO_EDGE = 2,
		CLAMP_TO_BORDER = 3,
		MIRROR_CLAMP_TO_EDGE = 4,
	};

	enum class Topology
	{
		POINT_LIST = 0,
		LINE_LIST = 1,
		LINE_STRIP = 2,
		TRIANGLE_LIST = 3,
		TRIANGLE_STRIP = 4,
		TRIANGLE_FAN = 5,
		PATCH_LIST = 6,
	};

	enum class PolygonMode
	{
		FILL = 0,
		LINE = 1,
		POINT = 2,
	};

	enum class CullMode
	{
		NONE = 0,
		FRONT = 0x00000001,
		BACK = 0x00000002,
		FRONT_AND_BACK = 0x00000003,
	};

	enum class FrontFace
	{
		COUNTER_CLOCKWISE = 0,
		CLOCKWISE = 1,
	};

	enum class Format
	{
		RGB32_FLOAT = 0x00000001,
		RGBA32_FLOAT = 0x00000002,
		D16_FLOAT = 0x00000004,
		D24_FLOAT = 0x00000008,
		D32_FLOAT = 0x00000010,
		RGBA16_FLOAT = 0x00000020,
		RGBA8_UNORM = 0x00000040,
		BGRA8_UNORM = 0x00000080,
		RG16_FLOAT = 0x00000100,
		RGBA8_RGB = 0x00000200,
		R10G10B10A2_UNORM = 0x00000400,
        
		BC1_RGB_UNORM = 0x00000800,
		BC1_RGB_SRGB = 0x00001000,
		BC1_RGBA_UNORM = 0x00002000,
		BC1_RGBA_SRGB = 0x00004000,
		BC3_UNORM = 0x00008000,
		BC3_SRGB = 0x00010000,
		BC7_UNORM = 0x00020000,
		BC7_SRGB = 0x00040000,
		BC6H_UF = 0x00080000,

        ASTC_4x4_SRGB = 0x00100000,
        ASTC_4x4_UNORM = 0x00200000,
        ASTC_5x5_SRGB = 0x00400000,
        ASTC_5x5_UNORM = 0x00800000,
        ASTC_6x6_SRGB = 0x01000000,
        ASTC_6x6_UNORM = 0x02000000,
        ASTC_8x8_SRGB = 0x04000000,
        ASTC_8x8_UNORM = 0x08000000,
        ASTC_10x10_SRGB = 0x10000000,
        ASTC_10x10_UNORM = 0x20000000,
        ASTC_12x12_SRGB = 0x40000000,
        ASTC_12x12_UNORM = 0x80000000,
	};

	enum class MemoryUsage
	{
		CPU_ONLY = 0,
		GPU_ONLY = 1,
		GPU_CPU = 2,
		CPU_GPU = 3,
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
		CUBE = 3,
		D2_ARRAY = 4,
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
		SHADER_READ_ONLY = 8,
        DEPTH_STENCIL_READ_ONLY = 16,
		DEPTH_STENCIL_ATTACHMENT = 32,
		PRESENT = 64,
		GENERAL = 128,
        EGENERIC_READ = 256,
	};

	enum class BlendOperation
	{
		ADD = 0,
		MUL = 1,
		SUB = 2,
		MIN = 3,
		MAX = 4,
	};

	enum class BlendFactor
	{
		SRC_ALPHA = 0,
		ONE_MINUS_SRC_ALPHA = 1,
		ONE = 2,
		ZERO = 3,
	};

	enum class BlendMode
	{
		OPAQUE_MODE = 0,
		ALPHA_MODE = 1,
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
