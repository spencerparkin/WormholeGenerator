#pragma once

#include "AssetCache.h"

class WormholeTreeAsset : public Imzadi::Asset
{
public:
	WormholeTreeAsset();
	virtual ~WormholeTreeAsset();

	virtual bool Load(const rapidjson::Document& jsonDoc, Imzadi::AssetCache* assetCache) override;
	virtual bool Unload() override;
};