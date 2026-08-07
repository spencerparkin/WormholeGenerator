#include "WormholeGenerator/WormholeRenderer.h"
#include <unordered_map>
#include <format>

using namespace WormholeGenerator;

//------------------------------------- WormholeRenderer -------------------------------------

WormholeRenderer::WormholeRenderer()
{
	this->speed = 0.0;
}

/*virtual*/ WormholeRenderer::~WormholeRenderer()
{
}

void WormholeRenderer::Clear()
{
	this->rootNode.reset();
}

bool WormholeRenderer::LoadWormholeData(const std::string& filePath)
{
	auto nodeMakerFunc = []() -> std::shared_ptr<WormholeTree::Node>
		{
			return std::make_shared<RenderNode>();
		};

	WormholeTree wormholeTree;
	if (!wormholeTree.LoadFromDisk(filePath, nodeMakerFunc))
		return false;

	wormholeTree.ForEachNode([&wormholeTree](const WormholeTree::Node* node) -> void
		{
			RenderNode* renderNode = (RenderNode*)node;

			glGenBuffers(1, &renderNode->indexBufferObject);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, renderNode->indexBufferObject);
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, renderNode->indexBuffer.size() * sizeof(uint32_t), renderNode->indexBuffer.data(), GL_STATIC_DRAW);
			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

			glGenBuffers(1, &renderNode->vertexBufferObject);
			glBindBuffer(GL_ARRAY_BUFFER, renderNode->vertexBufferObject);
			glBufferData(GL_ARRAY_BUFFER, renderNode->vertexBuffer.size() * sizeof(WormholeGenerator::WormholeTree::RenderVertex), renderNode->vertexBuffer.data(), GL_STATIC_DRAW);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
		});

	this->rootNode = std::dynamic_pointer_cast<RenderNode>(wormholeTree.GetRootNode()->shared_from_this());

	return true;
}

void WormholeRenderer::Render()
{
}

void WormholeRenderer::Advance(double deltaTime)
{
}

void WormholeRenderer::SetSpeed(double speed)
{
	this->speed = speed;
}

double WormholeRenderer::GetSpeed() const
{
	return this->speed;
}

void WormholeRenderer::ApplyRoll(double rollAngleDelta)
{
}

//------------------------------------- WormholeRenderer::RenderNode -------------------------------------

WormholeRenderer::RenderNode::RenderNode()
{
	this->vertexBufferObject = 0;
	this->indexBufferObject = 0;
}

/*virtual*/ WormholeRenderer::RenderNode::~RenderNode()
{
	glDeleteBuffers(1, &this->indexBufferObject);
	glDeleteBuffers(1, &this->vertexBufferObject);
}