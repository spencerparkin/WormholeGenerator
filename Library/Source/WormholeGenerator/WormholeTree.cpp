#include "WormholeGenerator/WormholeTree.h"
#include "HappyMath/Ray.h"
#include <unordered_set>
#include <math.h>
#include <format>

using namespace WormholeGenerator;

// STPTODO: Implement the renderer.  DX11 is a good fit, since it's API is much sainer than OpenGL and
//          I need a programmable shader pipeline.  That is the key to rendering the wormhole properly.
//          The subtraction of one tube from another will be done in the shader.  The tricky part will
//          be determining the closest point on a bezier curve to a given point.  I think that Newton
//          iteration may be helpful here.  Let f(t) = (x - r(t))^2.  We want to minimize this function.
//          The derivative, I think, is something like f'(t) = 2*(x - r(t)).r'(t).  We want a zero of
//          this function, so we need f"(t) = 2*r'(t).r"(t), or something like that.  Our seed for the
//          Newton iteration could be based on a lerp between the two end-points.  That would get us
//          pretty close to begin with.  All along I had been trying to do math on meshes in space or
//          some such thing, and generate the polygons, etc., but I really think that a GPU-based approach
//          is the way to go.  Maybe it's a bit unsatisfying to not figure out the mesh math, but I think
//          the shader-way of doing this is actually pretty slick, if it can be done efficiently enough.
//
//          An alternative approach is to use off-screen rendering and compositing into the final frame
//          buffer where all we need to do is make clever use of the Z-buffer.

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

	this->GeneratePolygons(config);

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

