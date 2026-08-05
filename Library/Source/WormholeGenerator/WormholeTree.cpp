#include "WormholeGenerator/WormholeTree.h"
#include "HappyMath/Ray.h"
#include <unordered_set>
#include <math.h>

using namespace WormholeGenerator;

//--------------------------------- WormholeTree ---------------------------------

WormholeTree::WormholeTree()
{
}

/*virtual*/ WormholeTree::~WormholeTree()
{
}

void WormholeTree::Clear()
{
	this->rootNode.reset();
}

bool WormholeTree::Generate(const GeneratorConfig& config)
{
	if (!config.random)
		return false;

	this->Clear();

	this->rootNode = std::make_shared<Node>();
	this->rootNode->tangentPoint = config.initialTangentPoint;

	this->GenerateRecursive(config, this->rootNode, 0);

	return true;
}

void WormholeTree::GenerateRecursive(const GeneratorConfig& config, std::shared_ptr<Node> parentNode, int currentDepth)
{
	if (currentDepth > config.maxDepth)
		return;

	int branchFactor = 1;

	if (config.maxBranchFactor > 1 && config.random->InRange(0.0, 1.0) < config.branchProbability)
	{
		branchFactor = config.random->InRange(2, config.maxBranchFactor);
	}

	for (int i = 0; i < branchFactor; i++)
	{
		std::shared_ptr<Node> childNode = std::make_shared<Node>();

		double distance = config.random->InRange(config.minDistBetweenNodes, config.maxDistBetweenNodes);

		HappyMath::Vector3 unitDirection;
		unitDirection.SetAsRandomDirectionInCone(*config.random, parentNode->tangentPoint.unitDirection, config.maxAngleDeviation);

		childNode->tangentPoint.location = parentNode->tangentPoint.location + unitDirection * distance;
		childNode->tangentPoint.unitDirection.SetAsRandomDirectionInCone(*config.random, unitDirection, config.maxAngleDeviation);

		parentNode->childNodeArray.push_back(childNode);

		this->GenerateRecursive(config, childNode, currentDepth + 1);
	}
}

/*static*/ void WormholeTree::GenerateCubicBezierControlPoints(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, HappyMath::Vector3* controlPointArray)
{
	controlPointArray[0] = tangentPointA.location;
	controlPointArray[3] = tangentPointB.location;

	double length = (controlPointArray[3] - controlPointArray[0]).Length() / 2.0;

	controlPointArray[1] = controlPointArray[0] + tangentPointA.unitDirection * length;
	controlPointArray[2] = controlPointArray[3] - tangentPointB.unitDirection * length;
}

/*static*/ void WormholeTree::EvaluateCubicBezierCurve(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& curvePoint)
{
	HappyMath::Vector3 point[4];
	GenerateCubicBezierControlPoints(tangentPointA, tangentPointB, point);

	double t = curveParameter;
	double t2 = t * t;
	double t3 = t2 * t;
	double omt = 1.0 - t;
	double omt2 = omt * omt;
	double omt3 = omt2 * omt;

	curvePoint = omt3 * point[0] + 3.0 * omt2 * t * point[1] + 3.0 * omt * t2 * point[2] + t3 * point[3];
}

/*static*/ void WormholeTree::EvaluateCubicBezierCurveDerivative(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& curveDerivative)
{
	HappyMath::Vector3 point[4];
	GenerateCubicBezierControlPoints(tangentPointA, tangentPointB, point);

	double t = curveParameter;
	double t2 = t * t;
	double omt = 1.0 - t;
	double omt2 = omt * omt;

	curveDerivative = 3.0 * omt2 * (point[1] - point[0]) + 6.0 * omt * t * (point[2] - point[1]) + 3.0 * t2 * (point[3] - point[2]);
}

/*static*/ void WormholeTree::EvaluateCubicBezierCurveSecondDerivative(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& secondCurveDerivative)
{
	HappyMath::Vector3 point[4];
	GenerateCubicBezierControlPoints(tangentPointA, tangentPointB, point);

	double t = curveParameter;
	double omt = 1.0 - t;

	secondCurveDerivative = 6.0 * omt * (point[2] - 2.0 * point[1] + point[0]) + 6.0 * t * (point[3] - 2.0 * point[2] + point[1]);
}

/*static*/ void WormholeTree::FindClosestPointOnCubicBezierCurve(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, const HappyMath::Vector3& point, HappyMath::Vector3& closestPoint)
{
	int stepsPerPass = 10;
	double minCurveParameter = 0.0;
	double maxCurveParameter = 1.0;
	double epsilon = 0.01;

	while ((maxCurveParameter - minCurveParameter) > epsilon)
	{
		double smallestSquareDistance = std::numeric_limits<double>::max();

		int j = -1;

		for (int i = 0; i < stepsPerPass; i++)
		{
			double curveParameter = minCurveParameter + (double(i) / double(stepsPerPass - 1)) * (maxCurveParameter - minCurveParameter);

			HappyMath::Vector3 curvePoint;
			EvaluateCubicBezierCurve(tangentPointA, tangentPointB, curveParameter, curvePoint);

			double squareDistance = (curvePoint - point).SquareLength();
			if (squareDistance < smallestSquareDistance)
			{
				smallestSquareDistance = squareDistance;
				closestPoint = curvePoint;
				j = i;
			}
		}

		double newMin = minCurveParameter + (double(j - 1) / double(stepsPerPass - 1)) * (maxCurveParameter - minCurveParameter);
		double newMax = minCurveParameter + (double(j + 1) / double(stepsPerPass - 1)) * (maxCurveParameter - minCurveParameter);

		minCurveParameter = newMin;
		maxCurveParameter = newMax;
	}
}

