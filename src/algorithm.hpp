#pragma once

#include <vector>
#include <utility>

using AdjacencyMatrix = std::vector<std::vector<std::pair<float, int>>>;

struct TraversalResult
{
	std::vector<int> TraversedEdges;
	std::vector<int> FinalEdges;
	std::vector<size_t> Memory;
};

// Warning: modification of this requires modifying shader code
enum class AlgorithmType
{
	BFS,
	DFS,
	DijkstraArray,
	DijkstraQueue,
	DEsopoPape,
	BellmanFord,
	FloydWarshall,

	Count,
};

// Warning: modification of this requires modifying shader code
static constexpr size_t AlgorithmTypeCount = (size_t)AlgorithmType::Count;

class Algorithm
{
public:
	virtual ~Algorithm() = default;

	virtual AlgorithmType GetName() const = 0;

	// Everything in here will be timed
	virtual void FindPath(const AdjacencyMatrix& graph, int start, int end) = 0;

	// This can do post processing to get the final path/result without affecting the running time.
	virtual TraversalResult GetResult() = 0;

};
