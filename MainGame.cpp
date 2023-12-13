#define PLAY_IMPLEMENTATION
#define PLAY_USING_GAMEOBJECT_MANAGER
#include "Play.h"

int DISPLAY_WIDTH = 1280;
int DISPLAY_HEIGHT = 720;
int DISPLAY_SCALE = 1;


struct GameState
{
	int score = 0;
	int toolSpawnSeed = 100;
	int coinSpawnSeed = 100;

};

enum GameObjectType {
	TYPE_NULL = -1,
	TYPE_AGENT8, //0
	TYPE_COLLECTIBLE,
	TYPE_FAN,
	TYPE_TOOL,
	TYPE_COIN,
	TYPE_STAR,
	TYPE_LASER,
	TYPE_DESTROYED
};

GameState game_state;


void HandlePlayerControls(GameObject& agent8)
{
	
	if (Play::KeyDown(VK_UP))
	{
		agent8.velocity = { 0,-4 };
		Play::SetSprite(agent8, "agent8_climb", 0.25f);
	}
	else if (Play::KeyDown(VK_DOWN))
	{
		agent8.acceleration = { 0,1 };
		Play::SetSprite(agent8, "agent8_fall", 0.25f);
	}
	else 
	{
		Play::SetSprite(agent8, "agent8_hang", 0.25f);
		agent8.acceleration = { 0,0 };
		agent8.velocity *= 0.5f;
	}
	Play::UpdateGameObject(agent8);

	if (Play::IsLeavingDisplayArea(agent8))
	{
		agent8.pos = agent8.oldPos;
	}
	Play::DrawLine({ agent8.pos.x, 0 }, agent8.pos, Play::cWhite);
	Play::DrawObjectRotated(agent8);

};

//generate and launch the tools
void UpdateFan() {
	GameObject& obj_fan = Play::GetGameObjectByType(TYPE_FAN);
	
	//spawning some obstacles
	if (Play::RandomRoll(game_state.toolSpawnSeed) == game_state.toolSpawnSeed)
	{
		//spawn some object
		int id = Play::CreateGameObject(TYPE_TOOL, obj_fan.pos, 50, "driver");
		GameObject& obj_tool = Play::GetGameObject(id);
		obj_tool.velocity = Point2f(-8, Play::RandomRollRange(-1, 1) * 6);


		//change ~50% of sprites to be a wrench
		if (Play::RandomRoll(2) == 1)
		{
			Play::SetSprite(obj_tool, "spanner", 0);
			obj_tool.radius = 100;
			obj_tool.velocity.x = -4;
			obj_tool.rotSpeed = 0.1f;
			Play::UpdateGameObject(obj_tool);
		}
		Play::PlayAudio("tool");
	}

	//spawn some coins
	if (Play::RandomRoll(game_state.coinSpawnSeed) == game_state.coinSpawnSeed)
	{
		int id = Play::CreateGameObject(TYPE_COIN, obj_fan.pos, 40, "coin");
		GameObject& obj_coin = Play::GetGameObject(id);
		obj_coin.velocity = { -3, 0 };
		obj_coin.rotSpeed = 0.1f;

	}

	//redirect fan to stay onscreen
	if (Play::IsLeavingDisplayArea(obj_fan)) 
	{
		obj_fan.pos = obj_fan.oldPos;
		obj_fan.velocity *= -1;

	}


	//aninmating fan
	Play::SetSprite(obj_fan, "fan", 0.25f);
	Play::UpdateGameObject(obj_fan);
	Play::DrawObjectRotated(obj_fan);
};

