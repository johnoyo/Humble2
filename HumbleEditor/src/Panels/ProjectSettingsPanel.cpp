#include "ProjectSettingsPanel.h"

#include "ImGui/ImGuiRenderer.h"
#include "Systems/EditorPanelSystem.h"

namespace HBL2::Editor
{
	ProjectSettingsPanel::ProjectSettingsPanel(const std::string& name, EditorPanelSystem* owner)
	{
		m_Owner = owner;
		Name = name;
		Enabled = false;
	}

	void ProjectSettingsPanel::OnAttach()
	{
	}

	void ProjectSettingsPanel::OnCreate()
	{
	}

	void ProjectSettingsPanel::OnOpen()
	{
	}

	void ProjectSettingsPanel::OnRender(float ts)
	{
		Scene* ctx = m_Owner->m_Context;

		ImGui::Begin(Name.c_str(), &m_CloseState);

		auto& spec = HBL2::Project::GetActive()->GetSpecification();
		const ImGuiTreeNodeFlags treeNodeFlags = ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_FramePadding | ImGuiTreeNodeFlags_AllowOverlap;

		// Renderer.
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		bool rOpened = ImGui::TreeNodeEx((void*)69420690, treeNodeFlags, "Renderer Settings");
		ImGui::PopStyleVar();

		if (rOpened)
		{
			{
				const char* options[] = { "Forward", "ForwardPlus", "Deffered", "Custom" };
				int currentItem = (int)spec.Settings.Renderer;

				if (ImGui::Combo("Renderer Type", &currentItem, options, IM_ARRAYSIZE(options)))
				{
					spec.Settings.Renderer = (RendererType)currentItem;
				}

				ImGui::SameLine();

				ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");
			}

			{
				const char* options[] = { "None", "OpenGL", "Vulkan" };
				int currentItem = (int)spec.Settings.EditorGraphicsAPI;

				if (ImGui::Combo("Editor Graphics API", &currentItem, options, IM_ARRAYSIZE(options)))
				{
					spec.Settings.EditorGraphicsAPI = (GraphicsAPI)currentItem;
				}

				ImGui::SameLine();

				ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");
			}

			{
				const char* options[] = { "None", "OpenGL", "Vulkan" };
				int currentItem = (int)spec.Settings.RuntimeGraphicsAPI;

				if (ImGui::Combo("Runtime Graphics API", &currentItem, options, IM_ARRAYSIZE(options)))
				{
					spec.Settings.RuntimeGraphicsAPI = (GraphicsAPI)currentItem;
				}
			}

			ImGui::TreePop();
		}

		// Physics 2d.
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		bool ph2dOpened = ImGui::TreeNodeEx((void*)69420691, treeNodeFlags, "Physics2D Settings");
		ImGui::PopStyleVar();

		if (ph2dOpened)
		{
			if (ImGui::DragFloat2("Gravity##2d", glm::value_ptr(spec.Settings.PhysicsEngine2DSpec.GravityForce), 0.01f))
			{
			}

			if (ImGui::Checkbox("Enable Debug Draw##2d", &spec.Settings.EnableDebugDraw2D))
			{
				if (PhysicsEngine2D::Instance != nullptr)
				{
					PhysicsEngine2D::Instance->SetDebugDrawEnabled(spec.Settings.EnableDebugDraw2D);
				}
			}

			const char* options[] = { "Custom", "Box2D" };
			int currentItem = (int)spec.Settings.Physics2DImpl;

			if (ImGui::Combo("Implementation##2d", &currentItem, options, IM_ARRAYSIZE(options)))
			{
				spec.Settings.Physics2DImpl = (Physics2DEngineImpl)currentItem;
			}

			ImGui::TreePop();
		}

		// Physics 3d.
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		bool ph3dOpened = ImGui::TreeNodeEx((void*)69420692, treeNodeFlags, "Physics3D Settings");
		ImGui::PopStyleVar();

		if (ph3dOpened)
		{
			if (ImGui::DragFloat3("Gravity##3d", glm::value_ptr(spec.Settings.PhysicsEngine3DSpec.GravityForce), 0.01f))
			{
			}

			if (ImGui::InputInt("Max Bodies", (int*)&spec.Settings.PhysicsEngine3DSpec.MaxBodies))
			{
			}

			if (ImGui::InputInt("Max Body Pairs", (int*)&spec.Settings.PhysicsEngine3DSpec.MaxBodyPairs))
			{
			}

			if (ImGui::InputInt("Max Contact Constraints", (int*)&spec.Settings.PhysicsEngine3DSpec.MaxContactConstraints))
			{
			}

			if (ImGui::Checkbox("Enable Debug Draw##3d", &spec.Settings.EnableDebugDraw3D))
			{
				if (PhysicsEngine3D::Instance != nullptr)
				{
					PhysicsEngine3D::Instance->SetDebugDrawEnabled(spec.Settings.EnableDebugDraw3D);
				}
			}

			if (spec.Settings.EnableDebugDraw3D)
			{
				if (ImGui::Checkbox("Show Colliders", &spec.Settings.ShowColliders3D))
				{
					if (PhysicsEngine3D::Instance != nullptr)
					{
						PhysicsEngine3D::Instance->ShowColliders(spec.Settings.ShowColliders3D);
					}
				}

				if (ImGui::Checkbox("Show Bounding Boxes", &spec.Settings.ShowBoundingBoxes3D))
				{
					if (PhysicsEngine3D::Instance != nullptr)
					{
						PhysicsEngine3D::Instance->ShowBoundingBoxes(spec.Settings.ShowBoundingBoxes3D);
					}
				}
			}

			const char* options[] = { "Custom", "Jolt" };
			int currentItem = (int)spec.Settings.Physics3DImpl;

			if (ImGui::Combo("Implementation##3d", &currentItem, options, IM_ARRAYSIZE(options)))
			{
				spec.Settings.Physics3DImpl = (Physics3DEngineImpl)currentItem;
			}

			ImGui::TreePop();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		bool edsOpened = ImGui::TreeNodeEx((void*)69420693, treeNodeFlags, "Editor Settings");
		ImGui::PopStyleVar();

		if (edsOpened)
		{
			if (ImGui::Checkbox("Multiple Viewports", &spec.Settings.EditorMultipleViewports))
			{
			}

			ImGui::SameLine();

			ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");

			ctx->Filter<Component::EditorCamera>()
				.ForEach([&](Entity entity, Component::EditorCamera& editorCamera)
					{
						ImGui::Text("Camera Transform:");

						auto& transform = ctx->GetComponent<HBL2::Component::Transform>(entity);

						ImGui::DragFloat3("Translation##ed", glm::value_ptr(transform.Translation), 0.25f);
						ImGui::DragFloat3("Rotation##ed", glm::value_ptr(transform.Rotation), 0.25f);

						ImGui::Separator();

						ImGui::Text("Camera View:");

						auto& camera = ctx->GetComponent<HBL2::Component::Camera>(entity);

						ImGui::SliderFloat("Near##ed", &camera.Near, 0, 10);
						ImGui::SliderFloat("Far##ed", &camera.Far, 100, 2500);
						ImGui::SliderFloat("FOV##ed", &camera.Fov, 0, 120);
						ImGui::SliderFloat("Aspect Ratio##ed", &camera.AspectRatio, 0, 3);
						ImGui::SliderFloat("Exposure##ed", &camera.Exposure, 0, 50);
						ImGui::SliderFloat("Gamma##ed", &camera.Gamma, 0, 4);
						ImGui::SliderFloat("Zoom Level##ed", &camera.ZoomLevel, 0, 500);

						ImGui::Separator();

						ImGui::Text("Camera Controls:");

						ImGui::SliderFloat("MovementSpeed##ed", &editorCamera.MovementSpeed, 0, 150);
						ImGui::SliderFloat("MouseSensitivity##ed", &editorCamera.MouseSensitivity, 0, 100);
						ImGui::SliderFloat("PanSpeed##ed", &editorCamera.PanSpeed, 0, 200);
						ImGui::SliderFloat("ZoomSpeed##ed", &editorCamera.ZoomSpeed, 0, 15);
						ImGui::SliderFloat("ScrollZoomSpeed##ed", &editorCamera.ScrollZoomSpeed, 0, 200);

						// Sync component/editor state back into spec.
						spec.Settings.EditorCameraTranslation = transform.Translation;
						spec.Settings.EditorCameraRotation = transform.Rotation;

						spec.Settings.EditorCameraNear = camera.Near;
						spec.Settings.EditorCameraFar = camera.Far;
						spec.Settings.EditorCameraFov = camera.Fov;
						spec.Settings.EditorCameraAspectRatio = camera.AspectRatio;
						spec.Settings.EditorCameraExposure = camera.Exposure;
						spec.Settings.EditorCameraGamma = camera.Gamma;
						spec.Settings.EditorCameraZoomLevel = camera.ZoomLevel;

						spec.Settings.EditorCameraMovementSpeed = editorCamera.MovementSpeed;
						spec.Settings.EditorCameraMouseSensitivity = editorCamera.MouseSensitivity;
						spec.Settings.EditorCameraPanSpeed = editorCamera.PanSpeed;
						spec.Settings.EditorCameraZoomSpeed = editorCamera.ZoomSpeed;
						spec.Settings.EditorCameraScrollZoomSpeed = editorCamera.ScrollZoomSpeed;
					});

			ImGui::TreePop();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		bool ausOpened = ImGui::TreeNodeEx((void*)69420694, treeNodeFlags, "Audio Settings");
		ImGui::PopStyleVar();

		if (ausOpened)
		{
			const char* options[] = { "Custom", "FMOD" };
			int currentItem = (int)spec.Settings.SoundImpl;

			if (ImGui::Combo("Implementation##sound", &currentItem, options, IM_ARRAYSIZE(options)))
			{
				spec.Settings.SoundImpl = (SoundEngineImpl)currentItem;
			}

			ImGui::SameLine();

			ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");

			ImGui::TreePop();
		}

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
		bool asOpened = ImGui::TreeNodeEx((void*)69420695, treeNodeFlags, "Advanced Settings");
		ImGui::PopStyleVar();

		if (asOpened)
		{
			ImGui::InputInt("Max App Memory (in MB)", (int*)&spec.Settings.MaxAppMemory);
			ImGui::SameLine();
			ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");

			ImGui::InputInt("Max Main Thread Frame Arena Memory (in MB)", (int*)&spec.Settings.MaxMainThreadFrameArenaMemory);
			ImGui::SameLine();
			ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");

			ImGui::InputInt("Max Render Thread Frame Arena Memory (in MB)", (int*)&spec.Settings.MaxRenderThreadFrameArenaMemory);
			ImGui::SameLine();
			ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");

			ImGui::InputInt("Max UniformBuffer Memory (in MB)", (int*)&spec.Settings.MaxUniformBufferMemory);
			ImGui::SameLine();
			ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");

			ImGui::Separator();

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			bool rmOpened = ImGui::TreeNodeEx((void*)typeid(ResourceManager).hash_code(), treeNodeFlags, "Resource Manager Settings");
			ImGui::PopStyleVar();

			if (rmOpened)
			{
				ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");
				ImGui::InputInt("Textures Pool Size", (int*)&spec.Settings.ResourceManagerSpec.Textures);
				ImGui::InputInt("Shaders Pool Size", (int*)&spec.Settings.ResourceManagerSpec.Shaders);
				ImGui::InputInt("Buffers Pool Size", (int*)&spec.Settings.ResourceManagerSpec.Buffers);
				ImGui::InputInt("BindGroups Pool Size", (int*)&spec.Settings.ResourceManagerSpec.BindGroups);
				ImGui::InputInt("BindGroupLayouts Pool Size", (int*)&spec.Settings.ResourceManagerSpec.BindGroupLayouts);
				ImGui::InputInt("FrameBuffers Pool Size", (int*)&spec.Settings.ResourceManagerSpec.FrameBuffers);
				ImGui::InputInt("RenderPass Pool Size", (int*)&spec.Settings.ResourceManagerSpec.RenderPass);
				ImGui::InputInt("RenderPassLayouts Pool Size", (int*)&spec.Settings.ResourceManagerSpec.RenderPassLayouts);
				ImGui::InputInt("Meshes Pool Size", (int*)&spec.Settings.ResourceManagerSpec.Meshes);
				ImGui::InputInt("Materials Pool Size", (int*)&spec.Settings.ResourceManagerSpec.Materials);
				ImGui::InputInt("Scenes Pool Size", (int*)&spec.Settings.ResourceManagerSpec.Scenes);
				ImGui::InputInt("Scripts Pool Size", (int*)&spec.Settings.ResourceManagerSpec.Scripts);
				ImGui::InputInt("Sounds Pool Size", (int*)&spec.Settings.ResourceManagerSpec.Sounds);
				ImGui::InputInt("Prefabs Pool Size", (int*)&spec.Settings.ResourceManagerSpec.Prefabs);

				ImGui::TreePop();
			}

			ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{ 4, 4 });
			bool amOpened = ImGui::TreeNodeEx((void*)typeid(AssetManager).hash_code(), treeNodeFlags, "Asset Manager Settings");
			ImGui::PopStyleVar();

			if (amOpened)
			{
				ImGui::TextColored({ 1.0f, 1.0f, 0.f, 1.0f }, "*Requires restart to take effect");
				ImGui::InputInt("Asset Pool Size", (int*)&spec.Settings.AssetManagerSpec.Assets);

				ImGui::TreePop();
			}

			ImGui::TreePop();
		}

		ImGui::NewLine();

		if (ImGui::Button("Save Project Settings"))
		{
			HBL2::Project::GetActive()->Save();
		}

		ImGui::End();
	}

	void ProjectSettingsPanel::OnClose()
	{
	}

	void ProjectSettingsPanel::OnDestroy()
	{
	}
}