void WormholeTree::ForEachRenderLine(int linesPerCurve, std::function<void(const HappyMath::LineSegment&)> renderFunc) const
{
	this->ForEachNode([linesPerCurve, renderFunc](const Node* node) -> void
		{
			const TangentPoint& tangentPointA = node->tangentPoint;

			for (int i = 0; i < (int)node->childNodeArray.size(); i++)
			{
				const TangentPoint& tangentPointB = node->childNodeArray[i]->tangentPoint;

				HappyMath::LineSegment line;
				int k = 0;

				for (int j = 0; j <= linesPerCurve; j++)
				{
					double curveParameter = double(j) / double(linesPerCurve);

					EvaluateCubicBezierCurve(tangentPointA, tangentPointB, curveParameter, line.point[k]);

					k = 1 - k;

					if (j > 0)
						renderFunc(line);
				}
			}
		});
}

void WormholeTree::ForEachNode(std::function<void(const Node*)> nodeFunc) const
{
	if (!this->rootNode.get())
		return;

	std::vector<std::shared_ptr<Node>> nodeArray;
	nodeArray.push_back(this->rootNode);

	while (nodeArray.size() > 0)
	{
		std::shared_ptr<Node> node = nodeArray[0];

		if (nodeArray.size() > 1)
			nodeArray[0] = nodeArray[nodeArray.size() - 1];

		nodeArray.pop_back();

		nodeFunc(node.get());

		for (int i = 0; i < (int)node->childNodeArray.size(); i++)
			nodeArray.push_back(node->childNodeArray[i]);
	}
}

void WormholeTree::PopulateGraphWithSurfacePoints(HappyMath::Graph& graph) const
{
	this->GenerateSurfacePoints([&graph](const SurfacePoint& surfacePoint) -> void
		{
			auto node = new HappyMath::Graph::Node();
			node->SetVertex(surfacePoint.location);
			node->SetNormal(surfacePoint.normal);
			graph.AddNode(node);
		});
}

void WormholeTree::GenerateSurfacePoints(std::function<void(const SurfacePoint&)> pointFunc) const
{
	this->ForEachNode([this, pointFunc](const Node* node) -> void
		{
			this->GenerateSurfacePointsForNode(node, pointFunc);
		});
}

void WormholeTree::GenerateSurfacePointsForNode(const Node* node, std::function<void(const SurfacePoint&)> pointFunc) const
{
	int samplesPerLocation = 32;
	int numSteps = 16;
	double wormholeRadius = 0.5;
	
	for (int i = 0; i < (int)node->childNodeArray.size(); i++)
	{
		const Node* childNodeA = node->childNodeArray[i].get();

		for (int j = 0; j < numSteps; j++)
		{
			double curveParameter = double(j) / double(numSteps - 1);

			HappyMath::Vector3 curvePoint;
			EvaluateCubicBezierCurve(node->tangentPoint, childNodeA->tangentPoint, curveParameter, curvePoint);

			// Note that we're not getting the TNB frame here, but we're getting a frame that we can use.
			HappyMath::Vector3 xAxis, yAxis, zAxis;
			EvaluateCubicBezierCurveDerivative(node->tangentPoint, childNodeA->tangentPoint, curveParameter, zAxis);
			zAxis.Normalize();
			zAxis *= -1.0;
			xAxis.SetAsOrthogonalTo(zAxis);
			xAxis.Normalize();
			yAxis = zAxis.Cross(xAxis);

			for (int k = 0; k < samplesPerLocation; k++)
			{
				double angle = (double(k) / double(samplesPerLocation)) * 2.0 * M_PI;

				SurfacePoint surfacePoint;
				surfacePoint.location = curvePoint + wormholeRadius * (xAxis * ::cos(angle) + yAxis * ::sin(angle));
				surfacePoint.normal = (surfacePoint.location - curvePoint).Normalized();

				bool cullPoint = false;

				for (int l = 0; l < (int)node->childNodeArray.size(); l++)
				{
					if (i == l)
						continue;

					const Node* childNodeB = node->childNodeArray[l].get();

					HappyMath::Vector3 closestPoint;
					FindClosestPointOnCubicBezierCurve(node->tangentPoint, childNodeB->tangentPoint, surfacePoint.location, closestPoint);

					double squareDistance = (closestPoint - surfacePoint.location).SquareLength();
					if (squareDistance < wormholeRadius * wormholeRadius)
					{
						cullPoint = true;
						break;
					}
				}

				if (!cullPoint)
				{
					pointFunc(surfacePoint);
				}
			}
		}
	}
}

//--------------------------------- WormholeTree::GeneratorConfig ---------------------------------

WormholeTree::GeneratorConfig::GeneratorConfig()
{
	this->random = nullptr;
	this->maxDepth = 32;
	this->maxAngleDeviation = M_PI / 12.0;
	this->initialTangentPoint.location.SetComponents(0.0, 0.0, 0.0);
	this->initialTangentPoint.unitDirection.SetComponents(0.0, 0.0, -1.0);
	this->branchProbability = 0.1;
	this->maxBranchFactor = 2;
	this->minDistBetweenNodes = 10.0;
	this->maxDistBetweenNodes = 15.0;
}

//--------------------------------- WormholeTree::Node ---------------------------------

WormholeTree::Node::Node()
{
}

/*virtual*/ WormholeTree::Node::~Node()
{
}