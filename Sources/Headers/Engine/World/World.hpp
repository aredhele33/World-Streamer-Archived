#pragma once

#include <mutex>
#include <vector>

#include <SFML/Graphics.hpp>

#include <Engine/Streamer/StreamerData.hpp>
#include <Engine/World/SpaceReferencePrecise.hpp>

// This a very basic representation of a 2D RPG world which can be streamed from the disk.
// The world is sliced in a grid that contains cells. Each cell has multiple tiles, a terrain and an environment.
struct WorldCellData
{
	// Unique ID of the cell, can be used to compute its position in the grid.
	uint64_t CellID;

	std::vector<std::vector<sf::Vertex>> terrains;
	std::vector<sf::Vertex>				 terrainTiles;
	std::vector<sf::Vertex>				 envTiles;
};

class World
{
public:
	void OnEngineInitialize	();
	void OnEngineStart		();
	void OnEngineUpdate		();
	void OnEngineStop		();
	void OnEngineRender		(sf::RenderWindow& window, const float interpolation);

public:
	SpaceReferencePrecise& GetSpaceReference();

	// OnCellAddToWorld is used to add a new cell in the world (in order to be displayed) when the loading is over.
	// OnCellRemoveFromWorld is used when a cell is no more needed and must be unloaded (the space reference is too far of it).
	void OnCellAddToWorld	  (const CellLoadingResult& cell);
	void OnCellRemoveFromWorld(const uint64_t cell);

private:
	void OnWorldRender		(sf::RenderWindow& window, const float interpolation);
	void OnWorldDisplayDebug(sf::RenderWindow& window, const float interpolation);

private:
	SpaceReferencePrecise m_spaceReference;

private:
	sf::Font					m_font;
	std::vector<WorldCellData*> m_worldCells;
	sf::Texture				    m_textureAtlas;
	sf::Texture				    m_rpgTextureAtlas;
	std::mutex				    m_mutex;
};