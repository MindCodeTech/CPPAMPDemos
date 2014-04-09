//
// App.xaml.cpp
// Implementation of the App class.
//

#include "pch.h"
#include "App.xaml.h"

using namespace Chapter4;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::UI::Xaml;


// The Blank Application template is documented at http://go.microsoft.com/fwlink/?LinkId=234227

/// <summary>
/// Initializes the singleton application object.  This is the first line of authored code
/// executed, and as such is the logical equivalent of main() or WinMain().
/// </summary>
App::App()
{
	InitializeComponent();
	Suspending += ref new SuspendingEventHandler(this, &App::OnSuspending);
}

/// <summary>
/// Invoked when the application is launched normally by the end user.  Other entry points
/// will be used when the application is launched to open a specific file, to display
/// search results, and so forth.
/// </summary>
/// <param name="args">Details about the launch request and process.</param>
void App::OnLaunched(LaunchActivatedEventArgs^ args)
{
	//Note that Page element must be root of visual tree when its content is SwapChainBackgroundPanel element.
	this->mainPage = ref new MainPage();

	if (args->PreviousExecutionState == ApplicationExecutionState::Terminated)
	{
		throw ref new Exception(0, "Previous execution state was terminated");
	}

	// Place the page in the current window and ensure that it is active.
	Window::Current->Content = this->mainPage;
	Window::Current->Activate();
}

/// <summary>
/// Invoked when application execution is being suspended.  Application state is saved
/// without knowing whether the application will be terminated or resumed with the contents
/// of memory still intact.
/// </summary>
/// <param name="sender">The source of the suspend request.</param>
/// <param name="e">Details about the suspend request.</param>
void App::OnSuspending(Object^ sender, SuspendingEventArgs^ e)
{
	(void) sender;	// Unused parameter
	(void) e;	// Unused parameter
}

