#include <Engine/Engine.hpp>
#include <Engine/World/World.hpp>
#include <Engine/Streamer/Streamer.hpp>

#include <Utility/Log.hpp>
#include <Utility/String.hpp>

#pragma warning(push)
#pragma warning(disable:4068)
#include <GZIP/decompress.hpp>
#include <GZIP/config.hpp>
#include <GZIP/utils.hpp>
#include <GZIP/version.hpp>
#pragma warning(pop)

void World::OnEngineInitialize()
{
	LOG_DBG("Initializing the world...");
	if (!m_font.loadFromFile("../../Resources/Roboto-Light.ttf"))
	{
		LOG_ERR("Failed to load the font");
	}

	m_spaceReference.x = Engine::GetInstance()->GetDefaultSpaceReferenceX();
	m_spaceReference.y = Engine::GetInstance()->GetDefaultSpaceReferenceY();

	LOG_DBG("World initialized.");
}

void World::OnEngineStart()
{
	if (!m_textureAtlas.loadFromMemory(
		Streamer::GetInstance()->GetTextureAtlas().data(),
		Streamer::GetInstance()->GetTextureAtlas().size()))
	{
		LOG_ERR("Failed to read the texture atlas from memory");
	}

	if (!m_rpgTextureAtlas.loadFromMemory(
		Streamer::GetInstance()->GetRpgTextureAtlas().data(),
		Streamer::GetInstance()->GetRpgTextureAtlas().size()))
	{
		LOG_ERR("Failed to read the rpg texture atlas from memory");
	}
}

void World::OnEngineUpdate()
{
	// None
}

void World::OnEngineStop()
{
	// None
}

void World::OnEngineRender(sf::RenderWindow& window, const float interpolation)
{
	OnWorldRender	   (window, interpolation);
	OnWorldDisplayDebug(window, interpolation);
}

void World::OnWorldRender(sf::RenderWindow& window, const float interpolation)
{
	// Drawing all entities
	// Note : Design issue with the mutex. It was to avoid concurrency issues with m_worldCells.
	//		  Not really important for a sandbox project.
	m_mutex.lock();
	uint64_t GRID_WIDTH  = 256;
	uint64_t GRID_HEIGHT = 256;
	uint64_t CELL_SIZE   = 64;

	long long cellX = static_cast<long long>((m_spaceReference.x + (GRID_WIDTH  / 2) * CELL_SIZE) / CELL_SIZE);
	long long cellY = static_cast<long long>((m_spaceReference.y + (GRID_HEIGHT / 2) * CELL_SIZE) / CELL_SIZE);
	uint64_t currentCellID = GRID_HEIGHT * cellY + cellX;

	for (const auto& cell : m_worldCells)
	{
		long long worldCellX = cell->CellID % GRID_WIDTH;
		long long worldCellY = cell->CellID / GRID_HEIGHT;

		// Manhattan distance LOD
		uint64_t distance = abs(cellX - worldCellX) + abs(cellY - worldCellY);
		uint64_t LOD = (distance < 3) ? 2 :
					   (distance < 4) ? 1 : 0;	


		// Drawing terrain tiles
		sf::RenderStates terrainRenderState {};
		terrainRenderState.texture = &m_textureAtlas;
		window.draw(
			cell->terrainTiles.data(),
			cell->terrainTiles.size(), sf::Quads, terrainRenderState);

		// Drawning env tiles
		if (distance < 6)
		{
			uint64_t envCount = (distance > 3) ? std::max<uint64_t>(1, distance) : 1;
			sf::RenderStates envRenderState{};
			envRenderState.texture = &m_rpgTextureAtlas;
			window.draw(
				cell->envTiles.data(),
				cell->envTiles.size() / envCount, sf::Quads, envRenderState);
		}
		// Drawing terrain
		window.draw(
			cell->terrains[LOD].data(), 
			cell->terrains[LOD].size(), sf::Lines);
	}
	m_mutex.unlock();

	// Drawing the space reference
	sf::CircleShape spaceReferenceShape;
	spaceReferenceShape.setRadius   (2.0f);
	spaceReferenceShape.setFillColor(sf::Color::Red);
	spaceReferenceShape.setPosition(
		static_cast<float>( (m_spaceReference.x - spaceReferenceShape.getRadius() / 2.f + interpolation * (m_spaceReference.vx * Engine::GetDeltaTime()))),
		static_cast<float>(-(m_spaceReference.y - spaceReferenceShape.getRadius() / 2.f + interpolation * (m_spaceReference.vy * Engine::GetDeltaTime()))));

	// Creating the world view
	sf::View worldView(
		sf::Vector2f(
			spaceReferenceShape.getPosition().x, 
			spaceReferenceShape.getPosition().y),
		sf::Vector2f(window.getSize()));

	worldView.zoom(Engine::GetInstance()->GetZoomFactor());

	window.setView(worldView);
	window.draw(spaceReferenceShape);
}

