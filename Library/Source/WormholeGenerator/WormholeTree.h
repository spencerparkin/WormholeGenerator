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
#include <fstream>

namespace WormholeGenerator
{
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
			int samplesPerLocation;
			int numSteps;
			double wormholeRadius;
		};

		struct RenderVertex
		{
			HappyMath::Vector3 location;
			HappyMath::Vector3 normal;
		};

		class Node : public std::enable_shared_from_this<Node>
		{
		public:
			Node();
			virtual ~Node();

			bool SaveToStream(std::ostream& outputStream) const;
			bool LoadFromStream(std::istream& inputStream, std::function<std::shared_ptr<Node>()> nodeMakerFunc);

			void AddPolygon(const HappyMath::Polygon& polygon);

			TangentPoint tangentPoint;
			std::vector<std::shared_ptr<Node>> childNodeArray;
			std::vector<RenderVertex> vertexBuffer;
			std::vector<uint32_t> indexBuffer;
			std::unordered_map<std::string, uint32_t> indexBufferMap;
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
		bool SaveToDisk(const std::string& filePath) const;
		bool LoadFromDisk(const std::string& filePath, std::function<std::shared_ptr<Node>()> nodeMakerFunc = []() { return std::make_shared<WormholeTree::Node>(); });

		Node* GetRootNode() { return this->rootNode.get(); }
		const HappyMath::PolygonMesh& GetMesh() const { return this->mesh; }

	private:

		void GeneratePolygons(const GeneratorConfig& config) const;
		void GenerateRecursive(const GeneratorConfig& config, std::shared_ptr<Node> parentNode, int currentDepth);
		void GeneratePolygonsForNode(Node* node, const GeneratorConfig& config) const;

		static void EvaluateCubicBezierCurve(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& curvePoint);
		static void EvaluateCubicBezierCurveDerivative(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& curveDerivative);
		static void EvaluateCubicBezierCurveSecondDerivative(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& secondCurveDerivative);
		static void GenerateCubicBezierControlPoints(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, HappyMath::Vector3* controlPointArray);
		static void CalcTNBFrame(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& xAxis, HappyMath::Vector3& yAxis, HappyMath::Vector3& zAxis);

		std::shared_ptr<Node> rootNode;

		HappyMath::PolygonMesh mesh;
	};
}