#include "SFML/Graphics.hpp"
#include "Screen.h"
#include "GameObject.h"
#include "FileLoadException.h"
#include <utility>
#include <functional>

using namespace Engine;

static bool renderStarted = false;
static int currentFPS;
static sf::RenderWindow* windowPtr = nullptr;
static std::queue<std::pair<GameObject*, bool>> removeQueue;
#ifdef _DEBUG
static sf::Int64 frameDurationSum;
#endif

namespace Engine
{
	typedef std::map<GameObjectID, GameObject*> GameObjectMap;
	unsigned int Screen::windowWidth = 0;
	unsigned int Screen::windowHeight = 0;
	const char* Screen::windowTitle = nullptr;
	static Screen* currentScreen;
	static Screen* pendingSwitch;
	bool running = true;
	bool windowInitialized = false;

	void addEvents(GameObject* gameObject);

	Screen::Screen() {}

	void Screen::addMap(TileMap* map)
	{
		this->tMap = map;
	}

	void Screen::addMainCharacter(GameObject* mainCharacter)
	{
		this->add(mainCharacter);
		this->mainCharacter = mainCharacter;
		mainCharacter->AddedToScreen();
	}

	GameObject* Screen::getMainCharacter() const
	{
		return this->mainCharacter;
	}

	void Screen::remove(GameObject* gameObject, bool autoDelete)
	{
		if (this != currentScreen)
		{
			for (auto map : { &this->objects, &this->gObjects, &this->uiObjects })
			{
				if (map->find(gameObject->getID()) != map->end())
				{
					GameObjectID id = gameObject->getID();
					map->erase(id);
					if (autoDelete) { delete gameObject; }
					break;
				}
			}
		}
		else
		{
			removeQueue.push({ gameObject, autoDelete});
		}
	}

	sf::Vector2i Screen::getMousePosition() const
	{
		if (!windowPtr) { return sf::Vector2i(0, 0); }
		sf::Vector2i pixelPos = sf::Mouse::getPosition(*windowPtr);
		sf::Vector2f worldPos = windowPtr->mapPixelToCoords(pixelPos, windowPtr->getView());
		return sf::Vector2i(static_cast<int>(worldPos.x), static_cast<int>(worldPos.y));
	}

	const TileMap* Screen::getMap() const
	{
		return this->tMap;
	}

	void Screen::close()
	{
		running = false;
	}

