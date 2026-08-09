#include "CustomAssetCache.h"

CustomAssetCache::CustomAssetCache()
{
}

/*virtual*/ CustomAssetCache::~CustomAssetCache()
{
}

/*virtual*/ Imzadi::Asset* CustomAssetCache::CreateBlankAssetForFileType(const std::string& assetFile)
{
	std::filesystem::path assetPath(assetFile);
	std::string ext = assetPath.extension().string();
	std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
	
	// STPTODO: Make a wormhole asset.  It will get used by a wormhole render object, which I also need to make.

	Imzadi::Asset* asset = AssetCache::CreateBlankAssetForFileType(assetFile);
	if (asset)
		return asset;

	return nullptr;
}

/*virtual*/ bool CustomAssetCache::ResolvePath(const std::filesystem::path& givenPath, std::filesystem::path& resolvedPath)
{
	// STPTODO: Fix this later.

	if (givenPath.is_relative())
		resolvedPath = std::filesystem::path("D:/git_repos/WormholeGenerator/Dependencies/Imzadi") / givenPath;
	else
		resolvedPath = givenPath;

	return std::filesystem::exists(resolvedPath);
}