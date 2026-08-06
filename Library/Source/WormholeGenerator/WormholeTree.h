#pragma once

#include "HappyMath/Vector3.h"
#include "HappyMath/Random.h"
#include "HappyMath/LineSegment.h"
#include "HappyMath/Polygon.h"
#include "HappyMath/Graph.h"
#include "HappyMath/PolygonMesh.h"
#include <memory>
#include <vector>
#include <functional>

namespace WormholeGenerator
{
	struct SurfacePoint
	{
		HappyMath::Vector3 location;
		HappyMath::Vector3 normal;
	};

	/**
	 * These are cubic Bezier curves chained together so that we get
	 * continuity of the derivative across boundaries, but we're also
	 * a tree.  This allows the curve to branch.
	 */
	class WormholeTree
	{
	private:
		friend class Traveler;

	public:
		class Node;

		WormholeTree();
		virtual ~WormholeTree();

		struct TangentPoint
		{
			HappyMath::Vector3 location;
			HappyMath::Vector3 unitDirection;
		};

		class SurfacePointGeneratorConfig
		{
		public:
			SurfacePointGeneratorConfig();

			int samplesPerLocation;
			int numSteps;
			double wormholeRadius;
		};

		class AutoCompleteEdgesConfig
		{
		public:
			AutoCompleteEdgesConfig();

			double localityRadius;
			int maxDegree;
		};

		class GeneratorConfig
		{
		public:
			GeneratorConfig();

			HappyMath::Random* random;
			int maxDepth;
			double maxAngleDeviation;
			TangentPoint initialTangentPoint;
			double branchProbability;
			int maxBranchFactor;
			double minDistBetweenNodes;
			double maxDistBetweenNodes;
			SurfacePointGeneratorConfig surfacePointConfig;
			AutoCompleteEdgesConfig autoCompleteEdgesConfig;
		};

		class Node : public std::enable_shared_from_this<Node>
		{
		public:
			Node();
			virtual ~Node();

			TangentPoint tangentPoint;
			std::vector<std::shared_ptr<Node>> childNodeArray;

			// STPTODO: Add array of polygon indices associated with the node.  This way, the renderer need only render this node and the next rather than everything all the time.
		};

		class ProgressReporterInterface
		{
		public:
			ProgressReporterInterface() {}
			virtual ~ProgressReporterInterface() {}

			virtual void BeginTask(const std::string& message) = 0;
			virtual void TaskUpdate(double progress) = 0;
			virtual void EndTask() = 0;
		};

		void Clear();
		bool Generate(const GeneratorConfig& config, ProgressReporterInterface* progressReporter = nullptr);
		void ForEachRenderLine(int linesPerCurve, std::function<void(const HappyMath::LineSegment&)> renderFunc) const;
		void ForEachNode(std::function<void(const Node*)> nodeFunc) const;
		
		// STPTODO: Need a way to serialize everything in/out from/to disk.

		const std::vector<WormholeGenerator::SurfacePoint>& GetSurfacePointArray() const { return this->surfacePointArray; }
		const HappyMath::Graph& GetGraph() const { return this->graph; }
		const std::set<HappyMath::Graph::UnorderedEdge, HappyMath::Graph::UnorderedEdge>& GetEdgeSet() const { return this->edgeSet; }
		const HappyMath::PolygonMesh& GetMesh() const { return this->mesh; }

	private:

		void GenerateSurfacePoints(const SurfacePointGeneratorConfig& config, std::function<void(const SurfacePoint&)> pointFunc) const;
		void GenerateRecursive(const GeneratorConfig& config, std::shared_ptr<Node> parentNode, int currentDepth);
		void GenerateSurfacePointsForNode(const Node* node, const SurfacePointGeneratorConfig& config, std::function<void(const SurfacePoint&)> pointFunc) const;
		
		static void EvaluateCubicBezierCurve(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& curvePoint);
		static void EvaluateCubicBezierCurveDerivative(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& curveDerivative);
		static void EvaluateCubicBezierCurveSecondDerivative(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& secondCurveDerivative);
		static void GenerateCubicBezierControlPoints(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, HappyMath::Vector3* controlPointArray);
		static void FindClosestPointOnCubicBezierCurve(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, const HappyMath::Vector3& point, HappyMath::Vector3& closestPoint);

		std::shared_ptr<Node> rootNode;
		std::vector<WormholeGenerator::SurfacePoint> surfacePointArray;
		HappyMath::Graph graph;
		std::set<HappyMath::Graph::UnorderedEdge, HappyMath::Graph::UnorderedEdge> edgeSet;
		HappyMath::PolygonMesh mesh;

		// STPTODO: Add normal buffer and UV buffer.
	};
}