	void Screen::render(int fps)
	{
		if (fps < 1) { fps = 1; }
		else if (fps > 1000) { fps = 1000; }
		currentFPS = fps;

		unsigned int width = (Screen::windowWidth) ? Screen::windowWidth : 500;
		unsigned int height = (Screen::windowHeight) ? Screen::windowHeight : 500;
		const char* title = (Screen::windowTitle) ? Screen::windowTitle : "<no title>";
		static sf::RenderWindow window(sf::VideoMode(width, height), title, sf::Style::Close);
		static sf::Clock clock;
		static uint64_t frameCount = 0;

		sf::View view(sf::Vector2f(static_cast<float>(width / 2), static_cast<float>(height / 2)), sf::Vector2f(static_cast<float>(width), static_cast<float>(height)));
		windowPtr = &window;
		window.setView(view);

		if (renderStarted)
		{
			pendingSwitch = this;
			return;
		}
		else
		{
			pendingSwitch = nullptr;
		}
		currentScreen = this;
		renderStarted = true;

		while (window.isOpen() && !pendingSwitch)
		{
			try
			{
				clock.restart();

				//run the EveryFrame event on all objects
				for (auto map : { &currentScreen->objects, &currentScreen->gObjects, &currentScreen->uiObjects })
				{
					for (auto const& pair : *map)
					{
						GameObject* obj = pair.second;
						if (!obj->eventsDisabled) { obj->EveryFrame(frameCount); }
					}
				}

				sf::Event event;
				while (window.pollEvent(event))
				{
					if (event.type == sf::Event::Closed || !running)
					{
						window.close();
						return;
					}
					if (event.type == sf::Event::Resized)
					{
						// update the view to the new size of the window
						sf::FloatRect visibleArea(0.f, 0.f, static_cast<float>(event.size.width), static_cast<float>(event.size.height));
						view = sf::View(visibleArea);
					}
					//handle events on each object
					for (auto map : { &currentScreen->objects, &currentScreen->gObjects, &currentScreen->uiObjects })
					{
						for (auto const& pair : *map)
						{
							GameObject* obj = pair.second;
							if (obj->eventsDisabled) { continue; }
							switch (event.type)
							{
							case sf::Event::Resized:
								obj->Resized(event);
								break;
							case sf::Event::LostFocus:
								obj->LostFocus(event);
								break;
							case sf::Event::GainedFocus:
								obj->GainedFocus(event);
								break;
							case sf::Event::TextEntered:
								obj->TextEntered(event);
								break;
							case sf::Event::KeyPressed:
								obj->KeyPressed(event);
								break;
							case sf::Event::KeyReleased:
								obj->KeyReleased(event);
								break;
							case sf::Event::MouseWheelMoved:
								obj->MouseWheelMoved(event);
								break;
							case sf::Event::MouseWheelScrolled:
								obj->MouseWheelScrolled(event);
								break;
							case sf::Event::MouseButtonPressed:
								obj->MouseButtonPressed(event);
								break;
							case sf::Event::MouseButtonReleased:
								obj->MouseButtonReleased(event);
								break;
							case sf::Event::MouseMoved:
								obj->MouseMoved(event);
								break;
							case sf::Event::MouseEntered:
								obj->MouseEntered(event);
								break;
							case sf::Event::MouseLeft:
								obj->MouseLeft(event);
								break;
							case sf::Event::JoystickButtonPressed:
								obj->JoystickButtonPressed(event);
								break;
							case sf::Event::JoystickButtonReleased:
								obj->JoystickButtonReleased(event);
								break;
							case sf::Event::JoystickMoved:
								obj->JoystickMoved(event);
								break;
							case sf::Event::JoystickConnected:
								obj->JoystickConnected(event);
								break;
							case sf::Event::JoystickDisconnected:
								obj->JoystickDisconnected(event);
								break;
							case sf::Event::TouchBegan:
								obj->TouchBegan(event);
								break;
							case sf::Event::TouchMoved:
								obj->TouchMoved(event);
								break;
							case sf::Event::TouchEnded:
								obj->TouchEnded(event);
								break;
							case sf::Event::SensorChanged:
								obj->SensorChanged(event);
								break;
							default:
								break;
							}
						}
					}
				}

				window.clear();

				//draw the map

				unsigned int mapWidth = 0;
				unsigned int mapHeight = 0;

				if (currentScreen->tMap)
				{
					window.draw(*currentScreen->tMap);
					mapWidth = currentScreen->tMap->width() * currentScreen->tMap->tileSize().x;
					mapHeight = currentScreen->tMap->height() * currentScreen->tMap->tileSize().y;
				}

				//draw the objects
				for (auto const& pair : currentScreen->gObjects)
				{
					GraphicalGameObject* obj = dynamic_cast<GraphicalGameObject*>(pair.second); //does not need to be checked, they are checked on insertion into the maps

					//prevent objects from leaving the map
					if (!obj->ignoreObstacles)
					{
						if (sf::Transformable* transformable = dynamic_cast<sf::Transformable*>(obj->getGraphic()))
						{
							sf::Vector2u size(0, 0);
							if (sf::Sprite* sprite = dynamic_cast<sf::Sprite*>(obj->getGraphic())) { size = sf::Vector2u(sprite->getTextureRect().width, sprite->getTextureRect().height); }
#define X (transformable->getPosition().x)
#define Y (transformable->getPosition().y)
							if (X < 0.f) { transformable->setPosition(0.f, Y); }
							if (Y < 0.f) { transformable->setPosition(X, 0.f); }
							if (Y + size.y > mapHeight) { transformable->setPosition(X, static_cast<float>(mapHeight - size.y)); }
							if (X + size.x > mapWidth) { transformable->setPosition(static_cast<float>(mapWidth - size.x), Y); }

							sf::Vector2f offsets(0.f, 0.f);
							if (obj->obstacleCollisionSize.width > 0.f && obj->obstacleCollisionSize.height > 0.f)
							{
								offsets.x = obj->obstacleCollisionSize.left;
								offsets.y = obj->obstacleCollisionSize.top;
								size.x = static_cast<unsigned int>(obj->obstacleCollisionSize.width);
								size.y = static_cast<unsigned int>(obj->obstacleCollisionSize.height);
							}

							do
							{
								sf::Vector2f corners[4] = {
									{X + static_cast<float>(offsets.x)          , Y + static_cast<float>(offsets.y)          },
									{X + static_cast<float>(size.x + offsets.x) , Y + static_cast<float>(offsets.y)          },
									{X + static_cast<float>(offsets.x)          , Y + static_cast<float>(size.y + offsets.y) },
									{X + static_cast<float>(size.x + offsets.x) , Y + static_cast<float>(size.y + offsets.y) }
								};

								bool collision = false;
								for (auto corner : corners)
								{
									if (currentScreen->tMap->isObstacle(corner))
									{
										collision = true;
										break;
									}
								}
								if (collision)
								{
									if (obj->spawnCollisionsResolved) { transformable->setPosition(obj->lastPos); }
									else
									{
										auto positions = this->tMap->getSafeSpawnPositions();
										transformable->setPosition(positions[rand() % positions.size()]);
									}
								}
								else { obj->spawnCollisionsResolved = true; }
							} while (!obj->spawnCollisionsResolved);

							obj->lastPos = { X , Y };
#undef X
#undef Y
						}
					}
					obj->draw(window);
				}

				//draw the UI objects
				for (auto const& pair : currentScreen->uiObjects)
				{
					GraphicalGameObject* obj = dynamic_cast<GraphicalGameObject*>(pair.second);
					sf::Transformable* transformable = dynamic_cast<sf::Transformable*>(obj->getGraphic());
					if (!transformable) { continue; }
					sf::Vector2f viewPos = window.getView().getCenter();
					sf::Vector2f screenPosition = transformable->getPosition();
					transformable->setPosition(viewPos - sf::Vector2f(static_cast<float>(currentScreen->windowWidth / 2), static_cast<float>(currentScreen->windowHeight / 2)) + screenPosition);
					obj->draw(window);
					transformable->setPosition(screenPosition);
				}

				//trigger collision events				
				for (auto const& p1 : currentScreen->gObjects)
				{
					GraphicalGameObject* eventReciever = dynamic_cast<GraphicalGameObject*>(p1.second);
					if (eventReciever->eventsDisabled) { continue; }
					sf::Sprite* receiverSprite = dynamic_cast<sf::Sprite*>(eventReciever->getGraphic());
					if (!receiverSprite) { continue; }
					for (auto const& p2 : currentScreen->gObjects)
					{
						GraphicalGameObject* eventArg = dynamic_cast<GraphicalGameObject*>(p2.second);
						if (eventArg == eventReciever || !eventArg->triggerCollisionEvents || !eventReciever->triggerCollisionEvents) { continue; }
						sf::Sprite* argSprite = dynamic_cast<sf::Sprite*>(eventArg->getGraphic());
						if (!argSprite) { continue; }
						sf::FloatRect r1 = receiverSprite->getGlobalBounds();
						sf::FloatRect r2 = argSprite->getGlobalBounds();
						if (r1.intersects(r2))
						{
							eventReciever->Collision(*eventArg);
						}
					}
				}

				//view moves with character
				if (GraphicalGameObject* mainCharGraphical = dynamic_cast<GraphicalGameObject*>(mainCharacter))
				{
					if (const sf::Transformable* graphicAsTransformable = dynamic_cast<const sf::Transformable*>(mainCharGraphical->getGraphic()))
					{
						sf::Vector2f pos = graphicAsTransformable->getPosition();
						float x = pos.x;
						float y = pos.y;
						float fWidth = static_cast<float>(mapWidth);
						float fHeight = static_cast<float>(mapHeight);
						float halfWidth = static_cast<float>(windowWidth / 2);
						float halfHeight = static_cast<float>(windowHeight / 2);
						if (x > halfWidth && x < (fWidth - halfWidth)
							&& y > halfHeight && y < (fHeight - halfHeight))
						{
							view.setCenter(pos);
						}
						else if (x >= 0.f && x <= halfWidth &&
							y >= 0.f && y <= halfHeight)
						{
							view.setCenter(halfWidth, halfHeight);
						}
						else if (x >= 0.f && x <= halfWidth &&
							y >= fHeight - halfHeight && y <= fHeight)
						{
							view.setCenter(halfWidth, fHeight - halfHeight);
						}
						else if (x >= fWidth - halfWidth && x <= fWidth &&
							y >= 0.f && y <= halfHeight)
						{
							view.setCenter(fWidth - halfWidth, halfHeight);
						}
						else if (x >= fWidth - halfWidth && x <= fWidth &&
							y >= fHeight - halfHeight && y <= fHeight)
						{
							view.setCenter(fWidth - halfWidth, fHeight - halfHeight);
						}
						else if (x > halfWidth && x < fWidth - halfWidth &&
							y >= 0.f && y <= halfHeight)
						{
							view.setCenter(x, halfHeight);
						}
						else if (x > halfWidth && x < fWidth - halfWidth &&
							y >= fHeight - halfHeight && y <= fHeight)
						{
							view.setCenter(x, fHeight - halfHeight);
						}
						else if (x >= 0.f && x <= halfWidth &&
							y > halfHeight && y < fHeight - halfHeight)
						{
							view.setCenter(halfWidth, y);
						}
						else if (x >= fWidth - halfWidth && x <= fWidth &&
							y > halfHeight && y < mapHeight - halfHeight)
						{
							view.setCenter(fWidth - halfWidth, y);
						}
					}
				}
				window.setView(view);
				window.display();

				//remove objects that are pending to be removed
				while (!removeQueue.empty())
				{
					std::pair<GameObject*, bool> pRemove = removeQueue.front();
					GameObject* toRemove = pRemove.first;
					bool autoDelete = pRemove.second;
					removeQueue.pop();
					for (auto map : { &currentScreen->objects, &currentScreen->gObjects, &currentScreen->uiObjects })
					{
						if (map->find(toRemove->getID()) != map->end())
						{
							GameObjectID id = toRemove->getID();
							toRemove->RemovedFromScreen();
							map->erase(id);
							if (autoDelete) { delete toRemove; }
							break;
						}
					}
				}

			}
			catch (GameException::FileLoadException& e)
			{
				std::cout << "Failed to load file: " << e.getFileName() << std::endl;
				std::cout << " -- Fatal error. Program must terminate." << std::endl;
				window.close();
			}
			catch (...)
			{
#ifdef _DEBUG
				std::cout << "DEBUG: Unknown error." << std::endl;
#endif
				window.close();
			}
			frameCount++;

#ifdef _DEBUG
			frameDurationSum += clock.getElapsedTime().asMicroseconds();
			int avgFrameReportFrequency = 60;
			if (frameCount % avgFrameReportFrequency == 0)
			{
				std::cout << "average frame compute time (microseconds): " << (frameDurationSum / avgFrameReportFrequency) << " (max " << (1000000 / currentFPS) << " before slowdown)" << std::endl;
				frameDurationSum = 0;
			}
#endif
			while (clock.getElapsedTime().asMicroseconds() < (1000000 / currentFPS)) {}
		}

		if (pendingSwitch)
		{
			renderStarted = false;
			pendingSwitch->render();
		}
	}

	Screen::~Screen()
	{
	}
	
	class Scheduler : public GameObject
	{
	private:
		uint64_t delay;
		uint64_t countdown;
		uint16_t repeatsRemaining;
		bool infinite;
		function<void()> func;
	public:
		Scheduler( function<void()> func, uint64_t delay, uint16_t repeatCount) : delay(delay), countdown(delay), repeatsRemaining(repeatCount), infinite(repeatCount == 0), func(func) {}
		void EveryFrame(uint64_t f)
		{
			if (this->countdown == 0)
			{
				func();
				if (repeatsRemaining > 0)
				{
					repeatsRemaining--;
					this->countdown = this->delay;
				}
				else if(!this->infinite) { this->screen->remove(this); }				
			}
			else { this->countdown--; }
		}
	};

	void Screen::schedule(function<void()> func, TimeUnit::Time delay, uint16_t repeatCount)
	{
		this->add(new Scheduler(func, delay, repeatCount));
	}
}