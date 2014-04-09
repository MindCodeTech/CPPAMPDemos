//////////////////////////////////////////////////////////////////////////////
////
//// Copyright (c) Microsoft Corporation. All rights reserved
////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: MainWindow.xaml.cs
//----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;
using System.IO;
using System.Threading.Tasks;
using System.Diagnostics;

namespace RandomTraveler
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        #region C'tors

        public MainWindow()
        {
            InitializeComponent();
            InitializeImages();
        }

        #endregion C'tors

        #region Private Properties

        private BitmapImage m_worldMapImage;
        private BitmapImage m_northAmericaMapImage;

        private FlightGraph m_worldFlightGraph;
        private FlightGraph m_northAmericaFlightGraph;

        private Dictionary<string, Airport> m_airportDict;

        #endregion Private Properties

        #region Methods and Delegates

        private delegate void VoidAction();

        /// <summary>
        /// This function gets called when the 'Calculate button is clicked'.
        /// </summary>
        /// <param name="sender"></param>
        /// <param name="e"></param>
        private void Button_Calculate_Click(object sender, RoutedEventArgs e)
        {
            var flightGraph = Radio_US.IsChecked == true ? m_northAmericaFlightGraph : m_worldFlightGraph;

            CalculateResults(Text_StartingAirport.Text, flightGraph.Airports, flightGraph.GraphMatrix);
        }

        /// <summary>
        /// This is the function that kicks off the actual computation.
        /// </summary>
        /// <param name="airportName"></param>
        /// <param name="airports"></param>
        /// <param name="matrix"></param>
        private void CalculateResults(string airportName, Airport[] airports, float[,] matrix)
        {

            // Validate the airport name entered by the user.
            Airport srcAirport = airports.Where(a => a.AirportCode == airportName).FirstOrDefault();
            if (srcAirport == null)
            {
                var airportNameLo = airportName.ToLower();
                var matchingAirports = airports.Where(a => a.Name.ToLower().Contains(airportNameLo) || a.City.ToLower().Contains(airportNameLo)).Take(10).ToArray();

                if (matchingAirports.Length == 0)
                {
                    MessageBox.Show("Unknown airport: '" + airportName + "'", "Random Traveler", MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }
                else if (matchingAirports.Length == 1)
                {
                    srcAirport = matchingAirports[0];
                }
                else if (matchingAirports.Length > 1)
                {
                    string message = "Multiple matching airports:" + Environment.NewLine;
                    foreach (var match in matchingAirports)
                    {
                        message += Environment.NewLine + "    " + match.City + ", " + match.Name + " (" + match.AirportCode + ")";
                    }

                    MessageBox.Show(message, "Random Traveler", MessageBoxButton.OK, MessageBoxImage.Error);
                    return;
                }
            }

            int srcAirportIndex = -1;
            for (int i = 0; i < airports.Length; i++)
            {
                if (airports[i] == srcAirport)
                {
                    srcAirportIndex = i;
                    break;
                }
            }

            long flights;
            if (!long.TryParse(Text_Flights.Text, out flights) || flights < 0)
            {
                MessageBox.Show("'Number of Flights' exceeded the range for long type", "Random Traveler", MessageBoxButton.OK, MessageBoxImage.Error);
                return;
            }

            Button_Calculate.IsEnabled = false;

            // Choose the appropriate matrix multiplication function to invoke based on user choice.
            MatrixOperations.MultiplyAction multiplyMatrix =
                Radio_CPU_Naive.IsChecked == true ? MatrixOperations.MultiplyMatrixCpu
                : Radio_CPU_Parallel_Unsafe.IsChecked == true ? MatrixOperations.MultiplyMatrixTpl
                : Radio_GPU.IsChecked == true ? MatrixOperations.MultiplyMatrixGpu
                : (MatrixOperations.MultiplyAction)null;

            // Kick off the computation asynchronously in a separate thread.
            Task.Factory.StartNew(() =>
            {
                Stopwatch sw = Stopwatch.StartNew();

                long flopCount = 0, mmCount = 0;
                float[,] matrixExp = null;

                try
                {
                    matrixExp = MatrixOperations.MatrixExp(matrix, flights, multiplyMatrix, ref mmCount, ref flopCount);
                }
                catch (Exception ex)
                {
                    MessageBox.Show(ex.Message, "Random Traveller", MessageBoxButton.OK, MessageBoxImage.Error);
                    this.Dispatcher.Invoke((VoidAction)(() => Button_Calculate.IsEnabled = true));                    
                    return;
                }

                sw.Stop();
                long milliseconds = sw.ElapsedMilliseconds;

                var destinations = Enumerable.Range(0, airports.Length).Select(i => Tuple.Create(airports[i].AirportCode, matrixExp[srcAirportIndex, i]));

                this.Dispatcher.Invoke((VoidAction)(() => Console.WriteLine(flopCount)));

                this.Dispatcher.Invoke((VoidAction)(() =>
                {
                    if (Radio_World.IsChecked == true)
                    {
                        MapDrawing.ShowProbabilities(MapCanvas, destinations, m_airportDict, 0, 0, 1);
                    }
                    if (Radio_US.IsChecked == true)
                    {
                        MapDrawing.ShowProbabilities(MapCanvas, destinations, m_airportDict, 103, 25, 2048.0 / 800);
                    }

                    ShowMostLikelyCities(destinations);
                    ShowMeasurements(matrixExp.GetLength(0), mmCount, milliseconds, flopCount);
                    Button_Calculate.IsEnabled = true;
                }));
            });
        }

        private void ShowMostLikelyCities(IEnumerable<Tuple<string, float>> destinations)
        {
            Grid_MostLikelyCities.Children.Clear();
            Thickness margin = new Thickness(10, 5, 10, 0);
            int index = 0;
            int row = 0, col = 0;
            foreach (var dest in destinations.OrderByDescending(d => d.Item2))
            {
                index++;
                var textBlock =
                    new TextBlock
                    {
                        Text = string.Format(
                            "{0}. {1} ({2}): {3:0.0}%",
                            index,
                            m_airportDict[dest.Item1].City,
                            m_airportDict[dest.Item1].AirportCode,
                            dest.Item2 * 100),
                        FontSize = 14,
                        Foreground = Brushes.White,
                        Margin = margin,
                    };
                Grid_MostLikelyCities.Children.Add(textBlock);
                Grid.SetRow(textBlock, row);
                Grid.SetColumn(textBlock, col);

                row++;
                if (row == Grid_MostLikelyCities.RowDefinitions.Count)
                {
                    row = 0; col++;
                }
                if (col == Grid_MostLikelyCities.ColumnDefinitions.Count) break;
            }
        }

        private void ShowMeasurements(int dim, long mmCount, long milliseconds, long flopCount)
        {
            Console.WriteLine("ShowMeasurements: flopCount=" + flopCount);
            double gFlops = 1e-9 * (1000.0 * flopCount) / milliseconds;
            Stack_Measurements.Children.Clear();
            string[] lines = new[] {                
                milliseconds + "ms",
                string.Format("{0:0.00} GFLOPS", gFlops),

                "Matrix size: "+dim+ " x " +dim,
                "Matrix multiplications: " + mmCount,
            };
            int[] lineSize = new[] { 16, 16, 14, 14 };
            bool[] lineIsBold = new[] { true, true, false, false };

            for (int i = 0; i < lines.Length; i++)
            {
                Thickness margin = new Thickness(10, lineSize[i] / 4, 10, 0);
                var textBlock =
                    new TextBlock
                    {
                        Text = lines[i],
                        FontSize = lineSize[i],
                        Foreground = Brushes.White,
                        Margin = margin,
                        FontWeight = lineIsBold[i] ? FontWeights.Bold : FontWeights.Normal,
                    };
                Stack_Measurements.Children.Add(textBlock);
            }

            Border_Measurements.Visibility = Visibility.Visible;
        }

        /// <summary>
        /// Loads data from file to initialize list of Airport and graphs
        /// </summary>
        private void LoadData()
        {
            var allAirports = Airport.LoadAirportData();

            string[] lines = File.ReadAllLines("matrix.txt");

            List<FlightEdge>[] edges = Enumerable.Range(0, lines.Length).Select(i => new List<FlightEdge>()).ToArray();
            for (int i = 0; i < lines.Length; i++)
            {
                foreach (string record in lines[i].Split(new[] { " " }, StringSplitOptions.RemoveEmptyEntries))
                {
                    string[] recordParts = record.Split(',');
                    int j = int.Parse(recordParts[0]);
                    int cnt = int.Parse(recordParts[1]);
                    edges[i].Add(new FlightEdge { To = j, Value = cnt });
                }
            }
            m_worldFlightGraph = new FlightGraph(lines.Length, allAirports, edges);
            m_worldFlightGraph = m_worldFlightGraph.LargestStronglyConnectedComponent();
            m_airportDict = m_worldFlightGraph.Airports.ToDictionary(a => a.AirportCode, a => a);

            m_northAmericaFlightGraph = m_worldFlightGraph.ReduceGraph(i => m_worldFlightGraph.Airports[i].IsInUS);
            m_northAmericaFlightGraph = m_northAmericaFlightGraph.LargestStronglyConnectedComponent();
        }

        private void InitializeImages()
        {
            m_worldMapImage = InitializeImage("worldmap800.jpg");
            m_northAmericaMapImage = InitializeImage("northamericamap800.jpg");
        }

        private BitmapImage InitializeImage(string fileName)
        {
            BitmapImage bitmap = new BitmapImage();
            bitmap.BeginInit();
            bitmap.UriSource = new Uri(string.Format("pack://application:,,,/RandomTraveler;component/{0}", fileName));
            bitmap.EndInit();
            return bitmap;
        }

        private void Window_Loaded(object sender, RoutedEventArgs e)
        {
            LoadData();
            Radio_US.IsChecked = true;
            Radio_CPU_Naive.IsChecked = true;
        }

        private void Radio_MapView_Checked(object sender, RoutedEventArgs e)
        {
            Image_USMap.Visibility = Image_WorldMap.Visibility = Visibility.Collapsed;
            if (sender == Radio_World) { Image_WorldMap.Visibility = Visibility.Visible; }
            if (sender == Radio_US) { Image_USMap.Visibility = Visibility.Visible; }
            ClearResults();
        }

        private void InputsChanged(object sender, RoutedEventArgs e)
        {
            ClearResults();
        }

        private void ClearResults()
        {
            if (IsLoaded)
            {
                MapCanvas.Children.Clear();
                Border_Measurements.Visibility = Visibility.Collapsed;
                Stack_Measurements.Children.Clear();
                Grid_MostLikelyCities.Children.Clear();
            }
        }

        #endregion Methods and Delegates
    }
}
