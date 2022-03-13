#pragma once

#define NOMINMAX
#include <Windows.h>

#include <SFML/Graphics.hpp>

#include <Engine/Streamer/StreamerData.hpp>

// The streamer is the class that will handle all aynchronous load requests.
// Loading cells from the game file is asynchronous but the moment where the loaded cells
// are added to the world is blocking. This streamer uses the Win32 asynchronous I/O.
class Streamer
{
public:
	void OnEngineInitialize	();
	void OnEngineStart		();
	void OnEngineUpdate		();
	void OnEngineStop		();
	void OnEngineRender		(sf::RenderWindow& window, const float interpolation);

public:
	static Streamer* GetInstance();

public:
	void PushCellRequest(std::vector<CellInfoEx> requests);
	void NotifyCellRequestCompleted(const OVERLAPPED& overlapped, const DWORD read);

public:
	uint64_t GetGridSizeX() const { return m_gridSizeX; }
	uint64_t GetGridSizeY() const { return m_gridSizeY; }
	uint64_t GetCellSize () const { return m_cellSize ; }
	std::vector<unsigned char>& GetTextureAtlas		();
	std::vector<unsigned char>& GetRpgTextureAtlas	();

private:
	std::vector<CellInfoEx>          m_requestToProcess;
	std::vector<CellLoadingRequest>  m_pendingRequest;
	std::vector<CellLoadingResult*>  m_requestInProgress;
	std::vector<CellInfo>			 m_cellTable;
	std::vector<unsigned char>		 m_textureAtlas;
	std::vector<unsigned char>		 m_rpgTextureAtlas;

private:
	uint64_t m_gridSizeX;
	uint64_t m_gridSizeY;
	uint64_t m_cellSize;
	HANDLE m_gameDataFile;

private:
	static Streamer* s_instance;
};