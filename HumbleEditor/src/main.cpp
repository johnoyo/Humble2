#include "Humble2.h"

#include "EditorContext.h"

int main()
{
	HBL2::ApplicationSpec applicationSpec;
	applicationSpec.Name = "Humble2 Editor";
	applicationSpec.VerticalSync = false;
	applicationSpec.GraphicsAPI = HBL2::GraphicsAPI::OPENGL;
	applicationSpec.Context = new HBL2::Editor::EditorContext;

	HBL2::Application* app = new HBL2::Application(applicationSpec);

	app->Start();

	delete app;

	return 0;
}
