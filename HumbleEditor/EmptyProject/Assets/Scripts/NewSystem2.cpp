#include "Humble2Core.h"

class NewSystem2 final : public HBL2::ISystem
{
	glm::vec3 velocity;
	glm::vec3 hangPoint;

public:
	virtual void OnCreate() override
	{
		velocity = { 0.f, 0.f, 0.f };
		hangPoint = { 0.0f, 3.0f, 0.0f };
		
	}

	virtual void OnUpdate(float ts) override
	{
		std::cout << "NewSystem2::OnUpdate" << std::endl;
		m_Context->GetRegistry()
			.view<entt::entity>()
			.each([&](entt::entity entity)
				{
					if (m_Context->HasComponent<HBL2::Component::Transform>(entity) && !m_Context->HasComponent<HBL2::Component::Camera>(entity))
					{
						int iterations = 5;
						float subStepTime = ts / iterations;

						auto& transform = m_Context->GetComponent<HBL2::Component::Transform>(entity);

						for (int i = 0; i < iterations; i++)
						{
							velocity += glm::vec3(0, -9.81f, 0) * subStepTime;

							glm::vec3 predictedPosition = transform.Translation + velocity * subStepTime;

							// Solve constraints

							// Floor
							if (predictedPosition.y < 0.0f)
							{
								predictedPosition.y = 0.0f;
							}

							// Hang point
							if (true)
							{
								float constraint = glm::distance(predictedPosition, hangPoint) - 2.0f;

								// Gradient
								glm::vec3 gradient = -glm::normalize(predictedPosition - hangPoint);

								float lamda = -constraint / (1.0f + (0.0f / (subStepTime * subStepTime)));

								predictedPosition += lamda * gradient;
							}

							velocity = (predictedPosition - transform.Translation) / subStepTime;
							transform.Translation = predictedPosition;
						}
					}
				});
	}
};

// Factory function to create the system
extern "C" __declspec(dllexport) HBL2::ISystem* CreateSystem()
{
	return new NewSystem2();
}