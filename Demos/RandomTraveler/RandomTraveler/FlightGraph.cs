//////////////////////////////////////////////////////////////////////////////
////
//// Copyright (c) Microsoft Corporation. All rights reserved
////
//////////////////////////////////////////////////////////////////////////////

//----------------------------------------------------------------------------
// File: FlightGraph.cs
//----------------------------------------------------------------------------

using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace RandomTraveler
{
    /// <summary>
    /// Represents an edge in flight graph
    /// </summary>
    internal class FlightEdge
    {
        public int To;
        public int Value;
    }

    internal class FlightGraph
    {
        #region Private properties

        private List<FlightEdge>[] m_edges;
        private Airport[] m_airports;
        
        /// <summary>
        /// Represents the graph in matrix form
        /// </summary>
        private volatile float[,] m_matrix;

        #endregion Private properties

        #region Public properties

        public Airport[] Airports
        {
            get { return m_airports; }
        }

        public float[,] GraphMatrix
        {
            get { return m_matrix; }
        }

        private int VertexCount
        {
            get { return m_edges.Length; }
        }

        #endregion Public properties

        #region C'tors

        public FlightGraph(int airportCount, Airport[] airports, List<FlightEdge>[] edges)
        {
            m_edges = edges;
            m_airports = airports;
            m_matrix = GenerateMatrix();
        }

        #endregion C'tors

        #region Function and Delegates

        public delegate bool VertexSelector(int vertex);

        /// <summary>
        /// Reduces the graph to contain only those vertices
        /// which satisfies vertex selector criteria.
        /// </summary>
        /// <param name="vertexSelector"></param>
        /// <returns></returns>
        public FlightGraph ReduceGraph(VertexSelector vertexSelector)
        {
            int[] newVertexIndex = new int[m_edges.Length];
            int newVertexCount = 0;
            List<Airport> newAirports = new List<Airport>();
            
            // Prepare list of vertices that satifies
            // vertex selector criteria.
            for (int i = 0; i < VertexCount; i++)
            {
                if (vertexSelector(i))
                {
                    newVertexIndex[i] = newVertexCount;
                    newVertexCount++;
                    newAirports.Add(Airports[i]);
                }
                else
                {
                    newVertexIndex[i] = -1;
                }
            }

            // Create new adjacency list with selected vertices
            var newEdges = new List<List<FlightEdge>>();

            for (int i = 0; i < m_edges.Length; i++)
            {
                if (newVertexIndex[i] != -1)
                {
                    var edgeList = new List<FlightEdge>();
                    foreach (var edge in m_edges[i])
                    {
                        if (newVertexIndex[edge.To] != -1)
                        {
                            edgeList.Add(new FlightEdge { To = newVertexIndex[edge.To], Value = edge.Value });
                        }
                    }
                    newEdges.Add(edgeList);
                }
            }

            return new FlightGraph(newVertexCount, newAirports.ToArray(), newEdges.ToArray());
        }

        /// <summary>
        /// Return the strongly connect component of graph
        /// with maximum number of vertices.
        /// </summary>
        /// <returns></returns>
        public FlightGraph LargestStronglyConnectedComponent()
        {
            int[] components = GetStronglyConnectedComponents();
            int maxComponent =
                components.GroupBy(cId => cId)
                    .Select(c => new { Component = c.Key, Size = c.Count() })
                    .OrderByDescending(g => g.Size)
                    .Select(g => g.Component)
                    .First();
            return ReduceGraph(i => components[i] == maxComponent);
        }

        private int[] GetStronglyConnectedComponents()
        {
            var detector = new SccDetector(m_edges);
            return detector.GetComponents();
        }

        /// <summary>
        /// Geberates the matrix representation of graph.
        /// </summary>
        /// <returns></returns>
        private float[,] GenerateMatrix()
        {
            int vertexCount = m_edges.Length;
            float[,] matrix = new float[vertexCount, vertexCount];

            for (int vertex = 0; vertex < vertexCount; vertex++)
            {
                int sum = 0;
                foreach (var edge in m_edges[vertex])
                {
                    sum += edge.Value;
                }

                if (sum > 0)
                {
                    foreach (var edge in m_edges[vertex])
                    {
                        matrix[vertex, edge.To] = (float)(((double)edge.Value) / sum);
                    }
                }
                else
                {
                    matrix[vertex, vertex] = 1;
                }
            }
            return matrix;
        }

        /// <summary>
        /// Detects strongly-connected components in a graph
        /// </summary>
        private class SccDetector
        {

            #region Private Properties

            private List<FlightEdge>[] m_edges;
            private List<FlightEdge>[] m_transposedEdges;
            private bool[] m_visited;
            private Stack<int> m_stack;

            #endregion Private Properties

            #region C'tors

            public SccDetector(List<FlightEdge>[] edges)
            {
                m_edges = edges;
                m_transposedEdges = Transpose(edges);
                m_visited = new bool[m_edges.Length];
            }

            #endregion C'tors

            #region Methods

            /// <summary>
            /// Gets all the strongly connect components in the graph.
            /// </summary>
            /// <returns></returns>
            public int[] GetComponents()
            {
                int vertexCount = m_edges.Length;
                Array.Clear(m_visited, 0, m_visited.Length);
                m_stack = new Stack<int>();

                for (int i = 0; i < m_edges.Length; i++)
                {
                    Recurse1(i);
                }

                int[] components = new int[vertexCount];

                Array.Clear(m_visited, 0, m_visited.Length);
                int componentId = 0;
                
                while(m_stack.Count > 0)
                {
                    int cur = m_stack.Pop();
                    if (!m_visited[cur])
                    {
                        Recurse2(cur, components, ++componentId);
                    }
                }

                return components;
            }

            /// <summary>
            /// Visists the node in depth first order and stores
            /// them in stack
            /// </summary>
            /// <param name="vertex"></param>
            private void Recurse1(int vertex)
            {
                if (m_visited[vertex]) { return; }

                m_visited[vertex] = true;

                foreach (var edge in m_edges[vertex])
                {
                    Recurse1(edge.To);
                }

                m_stack.Push(vertex);
            }

            /// <summary>
            /// Visists the nodes in transposed graph in depth first order
            /// and stored the strongly connected component id for each vertex.
            /// </summary>
            /// <param name="vertex"></param>
            /// <param name="components"></param>
            /// <param name="componentId"></param>
            private void Recurse2(int vertex, int[] components, int componentId)
            {
                if (m_visited[vertex]) { return; }

                components[vertex] = componentId;
                m_visited[vertex] = true;

                foreach (var edge in m_transposedEdges[vertex])
                {
                    Recurse2(edge.To, components, componentId);
                }
            }

            /// <summary>
            /// Generates the transpose of passed in graph.
            /// </summary>
            /// <param name="edges"></param>
            /// <returns></returns>
            private List<FlightEdge>[] Transpose(List<FlightEdge>[] edges)
            {
                List<FlightEdge>[] transposed = new List<FlightEdge>[edges.Length];

                for (int i = 0; i < transposed.Length; i++)
                {
                    transposed[i] = new List<FlightEdge>();
                }

                for(int from = 0; from < edges.Length; from++)
                {
                    foreach (var edge in edges[from])
                    {
                        var transposedEdge = new FlightEdge { To = from, Value = edge.Value };
                        transposed[edge.To].Add(transposedEdge);
                    }
                }

                return transposed;
            }

            #endregion Methods

        }

        #endregion Function and Delegates
    }
}
