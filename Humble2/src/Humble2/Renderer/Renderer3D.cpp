#include "Renderer3D.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "Utilities/tiny_obj_loader.h"

namespace HBL2
{
	void Renderer3D::Initialize(FrameBuffer* framebuffer)
	{
		m_FrameBuffer = framebuffer;
	}

	void Renderer3D::BeginFrame()
	{
		if (m_FrameBuffer != nullptr)
		{
			m_FrameBuffer->Bind();
		}

		RenderCommand::ClearScreen({ 0.2f, 0.2f, 0.2f, 1.f });
	}

	void Renderer3D::SetupMesh(Component::Transform& transform, Component::Mesh& mesh, glm::mat4& mvp)
	{
		if (!LoadFromObj(mesh))
			return;

		mesh.VertexArray = VertexArray::Create();

		VertexBufferLayout vertexBufferLayout(
		{
			{ 0, 3, Type::FLOAT, false },
			{ 1, 4, Type::FLOAT, false },
			{ 2, 2, Type::FLOAT, false },
			{ 3, 1, Type::FLOAT, false },
			{ 4, 3, Type::FLOAT, false },
		});

		mesh.VertexArray->AddVertexBuffer(VertexBuffer::Create(mesh.Data.data(), mesh.Data.size(), vertexBufferLayout));

#ifdef EMSCRIPTEN
		Shader::Create("BasicMesh", "assets/shaders/BasicMeshES.shader")->Bind();

		Shader::Get(mesh.ShaderName)->SetInt1(0, "u_Textures0");
		Shader::Get(mesh.ShaderName)->SetInt1(1, "u_Textures1");
		Shader::Get(mesh.ShaderName)->SetInt1(2, "u_Textures2");
		Shader::Get(mesh.ShaderName)->SetInt1(3, "u_Textures3");
		Shader::Get(mesh.ShaderName)->SetInt1(4, "u_Textures4");
		Shader::Get(mesh.ShaderName)->SetInt1(5, "u_Textures5");
		Shader::Get(mesh.ShaderName)->SetInt1(6, "u_Textures6");
		Shader::Get(mesh.ShaderName)->SetInt1(7, "u_Textures7");
		Shader::Get(mesh.ShaderName)->SetInt1(8, "u_Textures8");
		Shader::Get(mesh.ShaderName)->SetInt1(9, "u_Textures9");
		Shader::Get(mesh.ShaderName)->SetInt1(10, "u_Textures10");
		Shader::Get(mesh.ShaderName)->SetInt1(11, "u_Textures11");
		Shader::Get(mesh.ShaderName)->SetInt1(12, "u_Textures12");
		Shader::Get(mesh.ShaderName)->SetInt1(13, "u_Textures13");
		Shader::Get(mesh.ShaderName)->SetInt1(14, "u_Textures14");
		Shader::Get(mesh.ShaderName)->SetInt1(15, "u_Textures15");
#else
		Shader::Create("BasicMesh", "assets/shaders/BasicMesh.shader")->Bind();
		
		int samplers[32];
		for (uint32_t i = 0; i < 32; i++)
			samplers[i] = i;

		Shader::Get(mesh.ShaderName)->SetIntPtr1(samplers, 32, "u_Textures");
#endif
		Shader::Get(mesh.ShaderName)->SetMat4(mvp, "u_VP");

		Shader::Get(mesh.ShaderName)->SetMat4(transform.Matrix, "u_M");

		m_Shaders.push_back(mesh.ShaderName);
	}

	void Renderer3D::SubmitMesh(Component::Transform& transform, Component::Mesh& mesh)
	{
		glm::mat4 mvp = glm::mat4(0.f);

		if (Context::ActiveScene->MainCamera != entt::null)
			mvp = Context::ActiveScene->GetComponent<Component::Camera>(Context::ActiveScene->MainCamera).ViewProjectionMatrix;

		if (mesh.VertexArray == nullptr)
			SetupMesh(transform, mesh, mvp);

		Texture::ForEach([](Texture* texture)
		{
			texture->Bind();
		});

		mesh.VertexArray->Bind();

		for (auto& buffer : mesh.VertexArray->GetVertexBuffers())
		{
			buffer->Bind();

			Shader::Get(m_Shaders[buffer->BatchIndex])->Bind();

			Shader::Get(m_Shaders[buffer->BatchIndex])->SetMat4(mvp, "u_VP");

			Shader::Get(m_Shaders[buffer->BatchIndex])->SetMat4(transform.Matrix, "u_M");

			RenderCommand::Draw(buffer);

			Shader::Get(m_Shaders[buffer->BatchIndex])->UnBind();

			buffer->UnBind();
		}

		mesh.VertexArray->UnBind();
	}

	void Renderer3D::EndFrame()
	{
		if (m_FrameBuffer != nullptr)
		{
			m_FrameBuffer->UnBind();
		}
	}

	void Renderer3D::Clean()
	{
	}

	bool Renderer3D::LoadFromObj(Component::Mesh& mesh)
	{
		//attrib will contain the vertex arrays of the file
		tinyobj::attrib_t attrib;
		//shapes contains the info for each separate object in the file
		std::vector<tinyobj::shape_t> shapes;
		//materials contains the information about the material of each shape, but we won't use it.
		std::vector<tinyobj::material_t> materials;

		//error and warning output from the load function
		std::string warn;
		std::string err;

		//load the OBJ file
		bool result = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, mesh.Path.c_str(), nullptr);

		//make sure to output the warnings to the console, in case there are issues with the file
		if (!warn.empty())
		{
			HBL2_CORE_WARN("WARN: {0}", warn);
		}

		//if we have any error, print it to the console, and break the mesh loading.
		//This happens if the file can't be found or is malformed
		if (!err.empty())
		{
			HBL2_CORE_ERROR("ERROR: {0}", err);
			return false;
		}

		if (!result)
			return false;

		for (int i = 0; i < shapes.size(); i++) 
		{
			tinyobj::shape_t& shape = shapes[i];
			tinyobj::mesh_t& m = shape.mesh;

			for (int j = 0; j < m.indices.size(); j++) 
			{
				tinyobj::index_t i = m.indices[j];
				glm::vec3 position = 
				{
					attrib.vertices[i.vertex_index * 3],
					attrib.vertices[i.vertex_index * 3 + 1],
					attrib.vertices[i.vertex_index * 3 + 2]
				};
				glm::vec3 normal = 
				{
					attrib.normals[i.normal_index * 3],
					attrib.normals[i.normal_index * 3 + 1],
					attrib.normals[i.normal_index * 3 + 2]
				};
				glm::vec2 texCoord = 
				{
					attrib.texcoords[i.texcoord_index * 2],
					attrib.texcoords[i.texcoord_index * 2 + 1],
				};

				//copy it into our vertex
				Buffer new_vert;
				new_vert.Position = position;
				new_vert.Normal = normal;
				new_vert.Color = { normal.x, normal.y, normal.z, 1.0f };
				new_vert.TextureCoord = texCoord;
				new_vert.TextureID = 0;

				mesh.Data.push_back(new_vert);
			}
		}

		return true;
	}
}