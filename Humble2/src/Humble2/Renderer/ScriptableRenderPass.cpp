#include "ScriptableRenderPass.h"

#include "Utilities\ShaderUtilities.h"

namespace HBL2
{
	static Handle<Mesh> g_FullScreenQuad = {};

	static Handle<Mesh> GetOrCreateFullScreenQuad()
	{
		if (g_FullScreenQuad.IsValid())
		{
			return g_FullScreenQuad;
		}

		float* vertexBuffer = new float[24]
		{
			-1.0, -1.0, 0.0, 1.0, 1.0, -1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 0.0,
			 1.0,  1.0, 1.0, 0.0,-1.0,  1.0, 0.0, 0.0,-1.0,-1.0, 0.0, 1.0,
		};

		auto buffer = ResourceManager::Instance->CreateBuffer({
			.debugName = "srp-quad-vertex-buffer",
			.usage = BufferUsage::VERTEX,
			.byteSize = sizeof(float) * 24,
			.initialData = vertexBuffer,
		});

		g_FullScreenQuad = ResourceManager::Instance->CreateMesh({
			.debugName = "srp-quad-mesh",
			.meshes = {
				{
					.debugName = "srp-quad-mesh-part",
					.subMeshes = { { .vertexOffset = 0, .vertexCount = 6, } },
					.vertexBuffers = { buffer },
				}
			}
		});
	}

	DrawList ScriptableRenderPass::GetDraws()
	{
		// TODO: Fix draw list handling, we need an arena.
		DrawList drawList;

		drawList.Insert({
			.Shader = m_RenderPassContext.Shader,
			// .Mesh = GetOrCreateFullScreenQuad(),
			.Material = m_RenderPassContext.Material,
		});

		return drawList;
	}

	RenderPassContext ScriptableRenderPass::CreateContext(const char* shaderPath, Handle<RenderPass> renderPass)
	{
		RenderPassContext ctx = {};

		// Create srp shaders.
		ShaderReflectionData outReflectionData;
		const auto& compilationData = ShaderUtilities::Get().Compile(shaderPath, &outReflectionData);

		ShaderDescriptor::RenderPipeline::PackedVariant variant = {};
		variant.blendEnabled = false;

		ctx.Shader = ResourceManager::Instance->CreateShader({
			.debugName = "srp-shader",
			.VS { .code = compilationData.vertexShaderCode.AsSpan(), .entryPoint = outReflectionData.entryPoints[0].name.c_str() },
			.FS { .code = compilationData.fragmentShaderCode.AsSpan(), .entryPoint = outReflectionData.entryPoints[1].name.c_str() },
			.bindGroups = { {}/*reflectionData.BindGroupLayout*/ },
			.renderPipeline {
				.vertexBufferBindings = outReflectionData.vertexBufferBindings,
				.variants = { variant },
			},
			.renderPass = renderPass,
		});

		ResourceManager::Instance->GetOrAddShaderVariant(ctx.Shader, variant);

		// Create srp material.
		ctx.Material = ResourceManager::Instance->CreateMaterial({
			.debugName = "srp-material",
			.shader = ctx.Shader,
		});

		Material* mat = ResourceManager::Instance->GetMaterial(ctx.Material);
		mat->VariantHash = variant;

		// Set the bindgroup.
		ctx.GlobalBindGroup = {}/*reflectionData.BindGroup*/;

		return ctx;
	}
}