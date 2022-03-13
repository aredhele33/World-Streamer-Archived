#pragma once

#include <SFML/Window.hpp>
#include <SFML/Graphics.hpp>

// Class used to mostly handle inputs since there's nothing to do.
// See Game.cpp for more details
class Game
{
public:
	Game();

	void OnEngineStart	();
	void OnEngineUpdate	();
	void OnEngineRender (sf::RenderWindow&, const double interpolation);
	void OnEngineStop	();
	void OnHandleInput	(const sf::Event& event);
};