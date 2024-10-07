#include "Humble2.h"

#include "RuntimeContext.h"

// int main(int argc, char** argv)
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	HBL2::ApplicationSpec applicationSpec;
	applicationSpec.Name = "Humble App";
	applicationSpec.VerticalSync = false;
	applicationSpec.GraphicsAPI = HBL2::GraphicsAPI::OPENGL;
	applicationSpec.Context = new HBL2Runtime::RuntimeContext;

	HBL2::Application* app = new HBL2::Application(applicationSpec);

	app->Start();

	delete app;

	return 0;
}