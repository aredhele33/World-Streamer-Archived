#include <Game/Game.hpp>

#include <Engine/Engine.hpp>
#include <Engine/World/World.hpp>

const float velocityX	   = 4.5f;
const float velocityY	   = 4.5f;
const float velocityBoost  = 60.f;

float zoomFactor		   = 1.0f;
const float zoomFactorUp   = 0.95f;
const float zoomFactorDown = 1.05f;

uint64_t gridRadius		   = 4;

bool isRunning     { false };
bool isMovingUp    { false };
bool isMovingDown  { false };
bool isMovingRight { false };
bool isMovingLeft  { false };

bool isZoomingUp   { false };
bool isZoomingDown { false };

Game::Game()
{
	// None
}

void Game::OnEngineStart()
{
	// None
}

void Game::OnEngineUpdate()
{
	// Move the space reference around depending the inptut
	const float currentBoost = (isRunning) ? velocityBoost : 1.f;
	const float currentVelX = velocityX * currentBoost * static_cast<float>(Engine::GetDeltaTime());
	const float currentVelY = velocityY * currentBoost * static_cast<float>(Engine::GetDeltaTime());
	
	Engine::GetWorld()->GetSpaceReference().vx = currentVelX;
	Engine::GetWorld()->GetSpaceReference().vy = currentVelY;

	float currentPosX = static_cast<float>(Engine::GetWorld()->GetSpaceReference().x);
	float currentPosY = static_cast<float>(Engine::GetWorld()->GetSpaceReference().y);

	if      (isMovingUp)    { currentPosY += currentVelY; }
	else if (isMovingDown)  { currentPosY -= currentVelY; }
	else if (isMovingRight) { currentPosX += currentVelX; }
	else if (isMovingLeft)  { currentPosX -= currentVelX; }

	Engine::GetWorld()->GetSpaceReference().x = currentPosX;
	Engine::GetWorld()->GetSpaceReference().y = currentPosY;

	Engine::GetInstance()->SetGridRadius(gridRadius);

	if      (isZoomingUp)   { zoomFactor *= zoomFactorUp;    Engine::GetInstance()->SetZoomFactor(zoomFactor); isZoomingUp   = false; }
	else if (isZoomingDown) { zoomFactor *= zoomFactorDown;  Engine::GetInstance()->SetZoomFactor(zoomFactor); isZoomingDown = false; }
}

void Game::OnEngineRender(sf::RenderWindow& window, const double interpolation)
{
	// None
}

void Game::OnEngineStop()
{
	// None
}

void Game::OnHandleInput(const sf::Event& event)
{
	// Input summary : 
	// Move			 = keyboard arrow keys
	// Speed boost	 = keyboard left shift key
	// Zoom in		 = keyboard numpad plus key  (+) or A (for keyboard without numpad +)
	// Zoom out		 = keyboard numpad minus key (-) or E (for keyboard without numpad -)
	// Increase grid = keyboard numpad minus key (/) or F (for keyboard without numpad /)
	// Decrease grid = keyboard numpad minus key (*) or G (for keyboard without numpad *)

	if (event.type == sf::Event::KeyPressed)
	{
		if (event.key.code == sf::Keyboard::Up)
		{
			isMovingUp    = true;
			isMovingDown  = false;
			isMovingRight = false;
			isMovingLeft  = false;
		}
		else if (event.key.code == sf::Keyboard::Right)
		{
			isMovingUp    = false;
			isMovingDown  = false;
			isMovingRight = true;
			isMovingLeft  = false;
		}
		else if (event.key.code == sf::Keyboard::Down)
		{
			isMovingUp    = false;
			isMovingDown  = true;
			isMovingRight = false;
			isMovingLeft  = false;
		}
		else if (event.key.code == sf::Keyboard::Left)
		{
			isMovingUp    = false;
			isMovingDown  = false;
			isMovingRight = false;
			isMovingLeft  = true;
		}
		else if (event.key.code == sf::Keyboard::LShift)
		{
			isRunning = true;
		}
		else if (event.key.code == sf::Keyboard::Add || 
				 event.key.code == sf::Keyboard::A)
		{
			isZoomingUp   = true;
		}
		else if (event.key.code == sf::Keyboard::Subtract ||
				 event.key.code == sf::Keyboard::E)
		{
			isZoomingDown = true;
		}
		else if (event.key.code == sf::Keyboard::Divide ||
			     event.key.code == sf::Keyboard::F)
		{
			gridRadius++;
		}
		else if (event.key.code == sf::Keyboard::Multiply ||
				 event.key.code == sf::Keyboard::G)
		{
			if (gridRadius > 2)
			{
				gridRadius--;
			}
		}
	}

	if (event.type == sf::Event::KeyReleased)
	{
		switch (event.key.code)
		{
			case sf::Keyboard::Up:       { isMovingUp    = false; break; }
			case sf::Keyboard::Right:    { isMovingRight = false; break; }
			case sf::Keyboard::Down:     { isMovingDown  = false; break; }
			case sf::Keyboard::Left:     { isMovingLeft  = false; break; }
			case sf::Keyboard::LShift:   { isRunning     = false; break; }
			default: break;
		}
	}
}
