//////////////////////////////////////////////////////////////////////////////
////
//// Copyright (c) Microsoft Corporation. All rights reserved
////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: Airport.cs
//----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.IO;
using System.Xml;
using System.Diagnostics;

namespace RandomTraveler
{
    internal class Airport
    {

        #region Properties

        public string AirportCode { get; private set; }
        public string City { get; private set; }
        public string CountryCode { get; private set; }
        public double Latitude { get; private set; }
        public double Longitude { get; private set; }
        public string Name { get; private set; }

        public bool IsInUS
        {
            get { return (CountryCode == "US");  }
        }

        #endregion Properties

        #region Methods

        /// <summary>
        /// Loads the airport data from xml file
        /// </summary>
        /// <returns>
        /// Array of airports sorted in ascending order by airport code.
        /// </returns>
        public static Airport[] LoadAirportData()
        {
            List<Airport> airports = new List<Airport>();
            using (FileStream file = File.Open("airports.xml", FileMode.Open))
            {
                using (XmlReader reader = XmlReader.Create(file))
                {
                    while (reader.Read())
                    {
                        if (reader.NodeType == XmlNodeType.Element && reader.Name == "AirportDetail")
                        {
                            try
                            {
                                airports.Add(new Airport
                                {
                                    AirportCode = reader["AirportCode"],
                                    City = reader["City"],
                                    CountryCode = reader["CountryCode"],
                                    Latitude = double.Parse(reader["Latitude"]),
                                    Longitude = double.Parse(reader["Longitude"]),
                                    Name = reader["Name"],
                                });
                            }
                            catch (Exception e)
                            {
                                Debug.WriteLine("Failed to parse an airport XML entry: " + e);
                            }
                        }
                    }
                }
            }

            // Sort the airport in ascending order by Airport code.
            return airports.OrderBy(a => a.AirportCode).ToArray();
        }

        #endregion Methods
    }
}
