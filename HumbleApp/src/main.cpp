#include "Humble2.h"

#include "RuntimeContext.h"

// int main(int argc, char** argv)
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	HBL2::ApplicationSpec applicationSpec =
	{
		.Name = "Humble App",
		.VerticalSync = false,
		.Context = new HBL2::Runtime::RuntimeContext,
	};

	HBL2::Application* app = new HBL2::Application(applicationSpec);

	app->Start();

	delete app;

	return 0;
}