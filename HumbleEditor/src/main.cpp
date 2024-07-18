#include "Humble2.h"

#include "EditorContext.h"

// int APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdLine, int cmdshow)
int main()
{
	HBL2::ApplicationSpec applicationSpec;
	applicationSpec.Name = "Humble2 Editor";
	applicationSpec.Vsync = false;
	applicationSpec.GraphicsAPI = HBL2::GraphicsAPI::OpenGL;
	applicationSpec.Context = new HBL2::Editor::EditorContext;

	HBL2::Application* app = new HBL2::Application(applicationSpec);

	app->Start();

	delete app;

	return 0;
}
