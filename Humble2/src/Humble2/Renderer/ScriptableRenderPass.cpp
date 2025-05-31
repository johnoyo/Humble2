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
					.subMeshes = { {.vertexOffset = 0, .vertexCount = 6, } },
					.vertexBuffers = { buffer },
				}
			}
		});
	}

	DrawList ScriptableRenderPass::GetDraws()
	{
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
		const auto& shaderCode = ShaderUtilities::Get().Compile(shaderPath);

		// Reflect shader.
		const auto& reflectionData = ShaderUtilities::Get().GetReflectionData(shaderPath);

		ShaderDescriptor::RenderPipeline::Variant variant = {};
		variant.blend.enabled = false;
		variant.shaderHashKey = Random::UInt64(); // Create a random UUID since we do not have an asset to retrieve from there the UUID.

		ctx.Shader = ResourceManager::Instance->CreateShader({
			.debugName = "srp-shader",
			.VS { .code = shaderCode[0], .entryPoint = reflectionData.VertexEntryPoint.c_str() },
			.FS { .code = shaderCode[1], .entryPoint = reflectionData.FragmentEntryPoint.c_str() },
			.bindGroups = { reflectionData.BindGroupLayout },
			.renderPipeline {
				.vertexBufferBindings = {
					{
						.byteStride = reflectionData.ByteStride,
						.attributes = reflectionData.Attributes,
					},
				},
				.variants = { variant },
			},
			.renderPass = renderPass,
		});

		ResourceManager::Instance->AddShaderVariant(ctx.Shader, variant);

		// Create srp material.
		ctx.Material = ResourceManager::Instance->CreateMaterial({
			.debugName = "srp-material",
			.shader = ctx.Shader,
		});

		Material* mat = ResourceManager::Instance->GetMaterial(ctx.Material);
		mat->VariantDescriptor = variant;

		// Set the bindgroup.
		ctx.GlobalBindGroup = reflectionData.BindGroup;

		return ctx;
	}
}