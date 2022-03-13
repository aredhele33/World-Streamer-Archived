#define	 NOMINMAX

#include <Windows.h>

#include <Engine/Engine.hpp>
#include <Engine/World/World.hpp>
#include <Engine/Streamer/GridManager.hpp>

#include <Game/Game.hpp>
#include <Utility/Log.hpp>
#include <Utility/String.hpp>

/* static */ Engine* Engine::s_engine    { nullptr };
/* static */ double Engine::s_deltaTimeS { 0.0 };
/* static */ World* Engine::s_world		 { nullptr };

Engine::Engine()
: m_game			(nullptr)
, m_gridManager		(nullptr)
, m_window			(nullptr)
, m_frametimeS		(0.0)
, m_frameDeltaS		(0.0)
, m_frametimeUs		(0)
, m_shouldQuit		(false)
, m_frameID			(0)
, m_spaceReferenceX	(0.f)
, m_spaceReferenceY	(0.f)
, m_gameDataFile	(nullptr)
, m_zoomFactor		(1.f)
, m_gridRadius		(4)
{
	s_engine = this;
}

bool Engine::Run(int argc, char** argv)
{
	if (!Initialize(argc, argv))
	{
		return false;
	}

	sf::Clock gameClock;

	// Using Int64 for precise frame time
	sf::Int64 start   = gameClock.getElapsedTime().asMicroseconds();
	sf::Int64 elapsed = start;
	double lastDelta  = static_cast<double>(start);

	OnEngineStart();
	while (m_window->isOpen() && !m_shouldQuit)
	{
		sf::Int64 end   = gameClock.getElapsedTime().asMicroseconds();
		sf::Int64 delta = end - start;
		m_frameDeltaS   = static_cast<float>(delta) / 1000000.0;

		start    = end;
		elapsed += delta;

		// Simple fixed step game loop
		while (elapsed > m_frametimeUs)
		{
			const double currentClock = gameClock.getElapsedTime().asSeconds();
			s_deltaTimeS = currentClock - lastDelta;
			lastDelta    = currentClock;

			OnHandleInput ();
			OnEngineUpdate();
			elapsed -= m_frametimeUs;
		}

		// Frame interpolation for smooth movements
		const double elapsedSecond = static_cast<double>(elapsed) / 1000000.0;
		OnRenderFrame(elapsedSecond / m_frametimeS);
		m_frameID++;
	}
	OnEngineStop();

	return true;
}

/* static */ Engine* Engine::GetInstance()
{
	return s_engine;
}

/* static */ double Engine::GetDeltaTime()
{
	return s_deltaTimeS;
}

/* static */ World* Engine::GetWorld()
{
	return s_world;
}

bool Engine::Initialize(int argc, char** argv)
{
	LOG_DBG("Initializing the engine...");

	if (argc != 4)
	{
		LOG_ERR("Missing command line arguments.");
		LOG_DBG("Usage   : MMFStreamer.exe X Y filename");
		LOG_DBG("Example : MMFStreamer.exe 0 0 game_data.bin");
		return false;
	}

	m_spaceReferenceX = static_cast<float>(atof(argv[1]));
	m_spaceReferenceY = static_cast<float>(atof(argv[2]));
	m_gameDataFile    = argv[3];

	// Main thread pinning on the first CPU Core
	// to make things easier while profiling
	SetThreadAffinityMask(GetCurrentThread(), 1);
	LOG_DBG("Main thread pinned on CPU Core 0");

	constexpr sf::Int64 framerate = 144;
	m_frametimeS  = 1		/ (double)framerate;
	m_frametimeUs = 1000000 / framerate;

	sf::ContextSettings settings {};
	settings.antialiasingLevel = 8;

	m_window = new sf::RenderWindow(sf::VideoMode(1280, 720), "Open World Streamer", sf::Style::Default, settings);

	s_world = new World();
	s_world->OnEngineInitialize();

	m_gridManager = new GridManager();
	m_gridManager->OnEngineInitialize();

	m_game = new Game();

	if (!m_font.loadFromFile("../../Resources/Roboto-Light.ttf"))
	{
		LOG_ERR("Failed to load the font");
	}

	LOG_DBG("Engine initialized.");
	return true;
}

