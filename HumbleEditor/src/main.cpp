#include "Humble2.h"

#include "EditorContext.h"

int main()
{
	HBL2::ApplicationSpec applicationSpec =
	{
		.Name = "Humble2 Editor",
		.GraphicsAPI = HBL2::GraphicsAPI::OPENGL,
		.VerticalSync = false,
		.Context = new HBL2::Editor::EditorContext,
	};

	HBL2::Application* app = new HBL2::Application(applicationSpec);

	app->Start();

	delete app;

	return 0;
}
