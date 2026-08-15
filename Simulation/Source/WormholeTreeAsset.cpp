#include "WormholeTreeAsset.h"

WormholeTreeAsset::WormholeTreeAsset()
{
}

/*virtual*/ WormholeTreeAsset::~WormholeTreeAsset()
{
}

/*virtual*/ bool WormholeTreeAsset::Load(const rapidjson::Document& jsonDoc, Imzadi::AssetCache* assetCache)
{
	return false;
}

/*virtual*/ bool WormholeTreeAsset::Unload()
{
	return false;
}