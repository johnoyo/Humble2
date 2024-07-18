#include "Humble2.h"

#include "RuntimeContext.h"

int main(int argc, char** argv)
{
	HBL2::ApplicationSpec applicationSpec;
	applicationSpec.Name = "Sample App";
	applicationSpec.Vsync = false;
	applicationSpec.GraphicsAPI = HBL2::GraphicsAPI::OpenGL;
	applicationSpec.Context = new HBL2Runtime::RuntimeContext;

	HBL2::Application* app = new HBL2::Application(applicationSpec);

	app->Start();

	delete app;

	return 0;
}