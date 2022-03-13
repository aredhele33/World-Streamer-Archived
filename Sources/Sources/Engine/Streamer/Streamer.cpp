#include <cstdlib>

#include <Engine/Streamer/Streamer.hpp>

#include <Engine/Engine.hpp>
#include <Engine/Streamer/GridManager.hpp>
#include <Engine/World/World.hpp>

#include <Utility/Log.hpp>

#pragma warning(disable:4996)

/* static */ Streamer* Streamer::s_instance { nullptr };

BufferAllocator bufferAlloc;

VOID CALLBACK FileIOCompletionRoutine(
	_In_     DWORD dwErrorCode,
	_In_     DWORD dwNumberOfBytesTransfered,
	_Inout_  LPOVERLAPPED lpOverlapped
)
{
	if (dwErrorCode != 0)
	{
		LOG_ERR("Async IO returns with a failure (", dwErrorCode);
	}
	else
	{
		Streamer::GetInstance()->NotifyCellRequestCompleted(*lpOverlapped, dwNumberOfBytesTransfered);
	}
}

void Streamer::OnEngineInitialize()
{
	s_instance = this;

	m_gridSizeX = 0;
	m_gridSizeY = 0;
	m_cellSize  = 0;
	m_gameDataFile = INVALID_HANDLE_VALUE;

	LOG_DBG("Initializing the streamer...");

	SetLastError(0);
	m_gameDataFile = CreateFile(
		Engine::GetInstance()->GetGameDataFileName(),
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_RANDOM_ACCESS | FILE_ATTRIBUTE_READONLY, /* FILE_FLAG_OVERLAPPED, */
		NULL);

	DWORD err = GetLastError();

	if (m_gameDataFile == INVALID_HANDLE_VALUE)
	{
		LOG_ERR("Unable to open the game data file !");
	}
	else
	{
		LOG_DBG("Deserializing file header...");
		DWORD read = 0; 
		if (!ReadFile(m_gameDataFile, &m_gridSizeX, sizeof(m_gridSizeX), &read, NULL)) {
			LOG_ERR("Unable read the file !");
		} else {
			if (read != sizeof(uint64_t))
			{
				LOG_ERR("Unable to deserialize the file header !");
			}

			read = 0;
		}

		if (!ReadFile(m_gameDataFile, &m_gridSizeY, sizeof(m_gridSizeY), &read, NULL)) {
			LOG_ERR("Unable read the file !");
		}
		else {
			if (read != sizeof(uint64_t))
			{
				LOG_ERR("Unable to deserialize the file header !");
			}

			read = 0;
		}

		if (!ReadFile(m_gameDataFile, &m_cellSize, sizeof(m_cellSize), &read, NULL)) {
			LOG_ERR("Unable read the file !");
		}
		else {
			if (read != sizeof(uint64_t))
			{
				LOG_ERR("Unable to deserialize the file header !");
			}

			read = 0;
		}

		LOG_DBG("Game data header deserialized !");
		LOG_DBG("World Grid Size X : ", m_gridSizeX);
		LOG_DBG("World Grid Size Y : ", m_gridSizeY);
		LOG_DBG("World Cell X      : ", m_cellSize);

		// Reading the table
		m_cellTable.resize(m_gridSizeX * m_gridSizeY);
		for (int i = 0; i < m_cellTable.size(); ++i)
		{
			uint64_t cellInfo[3];
			if (!ReadFile(m_gameDataFile, &cellInfo, sizeof(cellInfo), &read, NULL))
			{
				LOG_ERR("Unable read the cell table!");
			}
			else
			{
				m_cellTable[cellInfo[0]].cellID = cellInfo[0];
				m_cellTable[cellInfo[0]].offset = cellInfo[1];
				m_cellTable[cellInfo[0]].size   = cellInfo[2];
			}
		}

		LOG_DBG("Cell table deserialized ! (", m_cellTable.size(), " cells)");
		LOG_DBG("Loading texture atlas...");

		uint64_t atlasSize = 0;
		if (!ReadFile(m_gameDataFile, &atlasSize, sizeof(atlasSize), &read, NULL)) {
			LOG_ERR("Unable read the file !");
		}
		else {
			if (read != sizeof(uint64_t))
			{
				LOG_ERR("Unable to read the texture atlas size !");
			}
			read = 0;
		}

		m_textureAtlas.resize(atlasSize);
		if (!ReadFile(m_gameDataFile, m_textureAtlas.data(), static_cast<DWORD>(atlasSize), &read, NULL)) {
			LOG_ERR("Unable read the file !");
		}
		else {
			if (read != atlasSize)
			{
				LOG_ERR("Unable to read the texture atlas !");
			}
			read = 0;
		}

		LOG_DBG("Texture atlas deserialized!");
		LOG_DBG("Loading rpg texture atlas...");

		uint64_t rpgAtlasSize = 0;
		if (!ReadFile(m_gameDataFile, &rpgAtlasSize, sizeof(rpgAtlasSize), &read, NULL)) {
			LOG_ERR("Unable read the file !");
		}
		else {
			if (read != sizeof(uint64_t))
			{
				LOG_ERR("Unable to read the rpg texture atlas size !");
			}
			read = 0;
		}

		m_rpgTextureAtlas.resize(rpgAtlasSize);
		if (!ReadFile(m_gameDataFile, m_rpgTextureAtlas.data(), static_cast<DWORD>(rpgAtlasSize), &read, NULL)) {
			LOG_ERR("Unable read the file !");
		}
		else {
			if (read != rpgAtlasSize)
			{
				LOG_ERR("Unable to read the rpg texture atlas !");
			}
			read = 0;
		}

		LOG_DBG("RPG texture atlas deserialized!");
	}

	// Closing handle
	CloseHandle(m_gameDataFile);

	// Reopening for async I/O
	m_gameDataFile = CreateFile(
		Engine::GetInstance()->GetGameDataFileName(),
		GENERIC_READ,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_RANDOM_ACCESS | FILE_ATTRIBUTE_READONLY | FILE_FLAG_OVERLAPPED,
		NULL);

	if (m_gameDataFile == INVALID_HANDLE_VALUE)
	{
		LOG_ERR("Unable to open the game data file !");
	}

	bufferAlloc.Init();

	LOG_DBG("Streamer initialized!");
}

