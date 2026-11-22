/*
 * route_optimizer.c
 * Dijkstra's shortest path algorithm on Dehradun's city graph (18 locations).
 * Adjacency matrix representation with weighted undirected edges.
 */

#include "../../include/engine/route_optimizer.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/* ─── Initialize the Dehradun City Graph ─── */
void init_city_graph(CityGraph* graph) {
    /* Reset adjacency matrix */
    for (int i = 0; i < MAX_LOCATIONS; i++) {
        for (int j = 0; j < MAX_LOCATIONS; j++) {
            graph->adj_matrix[i][j] = 0;
        }
    }
    
    /* Define 18 Dehradun locations */
    graph->num_locations = 18;
    
    const char* loc_names[] = {
        "Clock Tower",             /* 0  */
        "Paltan Bazaar",           /* 1  */
        "Rajpur Road",             /* 2  */
        "ISBT Dehradun",           /* 3  */
        "Dehradun Railway Stn",    /* 4  */
        "Race Course",             /* 5  */
        "Dalanwala",               /* 6  */
        "Clement Town",            /* 7  */
        "Prem Nagar",              /* 8  */
        "Sahastradhara",           /* 9  */
        "Ballupur Chowk",          /* 10 */
        "GMS Road",                /* 11 */
        "Nagar Nigam",             /* 12 */
        "Doon Hospital",           /* 13 */
        "Survey Chowk",            /* 14 */
        "Rispana Bridge",          /* 15 */
        "Collectorate",            /* 16 */
        "IT Park Sahastradhara"    /* 17 */
    };
    
    for (int i = 0; i < 18; i++) {
        graph->locations[i].id = i;
        strncpy(graph->locations[i].name, loc_names[i],
                sizeof(graph->locations[i].name) - 1);
        graph->locations[i].name[sizeof(graph->locations[i].name) - 1] = '\0';
    }
    
    /* Define edges (distances in km, approximate real-world values) */
    /* Helper: set undirected edge */
    #define EDGE(a, b, w) do { \
        graph->adj_matrix[a][b] = w; \
        graph->adj_matrix[b][a] = w; \
    } while(0)
    
    /* Clock Tower connections */
    EDGE(0, 1, 1);     /* Clock Tower ↔ Paltan Bazaar: 1 km */
    EDGE(0, 2, 3);     /* Clock Tower ↔ Rajpur Road: 3 km */
    EDGE(0, 4, 2);     /* Clock Tower ↔ Railway Station: 2 km */
    EDGE(0, 12, 1);    /* Clock Tower ↔ Nagar Nigam: 1 km */
    EDGE(0, 13, 2);    /* Clock Tower ↔ Doon Hospital: 2 km */
    EDGE(0, 15, 2);    /* Clock Tower ↔ Rispana Bridge: 2 km */
    
    /* Paltan Bazaar connections */
    EDGE(1, 4, 1);     /* Paltan Bazaar ↔ Railway Station: 1 km */
    EDGE(1, 5, 2);     /* Paltan Bazaar ↔ Race Course: 2 km */
    EDGE(1, 12, 1);    /* Paltan Bazaar ↔ Nagar Nigam: 1 km */
    
    /* Rajpur Road connections */
    EDGE(2, 5, 2);     /* Rajpur Road ↔ Race Course: 2 km */
    EDGE(2, 10, 4);    /* Rajpur Road ↔ Ballupur Chowk: 4 km */
    EDGE(2, 6, 3);     /* Rajpur Road ↔ Dalanwala: 3 km */
    
    /* ISBT connections */
    EDGE(3, 4, 3);     /* ISBT ↔ Railway Station: 3 km */
    EDGE(3, 8, 4);     /* ISBT ↔ Prem Nagar: 4 km */
    EDGE(3, 11, 3);    /* ISBT ↔ GMS Road: 3 km */
    
    /* Railway Station connections */
    EDGE(4, 14, 2);    /* Railway Stn ↔ Survey Chowk: 2 km */
    
    /* Race Course connections */
    EDGE(5, 6, 2);     /* Race Course ↔ Dalanwala: 2 km */
    EDGE(5, 16, 2);    /* Race Course ↔ Collectorate: 2 km */
    
    /* Dalanwala connections */
    EDGE(6, 10, 3);    /* Dalanwala ↔ Ballupur Chowk: 3 km */
    EDGE(6, 16, 2);    /* Dalanwala ↔ Collectorate: 2 km */
    
    /* Clement Town connections */
    EDGE(7, 8, 3);     /* Clement Town ↔ Prem Nagar: 3 km */
    EDGE(7, 14, 4);    /* Clement Town ↔ Survey Chowk: 4 km */
    EDGE(7, 11, 5);    /* Clement Town ↔ GMS Road: 5 km */
    
    /* Prem Nagar connections */
    EDGE(8, 11, 3);    /* Prem Nagar ↔ GMS Road: 3 km */
    
    /* Sahastradhara connections */
    EDGE(9, 10, 5);    /* Sahastradhara ↔ Ballupur Chowk: 5 km */
    EDGE(9, 17, 2);    /* Sahastradhara ↔ IT Park: 2 km */
    
    /* Ballupur Chowk connections */
    EDGE(10, 17, 6);   /* Ballupur Chowk ↔ IT Park: 6 km */
    
    /* GMS Road connections */
    EDGE(11, 14, 3);   /* GMS Road ↔ Survey Chowk: 3 km */
    
    /* Nagar Nigam connections */
    EDGE(12, 13, 1);   /* Nagar Nigam ↔ Doon Hospital: 1 km */
    EDGE(12, 16, 2);   /* Nagar Nigam ↔ Collectorate: 2 km */
    
    /* Doon Hospital connections */
    EDGE(13, 15, 1);   /* Doon Hospital ↔ Rispana Bridge: 1 km */
    
    /* Survey Chowk connections */
    EDGE(14, 15, 2);   /* Survey Chowk ↔ Rispana Bridge: 2 km */
    
    #undef EDGE
}

