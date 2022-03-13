#pragma once

#include <SFML/Graphics.hpp>

class Game;
class World;
class GridManager;

class Engine
{
public:
	Engine();

public:
	bool Run(int argc, char** argv);

public:
	static Engine* GetInstance  ();
	static double  GetDeltaTime	();
	static World*  GetWorld		();

private:
	bool Initialize		(int argc, char** argv);
	void OnEngineStart	();
	void OnEngineUpdate	();
	void OnEngineStop	();
	void OnRenderFrame	(const double);
	void OnRenderDebug  (const double);
	void OnHandleInput	();

public:
	float GetDefaultSpaceReferenceX() const;
	float GetDefaultSpaceReferenceY() const;
	const char* GetGameDataFileName() const;

	void  SetZoomFactor	(float zoomFactor);
	float GetZoomFactor	() const;

	void	 SetGridRadius(uint64_t radius);
	uint64_t GetGridRadius() const;

private:
	Game*				m_game;
	GridManager*		m_gridManager;
	sf::RenderWindow*	m_window;
	double				m_frametimeS;
	double				m_frameDeltaS;
	sf::Int64			m_frametimeUs;
	bool				m_shouldQuit;
	sf::Font			m_font;
	uint64_t			m_frameID;
	float				m_spaceReferenceX;
	float				m_spaceReferenceY;
	const char*			m_gameDataFile;
	float				m_zoomFactor;
	uint64_t		    m_gridRadius;

private:
	static Engine* s_engine;
	static double  s_deltaTimeS;
	static World*  s_world;
};