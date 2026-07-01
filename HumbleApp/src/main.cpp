#include "Humble2.h"

#include "RuntimeContext.h"

#ifdef HBL2_PLATFORM_WINDOWS
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
#else
int main(int argc, char** argv)
#endif
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