/* ─── Dijkstra's Shortest Path Algorithm ─── */
int dijkstra(CityGraph* graph, int source, int destination, RouteResult* result) {
    int n = graph->num_locations;
    
    if (source < 0 || source >= n || destination < 0 || destination >= n) {
        return 0;
    }
    
    int dist[MAX_LOCATIONS];
    int visited[MAX_LOCATIONS];
    int prev[MAX_LOCATIONS];
    
    /* Initialize */
    for (int i = 0; i < n; i++) {
        dist[i] = INF;
        visited[i] = 0;
        prev[i] = -1;
    }
    dist[source] = 0;
    
    /* Main loop */
    for (int count = 0; count < n; count++) {
        /* Find minimum distance unvisited node */
        int u = -1;
        int min_dist = INF;
        for (int v = 0; v < n; v++) {
            if (!visited[v] && dist[v] < min_dist) {
                min_dist = dist[v];
                u = v;
            }
        }
        
        if (u == -1) break;             /* No reachable unvisited node */
        if (u == destination) break;    /* Reached destination */
        
        visited[u] = 1;
        
        /* Relax neighbors */
        for (int v = 0; v < n; v++) {
            if (!visited[v] && graph->adj_matrix[u][v] > 0) {
                int new_dist = dist[u] + graph->adj_matrix[u][v];
                if (new_dist < dist[v]) {
                    dist[v] = new_dist;
                    prev[v] = u;
                }
            }
        }
    }
    
    /* Check if destination is reachable */
    if (dist[destination] == INF) {
        result->path_length = 0;
        result->total_distance = -1;
        return 0;
    }
    
    /* Reconstruct path by backtracking from destination */
    int path_stack[MAX_LOCATIONS];
    int stack_size = 0;
    int current = destination;
    
    while (current != -1) {
        path_stack[stack_size++] = current;
        current = prev[current];
    }
    
    /* Reverse into result */
    result->path_length = stack_size;
    result->total_distance = dist[destination];
    for (int i = 0; i < stack_size; i++) {
        result->path[i] = path_stack[stack_size - 1 - i];
    }
    
    return 1;
}

/* ─── Get location name by ID ─── */
const char* get_location_name(CityGraph* graph, int location_id) {
    if (location_id >= 0 && location_id < graph->num_locations) {
        return graph->locations[location_id].name;
    }
    return "Unknown";
}

/* ─── Get location ID by name (case-insensitive partial match) ─── */
int get_location_id(CityGraph* graph, const char* name) {
    for (int i = 0; i < graph->num_locations; i++) {
        /* Case-insensitive substring search */
        const char* loc = graph->locations[i].name;
        int name_len = (int)strlen(name);
        int loc_len = (int)strlen(loc);
        
        if (name_len == 0) continue;
        
        for (int j = 0; j <= loc_len - name_len; j++) {
            int match = 1;
            for (int k = 0; k < name_len; k++) {
                if (tolower((unsigned char)loc[j + k]) !=
                    tolower((unsigned char)name[k])) {
                    match = 0;
                    break;
                }
            }
            if (match) return i;
        }
    }
    return -1;
}

/* ─── Print Route ─── */
void print_route(CityGraph* graph, RouteResult* result, int source, int destination) {
    if (result->path_length == 0) {
        printf("  No route found from %s to %s\n",
               get_location_name(graph, source),
               get_location_name(graph, destination));
        return;
    }
    
    printf("  Route: ");
    for (int i = 0; i < result->path_length; i++) {
        printf("%s", get_location_name(graph, result->path[i]));
        if (i < result->path_length - 1) {
            printf(" -> ");
        }
    }
    printf("\n  Total Distance: %d km\n", result->total_distance);
}

/* ─── Get number of locations ─── */
int get_num_locations(CityGraph* graph) {
    return graph->num_locations;
}