void Streamer::OnEngineStart()
{
	// None
}

void Streamer::OnEngineUpdate()
{
	while (true)
	{
		// Voluntary slow down the thread to visualize the loading in the window
		Sleep(rand() % 20);

		std::vector<CellInfoEx> newRequests;
		GridManager::GetInstance()->PullCellRequests(newRequests);

		for (int i = 0; i < newRequests.size(); ++i)
		{
			bool exist = false;
			for (const auto& inProgress : m_requestInProgress)
			{
				if (inProgress->cellID == newRequests[i].cellID)
				{
					exist = true;
					break;
				}
			}
		
			if (!exist)
			{
				m_requestToProcess.push_back(newRequests[i]);
			}
		}

		uint64_t counter = 0;
		for (int i = 0; i < m_requestToProcess.size() && counter < 2; )
		{
			CellLoadingResult* result = m_requestInProgress.emplace_back(new CellLoadingResult());
			memset(&(result->asynchIORequest), 0, sizeof(result->asynchIORequest));

			LARGE_INTEGER offsetLarge{};
			offsetLarge.QuadPart = m_cellTable[m_requestToProcess[i].cellID].offset;
			result->asynchIORequest.Offset     = offsetLarge.LowPart;
			result->asynchIORequest.OffsetHigh = offsetLarge.HighPart;
			result->asynchIORequest.hEvent     = CreateEvent(NULL, TRUE, FALSE, NULL);

			result->cellID    = m_requestToProcess[i].cellID;
			result->allocInfo = bufferAlloc.GetFreeBuffer();

			if (ReadFileEx(m_gameDataFile,
				result->allocInfo->bytes.data(), 
				static_cast<DWORD>(m_cellTable[m_requestToProcess[i].cellID].size), &result->asynchIORequest, FileIOCompletionRoutine) == 0)
			{
				LOG_ERR("Failed to start the async I/O !");
				DWORD err = GetLastError();
				std::cout << err << std::endl;
			}

			m_requestToProcess[i] = m_requestToProcess.back();
			m_requestToProcess.pop_back();
			counter++;
		}

		// Voluntary slow down the thread to visualize the loading in the window
		SleepEx(5, true);
	}
}

void Streamer::OnEngineStop()
{
	if (m_gameDataFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(m_gameDataFile);
	}
}

void Streamer::OnEngineRender(sf::RenderWindow& window, const float interpolation)
{
	// None
}

Streamer* Streamer::GetInstance()
{
	return s_instance;
}

void Streamer::PushCellRequest(std::vector<CellInfoEx> requests)
{
	// None
}

void Streamer::NotifyCellRequestCompleted(const OVERLAPPED& overlapped, const DWORD read)
{
	for (uint64_t i = 0; i < m_requestInProgress.size(); ++i)
	{
		if (m_requestInProgress[i]->asynchIORequest.Offset     == overlapped.Offset &&
			m_requestInProgress[i]->asynchIORequest.OffsetHigh == overlapped.OffsetHigh)
		{
			// Back swaping
			CellLoadingResult* r = m_requestInProgress[i];
			if(m_requestInProgress.size() != 1)
				m_requestInProgress[i] = m_requestInProgress.back();

			m_requestInProgress.pop_back();

			// Notify world and grid
			GridManager::GetInstance()->OnCellRequestDone(*r);
			bufferAlloc.FreeBuffer(r->allocInfo);
			delete r;
		}
	}
}

std::vector<unsigned char>& Streamer::GetTextureAtlas()
{
	return m_textureAtlas;
}

std::vector<unsigned char>& Streamer::GetRpgTextureAtlas()
{
	return m_rpgTextureAtlas;
}