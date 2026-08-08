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

	this->GeneratePolygons(config, progressReporter);

	// STPTODO: Note that at this point we could still go cull triangles that
	//          we know should not be there in each tube at an intersection.
	//          This could help reduce Z-fighting in the renderer.  The renderer
	//          would then need only deal with triangles that cross the boundary
	//          between legal and illegal.

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

		std::shared_ptr<Branch> branch = std::make_shared<Branch>();

		branch->node = childNode;

		parentNode->branchArray.push_back(branch);

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

			for (int i = 0; i < (int)node->branchArray.size(); i++)
			{
				const TangentPoint& tangentPointB = node->branchArray[i]->node->tangentPoint;

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

		for (int i = 0; i < (int)node->branchArray.size(); i++)
			nodeArray.push_back(node->branchArray[i]->node);
	}
}

void WormholeTree::GeneratePolygons(const GeneratorConfig& config, ProgressReporterInterface* progressReporter)
{
	int numNodes = 0;

	if (progressReporter)
	{
		this->ForEachNode([&numNodes](Node* node) -> void
			{
				numNodes++;
			});

		progressReporter->BeginTask("Generating polygons...");
	}

	int i = 0;

	this->ForEachNode([this, config, progressReporter, &i, numNodes](Node* node) -> void
		{
			this->GeneratePolygonsForNode(node, config);

			if (progressReporter)
			{
				double progress = double(++i) / double(numNodes);
				progressReporter->TaskUpdate(progress);
			}
		});

	if (progressReporter)
		progressReporter->EndTask();
}

void WormholeTree::GeneratePolygonsForNode(Node* node, const GeneratorConfig& config)
{
	for (int i = 0; i < (int)node->branchArray.size(); i++)
	{
		auto branch = node->branchArray[i];

		const Node* childNode = branch->node.get();

		HappyMath::Vector3** matrix = new HappyMath::Vector3*[config.numSteps];

		Frame frame;

		for (int row = 0; row < config.numSteps; row++)
		{
			double curveParameter = double(row) / double(config.numSteps - 1);

			HappyMath::Vector3 curvePoint;
			EvaluateCubicBezierCurve(node->tangentPoint, childNode->tangentPoint, curveParameter, curvePoint);

			// STPTODO: Note that this still isn't perfect, because the frame at the end of one tube doesn't match that at the beginning of another.
			CalcFrame(node->tangentPoint, childNode->tangentPoint, curveParameter, frame, row > 0);

			matrix[row] = new HappyMath::Vector3[config.samplesPerLocation];

			for (int col = 0; col < config.samplesPerLocation; col++)
			{
				double angle = (double(col) / double(config.samplesPerLocation)) * 2.0 * M_PI;

				matrix[row][col] = curvePoint + config.wormholeRadius * (frame.xAxis * ::cos(angle) + frame.yAxis * ::sin(angle));
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
				branch->AddPolygon(polygon);

				polygon.Clear();
				polygon.vertexArray.push_back(matrix[row][col]);
				polygon.vertexArray.push_back(matrix[row + 1][(col + 1) % config.samplesPerLocation]);
				polygon.vertexArray.push_back(matrix[row + 1][col]);
				branch->AddPolygon(polygon);
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

//--------------------------------- WormholeTree::RenderVertex ---------------------------------

std::string WormholeTree::RenderVertex::MakeKey() const
{
	return std::format("{}_{}_{}/{}_{}_{}",
		this->location.x, this->location.y, this->location.z,
		this->normal.x, this->normal.y, this->normal.z);
}

//--------------------------------- WormholeTree::Branch ---------------------------------

void WormholeTree::Branch::AddPolygon(const HappyMath::Polygon& polygon)
{
	HappyMath::Plane plane = polygon.CalcPlane(true);

	for (int j = 0; j < (int)polygon.vertexArray.size(); j++)
	{
		RenderVertex vertex;
		vertex.location = polygon.vertexArray[j];
		vertex.normal = plane.unitNormal;

		std::string key = vertex.MakeKey();

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

//--------------------------------- WormholeTree::Node ---------------------------------

WormholeTree::Node::Node()
{
}

/*virtual*/ WormholeTree::Node::~Node()
{
}

bool WormholeTree::Node::SaveToStream(std::ostream& outputStream) const
{
	this->tangentPoint.location.Dump(outputStream);
	this->tangentPoint.unitDirection.Dump(outputStream);

	int numBranches = (int)branchArray.size();
	outputStream.write((char*)&numBranches, sizeof(numBranches));

	for (const auto branch : this->branchArray)
	{
		int numVertices = (int)branch->vertexBuffer.size();
		outputStream.write((char*)&numVertices, sizeof(numVertices));

		for (int i = 0; i < numVertices; i++)
		{
			const RenderVertex& vertex = branch->vertexBuffer[i];
			vertex.location.Dump(outputStream);
			vertex.normal.Dump(outputStream);
		}

		int numIndices = (int)branch->indexBuffer.size();
		outputStream.write((char*)&numIndices, sizeof(numIndices));
		
		for (int index : branch->indexBuffer)
			outputStream.write((char*)&index, sizeof(index));

		if (!branch->node.get())
			return false;

		if (!branch->node->SaveToStream(outputStream))
			return false;
	}
	
	return true;
}

bool WormholeTree::Node::LoadFromStream(std::istream& inputStream, std::function<std::shared_ptr<Node>()> nodeMakerFunc)
{
	this->tangentPoint.location.Restore(inputStream);
	this->tangentPoint.unitDirection.Restore(inputStream);

	int numChildren = 0;
	inputStream.read((char*)&numChildren, sizeof(numChildren));

	for (int i = 0; i < numChildren; i++)
	{
		auto branch = std::make_shared<Branch>();

		int numVertices = 0;
		inputStream.read((char*)&numVertices, sizeof(numVertices));
		branch->vertexBuffer.reserve(numVertices);

		for (int j = 0; j < numVertices; j++)
		{
			RenderVertex vertex;
			vertex.location.Restore(inputStream);
			vertex.normal.Restore(inputStream);
			branch->vertexBuffer.push_back(vertex);
		}

		int numIndices = 0;
		inputStream.read((char*)&numIndices, sizeof(numIndices));
		branch->indexBuffer.reserve(numIndices);

		for (int j = 0; j < numIndices; j++)
		{
			int index = 0;
			inputStream.read((char*)&index, sizeof(index));
			branch->indexBuffer.push_back(index);
		}

		branch->node = nodeMakerFunc();
		if (!branch->node->LoadFromStream(inputStream, nodeMakerFunc))
			return false;

		this->branchArray.push_back(branch);
	}

	return true;
}