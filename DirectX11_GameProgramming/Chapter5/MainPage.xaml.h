//
// MainPage.xaml.h
// Declaration of the MainPage class.
//

#pragma once

#include "MainPage.g.h"
#include "FrameWork\Timer.h"
#include "Scenes\SimpleScene.h"

using namespace Windows::UI::Core;

namespace Chapter4
{
	/// <summary>
	/// An empty page that can be used on its own or navigated to within a Frame.
	/// </summary>
	public ref class MainPage sealed
	{
	private:
		bool isDisposed;
		bool isWindowVisible;
		Windows::Foundation::EventRegistrationToken  renderingEventToken;
		SimpleScene^ Scene;
		Timer^ timer;

	public:
		MainPage();

	protected:
		virtual void OnNavigatedTo(Windows::UI::Xaml::Navigation::NavigationEventArgs^ e) override;
	private:
		void OnDisplayContentsInvalidated(Object^ sender);
		void OnLoaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnUnloaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnRendering(Platform::Object^ sender, Platform::Object^ args);
		void OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args);
		void OnSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ e);
		void OnKeyDown(CoreWindow^ sender, KeyEventArgs^ args);
		void OnKeyUp(CoreWindow^ sender, KeyEventArgs^ args);
		void OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args);
		void OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args);
		void OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args);
		void OnExitBtnClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnChecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnUnchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
		void OnComboSelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e);
	};
}