/*static*/ void WormholeTree::CalcTNBFrame(const TangentPoint& tangentPointA, const TangentPoint& tangentPointB, double curveParameter, HappyMath::Vector3& xAxis, HappyMath::Vector3& yAxis, HappyMath::Vector3& zAxis)
{
	EvaluateCubicBezierCurveDerivative(tangentPointA, tangentPointB, curveParameter, zAxis);
	zAxis.Normalize();

	// Approximate the derivative of the tangent function using central differencing.
	double dt = 0.05;
	HappyMath::Vector3 vectorA, vectorB;
	EvaluateCubicBezierCurveDerivative(tangentPointA, tangentPointB, curveParameter + dt, vectorA);
	EvaluateCubicBezierCurveDerivative(tangentPointA, tangentPointB, curveParameter - dt, vectorB);
	vectorA.Normalize();
	vectorB.Normalize();
	xAxis = (vectorA - vectorB) / (2.0 * dt);

	// This is to account for round-off error.  Force a result that is orthogonal and unit-length.
	xAxis = xAxis.RejectedFrom(zAxis).Normalized();

	// Complete the frame to make a right-handed system.
	yAxis = zAxis.Cross(xAxis);
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

void WormholeTree::GeneratePolygons(const GeneratorConfig& config) const
{
	this->ForEachNode([this, config](const Node* node) -> void
		{
			this->GeneratePolygonsForNode(const_cast<Node*>(node), config);
		});
}

void WormholeTree::GeneratePolygonsForNode(Node* node, const GeneratorConfig& config) const
{
	for (int i = 0; i < (int)node->childNodeArray.size(); i++)
	{
		const Node* childNode = node->childNodeArray[i].get();

		HappyMath::Vector3** matrix = new HappyMath::Vector3*[config.numSteps];

		for (int row = 0; row < config.numSteps; row++)
		{
			double curveParameter = double(row) / double(config.numSteps);

			HappyMath::Vector3 curvePoint;
			EvaluateCubicBezierCurve(node->tangentPoint, childNode->tangentPoint, curveParameter, curvePoint);

			HappyMath::Vector3 xAxis, yAxis, zAxis;
			CalcTNBFrame(node->tangentPoint, childNode->tangentPoint, curveParameter, xAxis, yAxis, zAxis);

			matrix[row] = new HappyMath::Vector3[config.samplesPerLocation];

			for (int col = 0; col < config.samplesPerLocation; col++)
			{
				double angle = (double(col) / double(config.samplesPerLocation)) * 2.0 * M_PI;

				matrix[row][col] = curvePoint + config.wormholeRadius * (xAxis * ::cos(angle) + yAxis * ::sin(angle));
			}
		}

		for (int row = 0; row < config.numSteps - 1; row++)
		{
			for (int col = 0; col < config.samplesPerLocation; col++)
			{
				HappyMath::Polygon polygon;
				polygon.vertexArray.push_back(matrix[row][col]);
				polygon.vertexArray.push_back(matrix[row][(col + 1) % config.samplesPerLocation]);
				polygon.vertexArray.push_back(matrix[row + 1][(col + 1) % config.samplesPerLocation]);
				node->AddPolygon(polygon, i);

				polygon.Clear();
				polygon.vertexArray.push_back(matrix[row][col]);
				polygon.vertexArray.push_back(matrix[row + 1][(col + 1) % config.samplesPerLocation]);
				polygon.vertexArray.push_back(matrix[row + 1][col]);
				node->AddPolygon(polygon, i);
			}
		}

		for (int row = 0; row < config.numSteps; row++)
			delete[] matrix[row];

		delete[] matrix;
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
	this->samplesPerLocation = 32;
	this->numSteps = 32;
	this->wormholeRadius = 0.25;
}

//--------------------------------- WormholeTree::Node ---------------------------------

WormholeTree::Node::Node()
{
}

/*virtual*/ WormholeTree::Node::~Node()
{
}

void WormholeTree::Node::AddPolygon(const HappyMath::Polygon& polygon, uint32_t curve)
{
	HappyMath::Plane plane = polygon.CalcPlane(true);

	for (int j = 0; j < (int)polygon.vertexArray.size(); j++)
	{
		RenderVertex vertex;
		vertex.location = polygon.vertexArray[j];
		vertex.normal = plane.unitNormal;
		vertex.curve = curve;

		// This is actually really stupid, because I don't think we'll be re-using any vertex.
		// But I'm going to keep the index/vertex buffer combo, because maybe we can optimize this later.
		std::string key = std::format("{}_{}_{}/{}_{}_{}/{}",
			vertex.location.x, vertex.location.y, vertex.location.z,
			vertex.normal.x, vertex.normal.y, vertex.normal.z, curve);

		auto pair = indexBufferMap.find(key);
		if (pair != indexBufferMap.end())
		{
			int index = pair->second;
			indexBuffer.push_back(index);
		}
		else
		{
			int index = (int)vertexBuffer.size();
			indexBuffer.push_back(index);
			vertexBuffer.push_back(vertex);
			indexBufferMap.insert(std::pair(key, index));
		}
	}
}

bool WormholeTree::Node::SaveToStream(std::ostream& outputStream) const
{
	this->tangentPoint.location.Dump(outputStream);
	this->tangentPoint.unitDirection.Dump(outputStream);

	int size = (int)this->indexBuffer.size();
	outputStream.write((char*)&size, sizeof(size));
	
	for (int i = 0; i < size; i++)
		outputStream.write((char*)&this->indexBuffer[i], sizeof(uint32_t));

	size = (int)this->vertexBuffer.size();
	outputStream.write((char*)&size, sizeof(size));

	for (int i = 0; i < size; i++)
	{
		const RenderVertex& vertex = this->vertexBuffer[i];
		vertex.location.Dump(outputStream);
		vertex.normal.Dump(outputStream);
		outputStream.write((char*)&vertex.curve, sizeof(uint32_t));
	}

	size = (int)childNodeArray.size();
	outputStream.write((char*)&size, sizeof(size));

	for (const std::shared_ptr<Node> childNode : this->childNodeArray)
		if (!childNode->SaveToStream(outputStream))
			return false;
	
	return true;
}

bool WormholeTree::Node::LoadFromStream(std::istream& inputStream, std::function<std::shared_ptr<Node>()> nodeMakerFunc)
{
	this->tangentPoint.location.Restore(inputStream);
	this->tangentPoint.unitDirection.Restore(inputStream);

	int size = 0;
	inputStream.read((char*)&size, sizeof(size));

	for (int i = 0; i < size; i++)
	{
		uint32_t index = 0;
		inputStream.read((char*)&index, sizeof(int));
		this->indexBuffer.push_back(index);
	}

	inputStream.read((char*)&size, sizeof(size));

	for (int i = 0; i < size; i++)
	{
		RenderVertex renderVertex;
		renderVertex.location.Restore(inputStream);
		renderVertex.normal.Restore(inputStream);
		inputStream.read((char*)&renderVertex.curve, sizeof(uint32_t));
		this->vertexBuffer.push_back(renderVertex);
	}

	inputStream.read((char*)&size, sizeof(size));

	for (int i = 0; i < size; i++)
	{
		auto childNode = nodeMakerFunc();
		if (!childNode->LoadFromStream(inputStream, nodeMakerFunc))
			return false;

		this->childNodeArray.push_back(childNode);
	}

	return true;
}