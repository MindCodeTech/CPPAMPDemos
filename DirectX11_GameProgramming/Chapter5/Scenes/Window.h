#pragma once
/*
 * Copyright (c) 2013 Pooya Eimandar. All rights reserved.
 * The code is provided with NO WARRANTY.
 * Please direct any bugs to Pooya.Eimandar@Live.com

 * File Name        : Window.h
 * File Description : The main window
 */
#include "pch.h"
#include "SimpleScene.h"

using namespace Platform;
using namespace Windows::UI::Core;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::ApplicationModel::Activation;

ref class Window sealed : public IFrameworkView
{
private:
	bool isWindowClosed;
	bool isWindowVisible;
	SimpleScene^ Scene;

public:
	Window();

	// IFrameworkView Methods.
	virtual void Initialize(CoreApplicationView^ applicationView);
	virtual void SetWindow(CoreWindow^ window);
	virtual void Load(String^ entryPoint);
	virtual void Run();
	virtual void Uninitialize();

protected:
	// Event Handlers.
	void OnWindowSizeChanged(CoreWindow^ sender, WindowSizeChangedEventArgs^ args);
	void OnLogicalDpiChanged(Object^ sender);
	void OnActivated(CoreApplicationView^ appView, IActivatedEventArgs^ args);
	void OnSuspending(Object^ sender, SuspendingEventArgs^ args);
	void OnResuming(Object^ sender, Object^ args);
	void OnWindowClosed(CoreWindow^ sender, CoreWindowEventArgs^ args);
	void OnVisibilityChanged(CoreWindow^ sender, VisibilityChangedEventArgs^ args);
	void OnPointerPressed(CoreWindow^ sender, PointerEventArgs^ args);
	void OnPointerReleased(CoreWindow^ sender, PointerEventArgs^ args);
	void OnPointerMoved(CoreWindow^ sender, PointerEventArgs^ args);
	void OnKeyDown(CoreWindow^ sender, KeyEventArgs^ args);
	void OnKeyUp(CoreWindow^ sender, KeyEventArgs^ args);
};

ref class App sealed : IFrameworkViewSource
{
public:
	virtual IFrameworkView^ CreateView();
};