void World::OnWorldDisplayDebug(sf::RenderWindow& window, const float interpolation)
{
	// Reset the current view
	const sf::View current = window.getView();
	window.setView(window.getDefaultView());

	sf::Text spaceReferenceText;
	spaceReferenceText.setFont	        (m_font);
	spaceReferenceText.setCharacterSize (16);
	spaceReferenceText.setPosition		(12.f, 12.f);
	spaceReferenceText.setString(
		"Space Ref Pos X : "  + to_string_with_precision(m_spaceReference.x, 3)  + "\n" +
		"Space Ref Pos Y : "  + to_string_with_precision(m_spaceReference.y, 3)  + "\n" +
		"Space Ref Vel X  : " + to_string_with_precision(m_spaceReference.vx, 3) + "\n" +
		"Space Ref Vel Y  : " + to_string_with_precision(m_spaceReference.vy, 3));

	window.draw(spaceReferenceText);
	window.setView(current);
}

SpaceReferencePrecise& World::GetSpaceReference()
{
	return m_spaceReference;
}

void World::OnCellAddToWorld(const CellLoadingResult& cell)
{
	m_mutex.lock();

	// A cell is loaded. The following method will deserialize the cell
	// and add it to the world.
	WorldCellData* worldCell = m_worldCells.emplace_back(new WorldCellData());
	worldCell->CellID = cell.cellID;

	// Step 0 : cell buffer decompression
	// Uncompress buffer. Don't forget that cells are small compressed buffers to save disk space
	const std::string& decompressed_data = gzip::decompress(
		(char*)cell.allocInfo->bytes.data(), cell.allocInfo->bytes.size());

	// Step 1 : deserialization of the terrain and its LODs
	uint64_t offset = 0;
	for (int i = 0; i < 3; ++i)
	{
		std::vector<sf::Vertex>& terrain = worldCell->terrains.emplace_back();
		
		// Get the size of the first LOD
		uint64_t LODSize = 0;
		memcpy(&LODSize, decompressed_data.data() + offset, sizeof(uint64_t));  offset += sizeof(uint64_t);

		// How many vertices
		uint64_t verticesCount = LODSize / sizeof(sf::Vertex);
		terrain.resize(verticesCount);

		sf::Vertex* in  = (sf::Vertex*)(decompressed_data.data() + offset);
		sf::Vertex* out = (sf::Vertex*)terrain.data();

		memcpy(out, in, LODSize); offset += LODSize;
	}

	// Step 2 : deserialization of the tiles
	uint64_t TileSize = 0;
	memcpy(&TileSize, decompressed_data.data() + offset, sizeof(uint64_t));  offset += sizeof(uint64_t);

	// How many vertices ?
	uint64_t verticesCount = TileSize / sizeof(sf::Vertex);
	worldCell->terrainTiles.resize(verticesCount);

	sf::Vertex* in  = (sf::Vertex*)(decompressed_data.data() + offset);
	sf::Vertex* out = (sf::Vertex*)worldCell->terrainTiles.data();
	memcpy(out, in, TileSize); offset += TileSize;

	// Step 3 : deserialization of the environment
	uint64_t EnvTileSize = 0;
	memcpy(&EnvTileSize, decompressed_data.data() + offset, sizeof(uint64_t));  offset += sizeof(uint64_t);

	// How many vertices ?
	verticesCount = EnvTileSize / sizeof(sf::Vertex);
	worldCell->envTiles.resize(verticesCount);

	in  = (sf::Vertex*)(decompressed_data.data() + offset);
	out = (sf::Vertex*)worldCell->envTiles.data();
	memcpy(out, in, EnvTileSize); offset += EnvTileSize;

	m_mutex.unlock();
}

void World::OnCellRemoveFromWorld(const uint64_t cellID)
{
	m_mutex.lock();
	for (int i = 0; i < m_worldCells.size();)
	{
		if (m_worldCells[i]->CellID == cellID)
		{
			WorldCellData* data = m_worldCells[i];
			m_worldCells[i] = m_worldCells.back();
			m_worldCells.pop_back();

			delete data;
		}
		else
		{
			++i;
		}
	}
	m_mutex.unlock();
}