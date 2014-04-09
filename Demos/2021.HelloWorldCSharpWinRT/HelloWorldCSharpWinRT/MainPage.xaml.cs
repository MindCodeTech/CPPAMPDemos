using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Windows.Foundation;
using Windows.Foundation.Collections;
using Windows.UI.Xaml;
using Windows.UI.Xaml.Controls;
using Windows.UI.Xaml.Controls.Primitives;
using Windows.UI.Xaml.Data;
using Windows.UI.Xaml.Input;
using Windows.UI.Xaml.Media;
using Windows.UI.Xaml.Navigation;

// The Blank Page item template is documented at http://go.microsoft.com/fwlink/?LinkId=234238

namespace HelloWorldCSharpWinRT
{
    /// <summary>
    /// An empty page that can be used on its own or navigated to within a Frame.
    /// </summary>
    public sealed partial class MainPage : Page
    {
        public MainPage()
        {
            this.InitializeComponent();
        }

        /// <summary>
        /// Invoked when this page is about to be displayed in a Frame.
        /// </summary>
        /// <param name="e">Event data that describes how this page was reached.  The Parameter
        /// property is typically used to configure the page.</param>
        protected override void OnNavigatedTo(NavigationEventArgs e)
        {
        }

        private async void Button_Example_Click(object sender, RoutedEventArgs e)
        {
            Button_Example.IsEnabled = false;
            var arr = new[] { 1.0f, 2.0f, 3.0f, 4.0f };
            List<float> inputs = new List<float>(arr);

            IReadOnlyList<float> outputs = await new HelloWorldLib.WinRTComponent()
                .square_array_async(inputs);

            await new Windows.UI.Popups.MessageDialog(string.Join(",", outputs)).ShowAsync();
            Button_Example.IsEnabled = true;
        }

    }
}