//move the tools along their path, and detect collisions
void UpdateTools(GameObject& agent8) {

	std::vector<int> v_tools = Play::CollectGameObjectIDsByType(TYPE_TOOL);

	for (int id : v_tools) {
		GameObject& obj_tool = Play::GetGameObject(id);

		if (Play::IsColliding(obj_tool, agent8))
		{
			//game over
			Play::StopAudioLoop("music");
			Play::PlayAudio("die");
			agent8.pos = { -100,-100 };
			Play::UpdateGameObject(agent8);
		}
		Play::UpdateGameObject(obj_tool);

		if (Play::IsLeavingDisplayArea(obj_tool, Play::VERTICAL))
		{
			obj_tool.pos = obj_tool.oldPos;
			obj_tool.velocity.y *= -1;
		}

		Play::DrawObjectRotated(obj_tool);
	}
	
};

//move coins and generate stars
void UpdateCoinsAndStars(GameObject& agent8) {
	std::vector<int> v_coins = Play::CollectGameObjectIDsByType(TYPE_COIN);
	//update the state of all the coins
	for (int id : v_coins) {
		GameObject& obj_coin = Play::GetGameObject(id);
		bool hasCollided = false;
		
		//if coin is colliding with agent8, increment score
		if (Play::IsColliding(agent8, obj_coin))
		{
			//1/4pi, 3/4pi, 1 1/4p1, 1 3/4pi,
			for (float rad {0.25f }; rad < 2; rad += 0.5f)
			{
				int star_id = Play::CreateGameObject(TYPE_STAR, agent8.pos, 0, "star");
				GameObject& obj_star = Play::GetGameObject(star_id);

				obj_star.rotSpeed = 0.1f;
				obj_star.acceleration = {0.0f, 0.5f};

				Play::SetGameObjectDirection(obj_star, 16, rad * PLAY_PI);
			}

			hasCollided = true;
			game_state.score += 500;
			Play::PlayAudio("collect");
		}
		
		//update coin position and draw to screen
		Play::UpdateGameObject(obj_coin);
		Play::DrawObjectRotated(obj_coin);

		//if coin is offscreen, or has collided with agent, destroy coin
		if( !Play::IsVisible(obj_coin) || hasCollided)
		{
			Play::DestroyGameObject(id);
		}

		//iterate over stars, move them, and destroy them when offscreen

		//get all stars
		std::vector<int> v_stars = Play::CollectGameObjectIDsByType(TYPE_STAR);
		for (int id_star : v_stars)
		{
			GameObject& star = Play::GetGameObject(id_star);
			Play::UpdateGameObject(star);
			Play::DrawObjectRotated(star);

			//see if star has left screen
			if (!Play::IsVisible(star))
			{
				Play::DestroyGameObject(id_star);
			}
		}
	}

}

// The entry point for a PlayBuffer program
void MainGameEntry( PLAY_IGNORE_COMMAND_LINE )
{
	Play::CreateManager(DISPLAY_WIDTH, DISPLAY_HEIGHT, DISPLAY_SCALE);
	Play::CreateGameObject(TYPE_AGENT8, { 115, 300 }, 50, "agent8");
	
	int fan_id = Play::CreateGameObject(TYPE_FAN, { 1140, 217}, 0, "fan");

	GameObject& fan_obj = Play::GetGameObjectByType(fan_id);

	fan_obj.velocity = { 0,3 };
	fan_obj.animSpeed = .25f;


	Play::LoadBackground("Data\\Backgrounds\\background.png");
	//Play::StartAudioLoop("music");
	Play::CentreAllSpriteOrigins();
}


// Called by PlayBuffer every frame (60 times a second!)
bool MainGameUpdate( float elapsedTime )
{
	Play::ClearDrawingBuffer( Play::cOrange );
	Play::DrawBackground();

	GameObject& agent8 = Play::GetGameObjectByType(TYPE_AGENT8);

	HandlePlayerControls(agent8);
	UpdateFan();
	UpdateTools(agent8);
	UpdateCoinsAndStars(agent8);

	Play::PresentDrawingBuffer();
	
	return Play::KeyDown( VK_ESCAPE );
}

// Gets called once when the player quits the game 
int MainGameExit( void )
{
	Play::DestroyManager();
	return PLAY_OK;
}

