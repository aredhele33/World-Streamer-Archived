#include <algorithm>

#include <Engine/Streamer/GridManager.hpp>

#include <Engine/Engine.hpp>
#include <Engine/Streamer/Streamer.hpp>
#include <Engine/World/World.hpp>

#include <Utility/Log.hpp>

/*static*/ GridManager* GridManager::s_manager { nullptr };

void GridManager::OnEngineInitialize()
{
	s_manager = this;

	m_streamer = new Streamer();
	m_streamer->OnEngineInitialize();

	GRID_WIDTH = m_streamer->GetGridSizeX();
	GRID_HEIGHT = m_streamer->GetGridSizeX();
	GRID_SIZE = GRID_WIDTH * GRID_HEIGHT;
	CELL_SIZE = m_streamer->GetCellSize();

	LOG_DBG("Initializing the grid manager...");
	m_cellX = 0;
	m_cellY = 0;
	m_currentCellID = 0;
	m_lastCellID = 0;

	if (!m_font.loadFromFile("../../Resources/Roboto-Light.ttf"))
	{
		LOG_ERR("Failed to load the font");
	}

	LOG_DBG("Default space reference x : 0");
	LOG_DBG("Default space reference y : 0");
	LOG_DBG("Grid manager initialized.");

	// The streamer work on a different thread
	m_loadingThread = new std::thread(&Streamer::OnEngineUpdate, m_streamer);
}

#pragma warning(push)
#pragma warning(disable:4244)
#pragma warning(disable:4018)
void GridManager::OnEngineStart()
{
	if (World* world = Engine::GetWorld())
	{
		MAIN_GRID_RADIUS = Engine::GetInstance()->GetGridRadius();

		const SpaceReferencePrecise& spaceReference = world->GetSpaceReference();

		// Locating the spaceReference
		m_cellX = (spaceReference.x + (GRID_WIDTH / 2) * CELL_SIZE) / CELL_SIZE;
		m_cellY = (spaceReference.y + (GRID_HEIGHT / 2) * CELL_SIZE) / CELL_SIZE;
		m_currentCellID = GRID_HEIGHT * m_cellY + m_cellX;

		// Main grid to load, main grid is 4 cells radius
		std::vector<CellInfoEx> cellsToPush;
		for (int64_t i = m_cellY - MAIN_GRID_RADIUS; i < m_cellY + MAIN_GRID_RADIUS + 1; ++i)
		{
			for (int64_t j = m_cellX - MAIN_GRID_RADIUS; j < m_cellX + MAIN_GRID_RADIUS + 1; ++j)
			{
				CellInfoEx cellInfo;
				cellInfo.cellID = GRID_HEIGHT * i + j;
				cellInfo.cellX = j;
				cellInfo.cellY = i;

				cellsToPush.push_back(cellInfo);
			}
		}

		PushCellRequests(cellsToPush);
	}
}

void GridManager::OnEngineUpdate()
{
	MAIN_GRID_RADIUS = Engine::GetInstance()->GetGridRadius();

	const SpaceReferencePrecise& spaceReference = Engine::GetWorld()->GetSpaceReference();

	// Updating the spaceReference location on the grid
	m_cellX = (spaceReference.x + (GRID_WIDTH / 2) * CELL_SIZE) / CELL_SIZE;
	m_cellY = (spaceReference.y + (GRID_HEIGHT / 2) * CELL_SIZE) / CELL_SIZE;
	m_currentCellID = GRID_HEIGHT * m_cellY + m_cellX;

	if (m_lastCellID != m_currentCellID)
	{
		m_lastCellID = m_currentCellID;
		std::vector<CellInfoEx> cellsToKeep;
		for (int64_t i = m_cellY - MAIN_GRID_RADIUS; i < m_cellY + MAIN_GRID_RADIUS + 1; ++i)
		{
			for (int64_t j = m_cellX - MAIN_GRID_RADIUS; j < m_cellX + MAIN_GRID_RADIUS + 1; ++j)
			{
				CellInfoEx& cellInfo = cellsToKeep.emplace_back();
				cellInfo.cellID = GRID_HEIGHT * i + j;
				cellInfo.cellX = j;
				cellInfo.cellY = i;
			}
		}
		UnloadLoadedCells(cellsToKeep);
	}

	// Main grid to load, main grid is 4 cells radius
	std::vector<CellInfoEx> cellsToPush;
	for (int64_t i = m_cellY - MAIN_GRID_RADIUS; i < m_cellY + MAIN_GRID_RADIUS + 1; ++i)
	{
		for (int64_t j = m_cellX - MAIN_GRID_RADIUS; j < m_cellX + MAIN_GRID_RADIUS + 1; ++j)
		{
			CellInfoEx cellInfo;
			cellInfo.cellID = GRID_HEIGHT * i + j;
			cellInfo.cellX = j;
			cellInfo.cellY = i;
			cellsToPush.push_back(cellInfo);
		}
	}

	PushCellRequests(cellsToPush);
}

