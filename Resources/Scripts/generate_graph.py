#!/usr/bin/env python3

import sys
import osmnx as ox
import yaml
import argparse

def parse_args():
    parser = argparse.ArgumentParser(description="Generate city network graphs.")
    parser.add_argument("--location", "-l", nargs="+", help="One or more location strings.")
    parser.add_argument("--network-type", "-t", choices=["drive", "walk", "bike", "all"])
    parser.add_argument("--output-file", "-o", help="Output file path.")
    parser.add_argument("--normalize-range", "-n", type=float)
    parser.add_argument("--simplified-graph", "-s", action="store_true")
    return parser.parse_args()

def main():
    try:
        args = parse_args()

        if not args.location:
            print("Error: No locations provided.", file=sys.stderr)
            return 1

        if not args.output_file:
            print("Error: No output file specified.", file=sys.stderr)
            return 1

        # Assign defaults if needed
        normalize_range = args.normalize_range or 2000.0
        simplified_graph = args.simplified_graph
        network_type = args.network_type or "drive"

        # Download road network safely
        try:
            G = ox.graph_from_place(args.location, network_type=network_type, simplify=simplified_graph)
        except Exception as e:
            print(f"Error downloading graph: {e}", file=sys.stderr)
            return 2

        if len(G.nodes) == 0:
            print(f"Error: No nodes returned for location {args.location}", file=sys.stderr)
            return 3

        # Map OSM IDs to sequential IDs
        node_id_map = {node_id: idx for idx, node_id in enumerate(G.nodes())}

        lats = [data['y'] for _, data in G.nodes(data=True)]
        lons = [data['x'] for _, data in G.nodes(data=True)]
        min_lat, max_lat = min(lats), max(lats)
        min_lon, max_lon = min(lons), max(lons)
        lat_range = max_lat - min_lat
        lon_range = max_lon - min_lon

        scale = (2.0 * normalize_range) / max(lat_range, lon_range)
        lat_offset = (max_lat + min_lat) / 2.0
        lon_offset = (max_lon + min_lon) / 2.0

        nodes = [{"x": float(data['x'] - lon_offset) * scale, 
                  "y": -float(data['y'] - lat_offset) * scale} 
                 for _, data in G.nodes(data=True)]
        edges = [{"source": node_id_map[u], "target": node_id_map[v], "length": float(data.get('length', 0)), "name": data.get('name', "")}
                 for u, v, data in G.edges(data=True)]

        graph_data = {'nodes': nodes, 'edges': edges}

        try:
            with open(args.output_file, 'w') as f:
                yaml.dump(graph_data, f, default_flow_style=False)
        except Exception as e:
            print(f"Error writing output file: {e}", file=sys.stderr)
            return 4

        print(f"Saved {len(nodes)} nodes and {len(edges)} edges")
        print(f"Normalized to range: x=[{min(n['x'] for n in nodes):.3f}, {max(n['x'] for n in nodes):.3f}], "
              f"y=[{min(n['y'] for n in nodes):.3f}, {max(n['y'] for n in nodes):.3f}]")

        return 0

    except Exception as e:
        print(f"Unexpected error: {e}", file=sys.stderr)
        return 99

if __name__ == "__main__":
    sys.exit(main())
