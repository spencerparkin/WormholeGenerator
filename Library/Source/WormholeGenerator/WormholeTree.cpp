#include "WormholeGenerator/WormholeTree.h"
#include "HappyMath/Ray.h"
#include <unordered_set>
#include <math.h>
#include <format>

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

bool WormholeTree::Generate(const GeneratorConfig& config, ProgressReporterInterface* progressReporter /*= nullptr*/)
{
	if (!config.random)
		return false;

	this->Clear();

	this->rootNode = std::make_shared<Node>();
	this->rootNode->tangentPoint = config.initialTangentPoint;

	this->GenerateRecursive(config, this->rootNode, 0);

#if 0	// STPTODO: Uncomment when ready.
	HappyMath::Graph graph;
	if (!graph.FromSurface(this, 5, 0.05, HappyMath::Vector3(0.0, 0.0, 0.0)))
		return false;

	HappyMath::PolygonMesh mesh;
	if (!graph.ToPolygonMesh(mesh))
		return false;
#endif

	// STPTODO: Distribute polygons from the mesh to the various nodes.
	//          At run-time, only those nodes near the viewer are rendered rather
	//          then rendering them all at all times.

	return true;
}

/*virtual*/ bool WormholeTree::FindNearestPoint(const HappyMath::Vector3& point, HappyMath::Vector3& surfacePoint, HappyMath::Vector3& surfaceNormal) const
{
	// STPTODO: Write this.

	return false;
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

		parentNode->nodeArray.push_back(childNode);

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

/*static*/ void WormholeTree::FindNearestPointOnBezierCurveToGivenPoint(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double& curveParameter, const HappyMath::Vector3& givenPoint)
{
	//...
}

/*static*/ void WormholeTree::CalcFrame(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, Frame& frame, bool advance)
{
	EvaluateCubicBezierCurveDerivative(tangentPointA, tangentPointB, curveParameter, frame.zAxis);
	frame.zAxis.Normalize();

	// Note that one of the weakness of the TNB frame is that it can flip 180 where curvature goes to zero.
	// So the advanced flag should be set most of the time.
	if (!advance)
	{
		// Approximate the derivative of the tangent function using central differencing.
		double dt = 0.05;
		HappyMath::Vector3 vectorA, vectorB;
		EvaluateCubicBezierCurveDerivative(tangentPointA, tangentPointB, curveParameter + dt, vectorA);
		EvaluateCubicBezierCurveDerivative(tangentPointA, tangentPointB, curveParameter - dt, vectorB);
		vectorA.Normalize();
		vectorB.Normalize();
		frame.xAxis = (vectorA - vectorB) / (2.0 * dt);
	}

	// This is to account for round-off error.  Force a result that is orthogonal and unit-length.
	frame.xAxis = frame.xAxis.RejectedFrom(frame.zAxis).Normalized();

	// Complete the frame to make a right-handed system.
	frame.yAxis = frame.zAxis.Cross(frame.xAxis);
}

void WormholeTree::ForEachRenderLine(int linesPerCurve, std::function<void(const HappyMath::LineSegment&)> renderFunc) const
{
	this->ForEachNode([linesPerCurve, renderFunc](const Node* node) -> void
		{
			const TangentPoint& tangentPointA = node->tangentPoint;

			for (int i = 0; i < (int)node->nodeArray.size(); i++)
			{
				const TangentPoint& tangentPointB = node->nodeArray[i]->tangentPoint;

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
	std::function<void(Node*)> nonConstNodeFunc = nodeFunc;
	const_cast<WormholeTree*>(this)->ForEachNode(nonConstNodeFunc);
}

void WormholeTree::ForEachNode(std::function<void(Node*)> nodeFunc)
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

		for (int i = 0; i < (int)node->nodeArray.size(); i++)
			nodeArray.push_back(node->nodeArray[i]);
	}
}

bool WormholeTree::SaveToDisk(const std::string& filePath) const
{
	if (!this->rootNode.get())
		return false;

	std::ofstream fileStream;
	fileStream.open(filePath, std::ios::out | std::ios::binary);
	if (!fileStream.is_open())
		return false;

	if (!this->rootNode->SaveToStream(fileStream))
		return false;

	fileStream.close();
	return true;
}

bool WormholeTree::LoadFromDisk(const std::string& filePath, std::function<std::shared_ptr<Node>()> nodeMakerFunc /*= []() { return std::make_shared<Node>(); }*/)
{
	this->Clear();

	std::ifstream fileStream;
	fileStream.open(filePath, std::ios::in | std::ios::binary);
	if (!fileStream.is_open())
		return false;

	this->rootNode = nodeMakerFunc();
	if (!this->rootNode->LoadFromStream(fileStream, nodeMakerFunc))
		return false;

	fileStream.close();
	return true;
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
	this->wormholeRadius = 0.25;
}

//--------------------------------- WormholeTree::Node ---------------------------------

WormholeTree::Node::Node()
{
}

/*virtual*/ WormholeTree::Node::~Node()
{
}

bool WormholeTree::Node::SaveToStream(std::ostream& outputStream) const
{
	return false;
}

bool WormholeTree::Node::LoadFromStream(std::istream& inputStream, std::function<std::shared_ptr<Node>()> nodeMakerFunc)
{
	return false;
}