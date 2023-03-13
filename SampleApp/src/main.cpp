#include "Humble2.h"

int main()
{
	HBL::ApplicationSpec applicationSpec;
	applicationSpec.Name = "Sample App";
	applicationSpec.Vsync = false;
	applicationSpec.GraphicsAPI = HBL::GraphicsAPI::OpenGL;

	HBL::Application* app = new HBL::Application(applicationSpec);

	app->Start();

	delete app;

	return 0;
}