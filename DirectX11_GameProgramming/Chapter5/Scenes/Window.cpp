/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Window.cpp
 * File Description : 
 */
#include "pch.h"
#include "Window.h"
#include "FrameWork\Timer.h"
#include "FrameWork\Input\InputManager.h"

using namespace Windows::System;
using namespace Windows::Foundation;

Window::Window() : isWindowClosed(false), isWindowVisible(true)
{
}

void Window::Initialize(CoreApplicationView^ appView)
{
	appView->Activated += ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &Window::OnActivated);
	CoreApplication::Suspending += ref new EventHandler<SuspendingEventArgs^>(this, &Window::OnSuspending);
	CoreApplication::Resuming += ref new EventHandler<Platform::Object^>(this, &Window::OnResuming);

}

void Window::SetWindow(CoreWindow^ window)
{
	window->SizeChanged += ref new TypedEventHandler<CoreWindow^, WindowSizeChangedEventArgs^>(this, &Window::OnWindowSizeChanged);
	window->VisibilityChanged += ref new TypedEventHandler<CoreWindow^, VisibilityChangedEventArgs^>(this, &Window::OnVisibilityChanged);
	window->Closed += ref new TypedEventHandler<CoreWindow^, CoreWindowEventArgs^>(this, &Window::OnWindowClosed);
	
	//Pointer's events
	window->PointerCursor = ref new CoreCursor(CoreCursorType::Arrow, 0);
	window->PointerPressed += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &Window::OnPointerPressed);
	window->PointerReleased += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &Window::OnPointerReleased);
	window->PointerMoved += ref new TypedEventHandler<CoreWindow^, PointerEventArgs^>(this, &Window::OnPointerMoved);
	
	//Keyboard's events
	window->KeyDown += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &Window::OnKeyDown);
	window->KeyUp += ref new TypedEventHandler<CoreWindow^, KeyEventArgs^>(this, &Window::OnKeyUp);

	this->Scene = ref new SimpleScene(CoreWindow::GetForCurrentThread());
	this->Scene->Initialize(nullptr);
}

void Window::Load(Platform::String^ entryPoint)
{
	this->Scene->Load();
}

void Window::Run()
{
	auto timer = ref new Timer();

	while (!isWindowClosed)
	{
		if (isWindowVisible)
		{
			timer->Update();
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
			InputManager::Update();	
			this->Scene->Update(timer->Total, timer->Delta);
			this->Scene->Render();
			this->Scene->Present();
			InputManager::ClearBuffers();
		}
		else
		{
			CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessOneAndAllPending);
		}
	}
}

void Window::Uninitialize()
{
	this->Scene->Unload();
	InputManager::Unload();
}

void Window::OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args)
{
	this->Scene->WindowSizeChanged();
}

void Window::OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args)
{
	this->isWindowVisible = args->Visible;
}

void Window::OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args)
{
	this->isWindowClosed = true;
}

#pragma region Keyboard

void Window::OnKeyDown(CoreWindow^ sender, KeyEventArgs^ args)
{
	InputManager::keyboardState.SaveKeyState(args->VirtualKey, true);
}

void Window::OnKeyUp(CoreWindow^ sender, KeyEventArgs^ args)
{
	InputManager::keyboardState.SaveKeyState(args->VirtualKey, false);
}

#pragma endregion

#pragma region Pointer

void Window::OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args)
{
	InputManager::pointerState.SaveState(args->CurrentPoint->Properties);
}

void Window::OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args)
{
	InputManager::pointerState.SaveState(args->CurrentPoint->Properties);
}

void Window::OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args)
{
	InputManager::pointerState.SavePosition(args->CurrentPoint->Position);
}

#pragma endregion


void Window::OnActivated(CoreApplicationView^ applicationView, IActivatedEventArgs^ args)
{
	CoreWindow::GetForCurrentThread()->Activate();
}

void Window::OnSuspending(Platform::Object^ sender, SuspendingEventArgs^ args)
{
}

void Window::OnResuming(Platform::Object^ sender, Platform::Object^ args)
{
	// Restore any data or state that was unloaded on suspend. By default, data
	// and state are persisted when resuming from suspend. Note that this event
	// does not occur if the app was previously terminated.
}

IFrameworkView^ App::CreateView()
{
	return ref new Window();
}

#ifndef XAML

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	auto app = ref new App();
	CoreApplication::Run(app);
	return 0;
}

#endif