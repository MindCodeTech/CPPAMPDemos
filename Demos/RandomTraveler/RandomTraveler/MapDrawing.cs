//////////////////////////////////////////////////////////////////////////////
////
//// Copyright (c) Microsoft Corporation. All rights reserved
////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: MapDrawing.cs
//----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Media;
using System.Windows.Controls;
using System.Windows.Shapes;

namespace RandomTraveler
{
    internal class MapDrawing
    {
        #region Private Properties

        private const double ONE_PERCENT_CIRCLE_DIAMETER = 12;

        // Input tables for Robinson projection
        private static double[] s_pLenTable = { 1.0000, 0.9986, 0.9954, 0.9900, 0.9822, 0.9730, 0.9600, 0.9427, 0.9216, 0.8962, 0.8679, 0.8350, 0.7986, 0.7597, 0.7186, 0.6732, 0.6213, 0.5722, 0.5322 };
        private static double[] s_pDfeTable = { 0.0000, 0.0620, 0.1240, 0.1860, 0.2480, 0.3100, 0.3720, 0.4340, 0.4958, 0.5571, 0.6176, 0.6769, 0.7346, 0.7903, 0.8435, 0.8936, 0.9394, 0.9761, 1.0000 };
        
        #endregion Private Properties

        #region Methods

        public static void ShowProbabilities(
            Canvas mapCanvas, 
            IEnumerable<Tuple<string, float>> destinations, 
            Dictionary<string, Airport> airports,
            int originX, 
            int originY, 
            double scale)
        {
            mapCanvas.Children.Clear();
            foreach (var dest in destinations)
            {
                var airport = airports[dest.Item1];
                DrawCircle(mapCanvas, 100.0 * dest.Item2, airport.Latitude, airport.Longitude, originX, originY, scale);
            }
        }

        private static void DrawCircle(
            Canvas mapCanvas, 
            double percent, 
            double lattitude, 
            double longitude,
            int originX, 
            int originY, 
            double scale)
        {
            double size = Math.Sqrt(percent) * ONE_PERCENT_CIRCLE_DIAMETER;
            double x, y;
            RobinsonProjection(lattitude, longitude, out x, out y);

            Brush brush = new SolidColorBrush(Color.FromArgb(200, 255, 0, 0));
            Ellipse ellipse = new Ellipse { Width = size, Height = size, Fill = brush };

            double xAbs = x * mapCanvas.Width * scale - 0.5 * size - originX;
            double yAbs = y * mapCanvas.Height * scale - 0.5 * size - originY;

            if (xAbs >= 0 && yAbs >= 0 && xAbs < mapCanvas.Width && yAbs < mapCanvas.Height)
            {
                Canvas.SetLeft(ellipse, xAbs);
                Canvas.SetTop(ellipse, yAbs);

                mapCanvas.Children.Add(ellipse);
            }
        }

        private static void RobinsonProjection(double lattitude, double longitude, out double x, out double y)
        {
            double absLat = Math.Abs(lattitude);
            int lo = ((int)absLat) / 5;
            int hi = lo + 1;
            if (hi > s_pLenTable.Length) hi--;

            double f = (absLat - lo * 5) / 5;
            double PLen = (1 - f) * s_pLenTable[lo] + f * s_pLenTable[hi];
            double PDfe = (1 - f) * s_pDfeTable[lo] + f * s_pDfeTable[hi];
            if (lattitude < 0) PDfe *= -1;
            y = 0.5 - PDfe * 0.5072;
            x = 0.5 + 0.5 * PLen * longitude / 180.0;
        }

        #endregion Methods
    }
}