void GridManager::OnEngineStop()
{
	// Shutdown thread safely here
	// m_streamer->OnEngineStop();
	// delete m_streamer;
}

void GridManager::OnEngineRender(sf::RenderWindow& window, const float interpolation)
{
	OnGridManagerRenderDebug(window, interpolation);
}

void GridManager::OnGridManagerRenderDebug(sf::RenderWindow& window, const float interpolation)
{
	m_mutex.lock();

	// Draw the debug grid
	const int64_t GRID_LENGTH_X = GRID_WIDTH * CELL_SIZE;
	const int64_t GRID_LENGTH_Y = GRID_HEIGHT * CELL_SIZE;

	static std::vector<sf::Vertex> lines;

	lines.clear();
	for (int64_t i = 0; i < GRID_HEIGHT; ++i)
	{
		sf::Vertex vertex[2];
		vertex[0].position.x = -(GRID_LENGTH_X / 2);
		vertex[0].position.y = -(GRID_LENGTH_Y / 2) + (int64_t)CELL_SIZE * i;

		vertex[1].position.x = +(GRID_LENGTH_X / 2);
		vertex[1].position.y = -(GRID_LENGTH_Y / 2) + (int64_t)CELL_SIZE * i;

		vertex[0].color = sf::Color(125, 125, 125, 80);
		vertex[1].color = sf::Color(125, 125, 125, 80);

		lines.push_back(vertex[0]);
		lines.push_back(vertex[1]);
	}

	for (int64_t i = 0; i < GRID_WIDTH; ++i)
	{
		sf::Vertex vertex[2];
		vertex[0].position.x = -(GRID_LENGTH_X / 2) + (int64_t)CELL_SIZE * i;
		vertex[0].position.y = +(GRID_LENGTH_Y / 2);

		vertex[1].position.x = -(GRID_LENGTH_X / 2) + (int64_t)CELL_SIZE * i;
		vertex[1].position.y = -(GRID_LENGTH_Y / 2);

		vertex[0].color = sf::Color(125, 125, 125, 80);
		vertex[1].color = sf::Color(125, 125, 125, 80);

		lines.push_back(vertex[0]);
		lines.push_back(vertex[1]);
	}

	window.draw(lines.data(), lines.size(), sf::Lines);

	// Drawing loaded cells and pending cells
	static std::vector<sf::Vertex> cellsToLoad;
	cellsToLoad.clear();
	for (const auto& cell : m_cellsToLoad)
	{
		sf::Vertex vertices[8];
		// DOWN
		vertices[0].position.x = (int64_t)cell.cellX * (int64_t)CELL_SIZE - ((int64_t)GRID_LENGTH_X / 2);
		vertices[0].position.y = (int64_t)cell.cellY * (int64_t)CELL_SIZE - ((int64_t)GRID_LENGTH_Y / 2);
		vertices[0].position.y *= -1.0f;

		vertices[1].position.x = vertices[0].position.x + (int64_t)CELL_SIZE;
		vertices[1].position.y = vertices[0].position.y;

		// RIGHT
		vertices[2].position.x = vertices[1].position.x;
		vertices[2].position.y = vertices[1].position.y;
		vertices[3].position.x = vertices[1].position.x;
		vertices[3].position.y = vertices[1].position.y - (int64_t)CELL_SIZE;

		// UP
		vertices[4].position.x = vertices[3].position.x;
		vertices[4].position.y = vertices[3].position.y;
		vertices[5].position.x = vertices[3].position.x - (int64_t)CELL_SIZE;
		vertices[5].position.y = vertices[3].position.y;

		// DOWN
		vertices[6].position.x = vertices[5].position.x;
		vertices[6].position.y = vertices[5].position.y;
		vertices[7].position.x = vertices[5].position.x;
		vertices[7].position.y = vertices[5].position.y + (int64_t)CELL_SIZE;

		for (auto& vertex : vertices)
		{
			vertex.color = sf::Color::Red;
			cellsToLoad.push_back(vertex);
		}
	}

	static std::vector<sf::Vertex> cellPending;

	cellPending.clear();
	for (const auto& cell : m_pendingCells)
	{
		sf::Vertex vertices[8];
		vertices[0].position.x = (int64_t)cell.cellX * (int64_t)CELL_SIZE - ((int64_t)GRID_LENGTH_X / 2);
		vertices[0].position.y = (int64_t)cell.cellY * (int64_t)CELL_SIZE - ((int64_t)GRID_LENGTH_Y / 2);
		vertices[0].position.y *= -1.0f;
		vertices[1].position.x = vertices[0].position.x + (int64_t)CELL_SIZE;
		vertices[1].position.y = vertices[0].position.y;
		vertices[2].position.x = vertices[1].position.x;
		vertices[2].position.y = vertices[1].position.y;
		vertices[3].position.x = vertices[1].position.x;
		vertices[3].position.y = vertices[1].position.y - (int64_t)CELL_SIZE;
		vertices[4].position.x = vertices[3].position.x;
		vertices[4].position.y = vertices[3].position.y;
		vertices[5].position.x = vertices[3].position.x - (int64_t)CELL_SIZE;
		vertices[5].position.y = vertices[3].position.y;
		vertices[6].position.x = vertices[5].position.x;
		vertices[6].position.y = vertices[5].position.y;
		vertices[7].position.x = vertices[5].position.x;
		vertices[7].position.y = vertices[5].position.y + (int64_t)CELL_SIZE;

		for (auto& vertex : vertices)
		{
			vertex.color = sf::Color::Magenta;
			cellPending.push_back(vertex);
		}
	}

	static std::vector<sf::Vertex> cellLoaded;
	cellLoaded.clear();
	for (const auto& cell : m_loadedCells)
	{
		// Tri
		sf::Vertex vertices[8];
		vertices[0].position.x = (int64_t)cell.cellX * (int64_t)CELL_SIZE - ((int64_t)GRID_LENGTH_X / 2);
		vertices[0].position.y = (int64_t)cell.cellY * (int64_t)CELL_SIZE - ((int64_t)GRID_LENGTH_Y / 2);
		vertices[0].position.y *= -1.0f;
		vertices[1].position.x = vertices[0].position.x + (int64_t)CELL_SIZE;
		vertices[1].position.y = vertices[0].position.y;
		vertices[2].position.x = vertices[1].position.x;
		vertices[2].position.y = vertices[1].position.y;
		vertices[3].position.x = vertices[1].position.x;
		vertices[3].position.y = vertices[1].position.y - (int64_t)CELL_SIZE;
		vertices[4].position.x = vertices[3].position.x;
		vertices[4].position.y = vertices[3].position.y;
		vertices[5].position.x = vertices[3].position.x - (int64_t)CELL_SIZE;
		vertices[5].position.y = vertices[3].position.y;
		vertices[6].position.x = vertices[5].position.x;
		vertices[6].position.y = vertices[5].position.y;
		vertices[7].position.x = vertices[5].position.x;
		vertices[7].position.y = vertices[5].position.y + (int64_t)CELL_SIZE;

		for (auto& vertex : vertices)
		{
			vertex.color = sf::Color::Green;
			cellLoaded.push_back(vertex);
		}
	}

	window.draw(cellsToLoad.data(), cellsToLoad.size(), sf::Lines);
	window.draw(cellPending.data(), cellPending.size(), sf::Lines);
	window.draw(cellLoaded.data(), cellLoaded.size(), sf::Lines);


	const sf::View current = window.getView();
	window.setView(window.getDefaultView());

	sf::Text renderText;
	renderText.setFont(m_font);
	renderText.setCharacterSize(16);
	renderText.setPosition(window.getView().getSize().x - 180.f, 12.0f);
	renderText.setString(
		"Current Cell ID : " + std::to_string(m_currentCellID) + "\n"
		"Current Cell X  : " + std::to_string(m_cellX) + "\n" +
		"Current Cell Y  : " + std::to_string(m_cellY));

	window.draw(renderText);
	window.setView(current);

	m_mutex.unlock();
}
#pragma warning(pop)

