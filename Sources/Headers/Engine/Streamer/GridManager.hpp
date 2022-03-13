#pragma once

#include <mutex>
#include <thread>

#include <SFML/Graphics.hpp>

#include <Engine/Streamer/StreamerData.hpp>

class Streamer;

// The grid manager is what handle the streaming. It decides which cell should be fetched from the disk in the game data file,
// and which cell should be unloaded. The grid representation is simple :
//
// Schematic representation of the world divided into cells in 5x5 grid of 8x8 meters cells :
//
//	    Cell size = 8 meters
//             <--->
// +---+---+---+---+---+
// |   |   |   |   |   |
// +---+---+---+---+---+
// |   | / | / | / |   | If the main grid is 3x3, all cells 
// +---+---+---+---+---+ with a '/' will be loaded around the player '*'
// |   | / | * | / |   |
// +---+---+---+---+---+
// |   | / | / | / |   |
// +---+---+---+---+---+
// |   |   |   |   |   |
// +---+---+---+---+---+
// <------------------->
// Grid size = 5x5 cells
// 

class GridManager
{
public:
	// A CellID is a unique ID representing a specific cell in the grid. Nothing much than a simple unsigned.
	// A CellID can be computed when the following expression : CellID = s * y + x
	// where s is the size of a square grid (256 for 256x256 grid for example)
	// where y is the index of the cell on the y axis
	// where x is the index of the cell on the x axis
	using CellID = uint64_t;

	// The following two constants define the maximum grid size and the maximum cell size in meters.
	// 256x256 is purely arbitrary. It could have been much more, the only limit is the size on the disk of the map.
	// 1024 is also arbitrary but this metric is more important. If the number is to low, it will imply too much pressure
	// on the streaming system, but if it's too high, the engine will mostly fail to render and simulate the world.
	// 1024 is arbitrary here, but in an actual engine, it should be set up carrefuly.
	static constexpr uint64_t MAX_GRID_SIZE { 256 * 256 };
	static constexpr uint64_t MAX_CELL_SIZE { 1024 };

	// The main grid radius is what should be loaded around the player.
	// All of this following value are default values and they may be overwritten later.
	uint64_t MAIN_GRID_RADIUS	{ 12 };
	uint64_t GRID_WIDTH			{ 256 };
	uint64_t GRID_HEIGHT		{ 256 };
	uint64_t GRID_SIZE			{ 256 * 256 };
	uint64_t CELL_SIZE			{ 64 };

public:
	void OnEngineInitialize	();
	void OnEngineStart		();
	void OnEngineUpdate		();
	void OnEngineStop		();
	void OnEngineRender		(sf::RenderWindow& window, const float interpolation);
	void OnCellRequestDone  (const CellLoadingResult& r);

public: 
	// The following 3 methods are thread safe
	void PushCellRequests	(const std::vector<CellInfoEx>& request);
	void PullCellRequests	(std::vector<CellInfoEx>& request);
	void UnloadLoadedCells	(const std::vector<CellInfoEx>& toKeep);

public:
	Streamer*			GetWorldStreamer();
	static GridManager* GetInstance();

private:
	void OnGridManagerRenderDebug(sf::RenderWindow& window, const float interpolation);

private:
	uint64_t m_cellX;
	uint64_t m_cellY;
	CellID	 m_currentCellID;
	CellID	 m_lastCellID;

	std::vector<CellInfoEx>	m_loadedCells;
	std::vector<CellInfoEx> m_pendingCells;
	std::vector<CellInfoEx>	m_cellsToLoad;

	sf::Font			m_font;
	Streamer*			m_streamer;
	static GridManager* s_manager;
	std::thread*		m_loadingThread;
	std::mutex			m_mutex;
};