void Engine::OnEngineStart()
{
	LOG_DBG("Starting the engine...");
	m_gridManager->OnEngineStart();
	s_world->OnEngineStart();
	m_game->OnEngineStart();
	LOG_DBG("Engine started.");
}

void Engine::OnEngineUpdate()
{
	m_gridManager->OnEngineUpdate();
	s_world->OnEngineUpdate();
	m_game->OnEngineUpdate();
}

void Engine::OnEngineStop()
{
	LOG_DBG("Stopping the engine...");
	m_game->OnEngineStop();

	if (!m_window || !m_window->isOpen()) 
	{
		LOG_WRN("Windows closed.");
	}
	else
	{
		m_window->close();
		LOG_WRN("Engine exit requested.");
	}

	s_world->OnEngineStop();
	m_gridManager->OnEngineStop();

	delete m_game;
	delete s_world;
	delete m_gridManager;
	delete m_window;
	LOG_DBG("Engine stopped.");
}

void Engine::OnRenderFrame(const double interpolation)
{
	if (m_window)
	{
		m_window->clear();

		s_world->OnEngineRender(*m_window, static_cast<float>(interpolation));
		m_gridManager->OnEngineRender(*m_window, static_cast<float>(interpolation));
		m_game->OnEngineRender(*m_window, static_cast<float>(interpolation));

		OnRenderDebug(interpolation);
		m_window->display();
	}
}

void Engine::OnRenderDebug(const double interpolation)
{
	const sf::View current = m_window->getView();
	m_window->setView(m_window->getDefaultView());

	static double lastDeltaTime     = s_deltaTimeS;
	static double lastTrueDelta     = m_frameDeltaS;
	static double lastFrameTime     = m_frametimeS;
	static double lastInterpolation = interpolation;
	static uint64_t fps				= 0;
	static uint64_t lastFrameID     = m_frameID;

	static sf::Clock s_clock;
	if (s_clock.getElapsedTime().asSeconds() >= 1.0)
	{
		s_clock.restart();

		// Cheap way to get the FPS
		fps = m_frameID - lastFrameID;

		lastDeltaTime     = s_deltaTimeS;
		lastTrueDelta     = m_frameDeltaS;
		lastFrameTime     = m_frametimeS;
		lastInterpolation = interpolation;
		lastFrameID       = m_frameID;
	}

	sf::Text renderText;
	renderText.setFont(m_font);
	renderText.setCharacterSize(16);
	renderText.setPosition(12.f, 128.0f);
	renderText.setString(
		"FPS : " + std::to_string(fps) + "\n"
		"Frames : " + std::to_string(lastFrameID) + "\n" +
		"Delta time (ms) : " + to_string_with_precision(lastDeltaTime * 1000.0, 3) + "\n" +
		"True  frame duration (ms) : " + to_string_with_precision(lastTrueDelta * 1000.0, 4) + "\n" +
		"Fixed frame duration (ms) : " + to_string_with_precision(lastFrameTime * 1000.0, 4) + "\n" +
		"Frame interpolation  (ms) : " + to_string_with_precision(lastInterpolation, 4));

	m_window->draw(renderText);
	m_window->setView(current);
}

void Engine::OnHandleInput()
{
	sf::Event event;
	while (m_window->pollEvent(event))
	{
		if (event.type == sf::Event::Closed ||
			event.type == sf::Keyboard::Escape)
		{
			m_window->close();
		}

		m_game->OnHandleInput(event);
	}
}

float Engine::GetDefaultSpaceReferenceX() const
{
	return m_spaceReferenceX;
}

float Engine::GetDefaultSpaceReferenceY() const
{
	return m_spaceReferenceY;
}

const char* Engine::GetGameDataFileName() const
{
	return m_gameDataFile;
}

void Engine::SetZoomFactor(float zoomFactor)
{
	m_zoomFactor = zoomFactor;
}

float Engine::GetZoomFactor() const
{
	return m_zoomFactor;
}

void Engine::SetGridRadius(uint64_t radius)
{
	m_gridRadius = radius;
}

uint64_t Engine::GetGridRadius() const
{
	return m_gridRadius;
}