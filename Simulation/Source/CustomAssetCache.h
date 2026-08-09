#pragma once

#include "AssetCache.h"

class CustomAssetCache : public Imzadi::AssetCache
{
public:
	CustomAssetCache();
	virtual ~CustomAssetCache();

	virtual bool ResolvePath(const std::filesystem::path& givenPath, std::filesystem::path& resolvedPath) override;

protected:
	virtual Imzadi::Asset* CreateBlankAssetForFileType(const std::string& assetFile) override;
};