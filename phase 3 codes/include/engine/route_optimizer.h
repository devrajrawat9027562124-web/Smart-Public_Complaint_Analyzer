/*
 * route_optimizer.h
 * City graph (adjacency matrix) and Dijkstra's shortest path algorithm.
 * Models Dehradun's road network with 18 real locations.
 */

#ifndef ROUTE_OPTIMIZER_H
#define ROUTE_OPTIMIZER_H

#include "complaint_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Graph Operations ─── */

/* Initialize the city graph with Dehradun's real locations and roads */
void init_city_graph(CityGraph* graph);

/* Run Dijkstra's algorithm from source to destination.
 * Returns 1 on success, 0 if no path found. */
int dijkstra(CityGraph* graph, int source, int destination, RouteResult* result);

/* Get location name by ID */
const char* get_location_name(CityGraph* graph, int location_id);

/* Get location ID by name (case-insensitive partial match), returns -1 if not found */
int get_location_id(CityGraph* graph, const char* name);

/* Print the route result */
void print_route(CityGraph* graph, RouteResult* result, int source, int destination);

/* Get total number of locations */
int get_num_locations(CityGraph* graph);

#ifdef __cplusplus
}
#endif

#endif /* ROUTE_OPTIMIZER_H */
