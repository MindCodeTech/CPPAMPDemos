//
// App.xaml.h
// Declaration of the App class.
//

#pragma once

#include "App.g.h"
#include "MainPage.xaml.h"

namespace Chapter4
{
	/// <summary>
	/// Provides application-specific behavior to supplement the default Application class.
	/// </summary>
	ref class App sealed
	{
	public:
		App();
		virtual void OnLaunched(Windows::ApplicationModel::Activation::LaunchActivatedEventArgs^ args) override;

	private:
		MainPage^ mainPage;
		void OnSuspending(Platform::Object^ sender, Windows::ApplicationModel::SuspendingEventArgs^ e);
	};
}
