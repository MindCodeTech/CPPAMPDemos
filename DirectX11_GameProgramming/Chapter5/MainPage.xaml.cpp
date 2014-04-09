﻿//
// MainPage.xaml.cpp
// Implementation of the MainPage class.
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "FrameWork\Input\InputManager.h"

using namespace Chapter4;

using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;
using namespace Windows::Graphics::Display;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

MainPage::MainPage()
{
	InitializeComponent();

	auto coreWindow = Window::Current->CoreWindow;

	coreWindow->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &MainPage::OnSizeChanged);
	coreWindow->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &MainPage::OnVisibilityChanged);
	//Keyboard's events
	coreWindow->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &MainPage::OnKeyDown);
	coreWindow->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &MainPage::OnKeyUp);
	//Pointer's events
	coreWindow->PointerCursor = ref new CoreCursor(CoreCursorType::Arrow, 0);
	coreWindow->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &MainPage::OnPointerPressed);
	coreWindow->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &MainPage::OnPointerReleased);
	coreWindow->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &MainPage::OnPointerMoved);
	DisplayProperties::DisplayContentsInvalidated += ref new DisplayPropertiesEventHandler(this, &MainPage::OnDisplayContentsInvalidated);
	this->renderingEventToken = CompositionTarget::Rendering::add(ref new EventHandler<Object^>(this, &MainPage::OnRendering));

	this->Scene = ref new SimpleScene(coreWindow);
	this->timer = ref new Timer();
}

void MainPage::OnLoaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	this->Scene->Initialize(this->SwapChainPanel);
	this->Scene->Load();
}

void MainPage::OnUnloaded(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (!isDisposed)
	{
		isDisposed = true;
		InputManager::Unload();
		SAFE_UNLOAD(this->Scene);
	}
}

void MainPage::OnRendering(Platform::Object^ sender, Platform::Object^ args)
{
	if (!isDisposed && isWindowVisible)
	{
		this->timer->Update();
		InputManager::Update();	
		this->Scene->Update(this->timer->Total, this->timer->Delta);
		this->Scene->Render();
		this->Scene->Present();
		InputManager::ClearBuffers();
	}
	else
	{
		// App is in an inactive state so disable the OnRendering callback
		// This optimizes for power and allows the framework to become more queiecent
		if (this->renderingEventToken.Value != 0)
		{
			CompositionTarget::Rendering::remove(this->renderingEventToken);
			this->renderingEventToken.Value = 0;
		}
	}
}

void MainPage::OnDisplayContentsInvalidated(Object^ sender)
{
	this->renderingEventToken = CompositionTarget::Rendering::add(ref new EventHandler<Object^>(this, &MainPage::OnRendering));
}

void MainPage::OnSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ e)
{
	this->Scene->WindowSizeChanged();
	this->renderingEventToken = CompositionTarget::Rendering::add(ref new EventHandler<Object^>(this, &MainPage::OnRendering));
}

void MainPage::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	isWindowVisible = args->Visible;
	this->renderingEventToken = CompositionTarget::Rendering::add(ref new EventHandler<Object^>(this, &MainPage::OnRendering));
}

/// <summary>
/// Invoked when this page is about to be displayed in a Frame.
/// </summary>
/// <param name="e">Event data that describes how this page was reached.  The Parameter
/// property is typically used to configure the page.</param>
void MainPage::OnNavigatedTo(NavigationEventArgs^ e)
{
	(void) e;	// Unused parameter
}

#pragma region Keyboard

void MainPage::OnKeyDown(CoreWindow^ sender, KeyEventArgs^ args)
{
	InputManager::keyboardState.SaveKeyState(args->VirtualKey, true);
}

void MainPage::OnKeyUp(CoreWindow^ sender, KeyEventArgs^ args)
{
	InputManager::keyboardState.SaveKeyState(args->VirtualKey, false);
}

#pragma endregion

#pragma region Pointer

void MainPage::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
	InputManager::pointerState.SaveState(args->CurrentPoint->Properties);
}

void MainPage::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
	InputManager::pointerState.SaveState(args->CurrentPoint->Properties);
}

void MainPage::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
	InputManager::pointerState.SavePosition(args->CurrentPoint->Position);
}

#pragma endregion

#pragma region Controls Events

void MainPage::OnExitBtnClicked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	OnUnloaded(sender, e);
	Application::Current->Exit();
}

void MainPage::OnChecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (Scene == nullptr) return;
	auto checkBox = safe_cast<CheckBox^>(sender);
	auto tag = checkBox->Tag->ToString();

	if (tag == L"0")
	{
		this->Scene->quad->Negative = true;
	}
	else if (tag == L"1")
	{
		this->Scene->quad->HighLighter = true;
	}
	else
	{
		this->Scene->quad->OddEven = true;
	}
}


void MainPage::OnUnchecked(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
	if (Scene == nullptr) return;
	auto checkBox = safe_cast<CheckBox^>(sender);
	auto tag = checkBox->Tag->ToString();

	if (tag == L"0")
	{
		this->Scene->quad->Negative = false;
	}
	else if (tag == L"1")
	{
		this->Scene->quad->HighLighter = false;
	}
	else
	{
		this->Scene->quad->OddEven = false;
	}
}

void MainPage::OnComboSelectionChanged(Platform::Object^ sender, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^ e)
{
	if (Scene == nullptr) return;
	this->Scene->quad->PostProcessType = ! this->Scene->quad->PostProcessType;
}

#pragma endregion

