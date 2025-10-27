import osmnx as ox
import yaml


# CONFIGURATION
LOCATION = ["Sydney, NSW, Australia"] #["Willoughby, Sydney, NSW, Australia", "Artarmon, Sydney, NSW, Australia"]
NETWORK_TYPE = 'drive'  # 'drive', 'walk', 'bike', 'all'
OUTPUT_FILE = 'network.yaml'
NORMALIZE_RANGE = 2000.0  # Coordinates will be normalized to [-NORMALIZE_RANGE, NORMALIZE_RANGE]
SIMPLIFIED_GRAPH = True

# Download road network
G = ox.graph_from_place(LOCATION, network_type=NETWORK_TYPE, simplify=SIMPLIFIED_GRAPH)

# Create mapping from OSM IDs to sequential IDs
node_id_map = {node_id: idx for idx, node_id in enumerate(G.nodes())}

# Extract all coordinates to find min/max
lats = [data['y'] for _, data in G.nodes(data=True)]
lons = [data['x'] for _, data in G.nodes(data=True)]

min_lat, max_lat = min(lats), max(lats)
min_lon, max_lon = min(lons), max(lons)

# Calculate ranges
lat_range = max_lat - min_lat
lon_range = max_lon - min_lon

# Determine which axis to normalize
if lat_range > lon_range:
    scale = (2.0 * NORMALIZE_RANGE) / lat_range  # Changed this line
    lat_offset = (max_lat + min_lat) / 2.0
    lon_offset = (max_lon + min_lon) / 2.0
else:
    scale = (2.0 * NORMALIZE_RANGE) / lon_range  # Changed this line
    lat_offset = (max_lat + min_lat) / 2.0
    lon_offset = (max_lon + min_lon) / 2.0

# Convert to simple format with normalized coordinates
nodes = []
edges = []

for node_id, data in G.nodes(data=True):
    norm_lat = (data['y'] - lat_offset) * scale
    norm_lon = (data['x'] - lon_offset) * scale
    nodes.append({
        'x': float(norm_lon),
        'y': float(norm_lat)
    })

for u, v, data in G.edges(data=True):
    edges.append({
        'source': node_id_map[u],
        'target': node_id_map[v],
        'length': float(data.get('length', 0))
    })

# Save to YAML
graph_data = {
    'nodes': nodes,
    'edges': edges
}

with open(OUTPUT_FILE, 'w') as f:
    yaml.dump(graph_data, f, default_flow_style=False)

print(f"Saved {len(nodes)} nodes and {len(edges)} edges")
print(f"Normalized to range: x=[{min(n['x'] for n in nodes):.3f}, {max(n['x'] for n in nodes):.3f}], "
      f"y=[{min(n['y'] for n in nodes):.3f}, {max(n['y'] for n in nodes):.3f}]")
