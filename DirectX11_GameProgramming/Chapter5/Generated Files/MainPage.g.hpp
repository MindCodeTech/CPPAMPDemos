﻿

//------------------------------------------------------------------------------
//     This code was generated by a tool.
//
//     Changes to this file may cause incorrect behavior and will be lost if
//     the code is regenerated.
//------------------------------------------------------------------------------
#include "pch.h"
#include "MainPage.xaml.h"




void ::Chapter4::MainPage::InitializeComponent()
{
    if (_contentLoaded)
        return;

    _contentLoaded = true;

    // Call LoadComponent on ms-appx:///MainPage.xaml
    ::Windows::UI::Xaml::Application::LoadComponent(this, ref new ::Windows::Foundation::Uri(L"ms-appx:///MainPage.xaml"), ::Windows::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Application);

    // Get the SwapChainBackgroundPanel named 'SwapChainPanel'
    SwapChainPanel = safe_cast<::Windows::UI::Xaml::Controls::SwapChainBackgroundPanel^>(static_cast<Windows::UI::Xaml::IFrameworkElement^>(this)->FindName(L"SwapChainPanel"));
}

void ::Chapter4::MainPage::Connect(int connectionId, Platform::Object^ target)
{
    switch (connectionId)
    {
    case 1:
        (safe_cast<::Windows::UI::Xaml::FrameworkElement^>(target))->Loaded +=
            ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::Chapter4::MainPage::*)(Platform::Object^, Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnLoaded);
        (safe_cast<::Windows::UI::Xaml::FrameworkElement^>(target))->Unloaded +=
            ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::Chapter4::MainPage::*)(Platform::Object^, Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnUnloaded);
        break;
    case 2:
        (safe_cast<::Windows::UI::Xaml::Controls::Primitives::Selector^>(target))->SelectionChanged +=
            ref new ::Windows::UI::Xaml::Controls::SelectionChangedEventHandler(this, (void (::Chapter4::MainPage::*)(Platform::Object^, Windows::UI::Xaml::Controls::SelectionChangedEventArgs^))&MainPage::OnComboSelectionChanged);
        break;
    case 3:
        (safe_cast<::Windows::UI::Xaml::Controls::Primitives::ToggleButton^>(target))->Checked +=
            ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::Chapter4::MainPage::*)(Platform::Object^, Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnChecked);
        (safe_cast<::Windows::UI::Xaml::Controls::Primitives::ToggleButton^>(target))->Unchecked +=
            ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::Chapter4::MainPage::*)(Platform::Object^, Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnUnchecked);
        break;
    case 4:
        (safe_cast<::Windows::UI::Xaml::Controls::Primitives::ToggleButton^>(target))->Checked +=
            ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::Chapter4::MainPage::*)(Platform::Object^, Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnChecked);
        (safe_cast<::Windows::UI::Xaml::Controls::Primitives::ToggleButton^>(target))->Unchecked +=
            ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::Chapter4::MainPage::*)(Platform::Object^, Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnUnchecked);
        break;
    case 5:
        (safe_cast<::Windows::UI::Xaml::Controls::Primitives::ToggleButton^>(target))->Checked +=
            ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::Chapter4::MainPage::*)(Platform::Object^, Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnChecked);
        (safe_cast<::Windows::UI::Xaml::Controls::Primitives::ToggleButton^>(target))->Unchecked +=
            ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::Chapter4::MainPage::*)(Platform::Object^, Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnUnchecked);
        break;
    case 6:
        (safe_cast<::Windows::UI::Xaml::Controls::Primitives::ButtonBase^>(target))->Click +=
            ref new ::Windows::UI::Xaml::RoutedEventHandler(this, (void (::Chapter4::MainPage::*)(Platform::Object^, Windows::UI::Xaml::RoutedEventArgs^))&MainPage::OnExitBtnClicked);
        break;
    }
    (void)connectionId; // Unused parameter
    (void)target; // Unused parameter
    _contentLoaded = true;
}