Streamer* GridManager::GetWorldStreamer()
{
	return m_streamer; // Dangerous
}

void GridManager::OnCellRequestDone(const CellLoadingResult& r)
{
	m_mutex.lock();
	for (int i = 0; i < m_pendingCells.size();)
	{
		if (m_pendingCells[i].cellID == r.cellID)
		{
			// Back swaping
			m_loadedCells.push_back(m_pendingCells[i]);
			m_pendingCells[i] = m_pendingCells.back();
			m_pendingCells.pop_back();

			Engine::GetWorld()->OnCellAddToWorld(r);
		}
		else
		{
			++i;
		}
	}
	m_mutex.unlock();
}

GridManager* GridManager::GetInstance()
{
	return s_manager;
}

void GridManager::PushCellRequests(const std::vector<CellInfoEx>& requests)
{
	m_mutex.lock();
	for (const auto& req : requests)
	{
		if (std::find_if(m_cellsToLoad.begin(), m_cellsToLoad.end(),
			[&req](const CellInfoEx& other) { return other.cellID == req.cellID; }) != m_cellsToLoad.end())
		{
			continue;
		}

		if (std::find_if(m_pendingCells.begin(), m_pendingCells.end(),
			[&req](const CellInfoEx& other) { return other.cellID == req.cellID; }) != m_pendingCells.end())
		{
			continue;
		}

		if (std::find_if(m_loadedCells.begin(), m_loadedCells.end(),
			[&req](const CellInfoEx& other) { return other.cellID == req.cellID; }) != m_loadedCells.end())
		{
			continue;
		}

		m_cellsToLoad.push_back(req);
	}
	m_mutex.unlock();
}

void GridManager::PullCellRequests(std::vector<CellInfoEx>& request)
{
	m_mutex.lock();
	for (const auto& req : m_cellsToLoad)
	{
		request.push_back(req);
		m_pendingCells.push_back(req);
	}
	m_cellsToLoad.clear();
	m_mutex.unlock();
}

void GridManager::UnloadLoadedCells(const std::vector<CellInfoEx>& toKeep)
{
	m_mutex.lock();
	for (int i = 0; i < m_loadedCells.size();)
	{
		if (std::find_if(toKeep.begin(), toKeep.end(),
			[this, i](const CellInfoEx& other) { return other.cellID == m_loadedCells[i].cellID; }) != toKeep.end())
		{
			++i;
			continue;
		}

		// Back swaping
		Engine::GetWorld()->OnCellRemoveFromWorld(m_loadedCells[i].cellID);
		m_loadedCells[i] = m_loadedCells.back();
		m_loadedCells.pop_back();
	}
	m_mutex.unlock();
}