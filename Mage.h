#ifndef SOLDIER_HEADER
#define SOLDIER_HEADER

#include "GameObject.h"
#include "RespawnManager.h"
#include "ZombieBlast.h"
#include "Bullet.h"
#include "Screen.h"
#include <ctime>

template<typename T> class RespawnManager;

class Mage : public Engine::GraphicalGameObject
{
private:
	friend class RespawnManager<Mage>;
	bool W_KeyHeld = false;
	bool A_KeyHeld = false;
	bool S_KeyHeld = false;
	bool D_KeyHeld = false;
	float health;
	bool alive;
	sf::Vector2f bullet_position;
	bool isShooting;
	int bullet_cooldown;
	sf::Vector2u textureSize;
	sf::Vector2u imageCount;
	sf::Vector2u currentImage;
	RespawnManager<Mage>* respawnManager = nullptr;
public:
	Mage(sf::Sprite r) : Engine::GraphicalGameObject(r)
	{
		textureSize = this->spritePtr()->getTexture()->getSize();
		textureSize.x /= 4;
		textureSize.y /= 12;
		imageCount.x = 0;
		isShooting = false;
		bullet_cooldown = 0;
		switch (rand() % 4)
		{
		case 0:
			W_KeyHeld = true;
			A_KeyHeld = false;
			S_KeyHeld = false;
			D_KeyHeld = false;
			break;
		case 1:
			A_KeyHeld = true;
			W_KeyHeld = false;
			S_KeyHeld = false;
			D_KeyHeld = false;
			break;
		case 2:
			S_KeyHeld = true;
			A_KeyHeld = false;
			W_KeyHeld = false;
			D_KeyHeld = false;
			break;
		case 3:
			D_KeyHeld = true;
			A_KeyHeld = false;
			S_KeyHeld = false;
			W_KeyHeld = false;
			break;
		}
	}
	void EveryFrame(uint64_t f)
	{
		srand(time(0));
		if (f % 120 == 0)
		{
			switch (rand() % 4)
			{
			case 0:
				W_KeyHeld = true;
				A_KeyHeld = false;
				S_KeyHeld = false;
				D_KeyHeld = false;
				break;
			case 1:
				A_KeyHeld = true;
				W_KeyHeld = false;
				S_KeyHeld = false;
				D_KeyHeld = false;
				break;
			case 2:
				S_KeyHeld = true;
				A_KeyHeld = false;
				W_KeyHeld = false;
				D_KeyHeld = false;
				break;
			case 3:
				D_KeyHeld = true;
				A_KeyHeld = false;
				S_KeyHeld = false;
				W_KeyHeld = false;
				break;
			}
		}
		if (this->W_KeyHeld)
		{
			if (!isShooting)
				imageCount.y = 3;
			this->spritePtr()->move(0, -0.5);
			this->spritePtr()->setTextureRect(sf::IntRect(imageCount.x * textureSize.x,
				imageCount.y * textureSize.y, textureSize.x, textureSize.y));
			if (f % 100 == 0 && !isShooting)
			{
				imageCount.y = 7;
				isShooting = true;
				bullet_cooldown = 0;
				bullet_position = this->spritePtr()->getPosition();
				bullet_position.x += textureSize.x / 4;
				Bullet* bullet = new Bullet(bullet_position, DIRECTION::UP);
				this->screen->add(bullet);
			}

		}
		if (this->A_KeyHeld)
		{
			if (!isShooting)
				imageCount.y = 1;
			this->spritePtr()->move(-0.5, 0);
			this->spritePtr()->setTextureRect(sf::IntRect(imageCount.x * textureSize.x,
				imageCount.y * textureSize.y, textureSize.x, textureSize.y));
			if (f % 100 == 0 && !isShooting)
			{
				imageCount.y = 5;
				isShooting = true;
				bullet_cooldown = 0;
				bullet_position = this->spritePtr()->getPosition();
				bullet_position.y += textureSize.y / 4;
				Bullet* bullet = new Bullet(bullet_position, DIRECTION::LEFT);
				this->screen->add(bullet);
			}
		}
		if (this->S_KeyHeld)
		{
			if (!isShooting)
				imageCount.y = 0;
			this->spritePtr()->move(0, 0.5);
			this->spritePtr()->setTextureRect(sf::IntRect(imageCount.x * textureSize.x,
				imageCount.y * textureSize.y, textureSize.x, textureSize.y));
			if (f % 100 == 0 && !isShooting)
			{
				imageCount.y = 4;
				isShooting = true;
				bullet_cooldown = 0;
				bullet_position = this->spritePtr()->getPosition();
				bullet_position.x += textureSize.x / 4;
				bullet_position.y += textureSize.y;
				Bullet* bullet = new Bullet(bullet_position, DIRECTION::DOWN);
				this->screen->add(bullet);
			}
		}
		if (this->D_KeyHeld)
		{
			// row 3
			if (!isShooting)
				imageCount.y = 2;
			this->spritePtr()->move(0.5, 0);
			this->spritePtr()->setTextureRect(sf::IntRect(imageCount.x * textureSize.x,
				imageCount.y * textureSize.y, textureSize.x, textureSize.y));
			if (f % 100 == 0 && !isShooting)
			{
				imageCount.y = 6;
				isShooting = true;
				bullet_cooldown = 0;
				bullet_position = this->spritePtr()->getPosition();
				bullet_position.x += textureSize.x;
				bullet_position.y += textureSize.y / 4;
				Bullet* bullet = new Bullet(bullet_position, DIRECTION::RIGHT);
				this->screen->add(bullet);
			}
		}
		// how often the sprite sheet is changing
		if (f % 15 == 0)
		{
			if (imageCount.x == 3)
				imageCount.x = 0;
			else
				imageCount.x++;
		}

		// shooting delay
		bullet_cooldown++;
		if (bullet_cooldown == 50)
		{
			isShooting = false;
		}

	}
	void Collision(GraphicalGameObject& other)
	{
		if (dynamic_cast<ZombieBlast*>(&other))
		{
			if (this->respawnManager) { this->respawnManager->died(this); }
			this->screen->remove(this);
		}
	}
	sf::Sprite* spritePtr()
	{
		return dynamic_cast<sf::Sprite*>(this->graphic);
	}
};

#endif