#include "../entity/entity/World.hpp"
#include <iostream>
#include <time.h>

using namespace entity;

//#define DETAIL_LOG 1


World* g_pWorld = NULL;
World& GetWorld()
{
	return *g_pWorld;
}
class Utils
{
public:
	static bool FindFood(Entity& e, const char* foodType);
	static void KillObj(Entity& e);
};

struct LifeComponent
{
	LifeComponent() { alive = true; }
	bool alive;
};

struct AgeComponent
{
	AgeComponent(int maxAge = 1) : maxAge(maxAge) { age = 1; }
	int maxAge;
	int age;
};

enum eObjType
{
	Obj_Grass,
	Obj_Sheep,
	Obj_Wolf
};

class BaseObject
{
public:
	virtual void OnBorned() {}
	virtual void OnDead() {}
};

class ObjGrass:public BaseObject
{
	static int grass_num;
public:
	void OnBorned() override
	{
		grass_num++;
	}

	void OnDead() override
	{
		grass_num--;
	}
	static int GetGrassNum() { return grass_num;  }
	static void InitGrassNum() { grass_num = 0; }
};

int ObjGrass::grass_num = 0;

struct ObjectTypeComponent
{
	ObjectTypeComponent(int objType = 0, BaseObject* pObj = NULL):objType(objType),pObj(pObj){ }
	int objType;
	// 目前并没有考虑释放
	BaseObject *pObj;
};

// 繁殖
struct SpawnComponent
{
	SpawnComponent(int interval = 0) :spawnInterval(interval) 
	{
		spawnCounter = 0;
		foodEnough = true;
	}

	bool foodEnough;
	int spawnInterval;
	int spawnCounter;
};

struct FoodComponent
{
	FoodComponent(int food = 1):food(food){}
	int food;
};

// 饥饿度
struct HungerComponent
{
	HungerComponent(int maxFood = 1, int cost = 1)
		:maxFood(maxFood),food(maxFood/5),cost(cost)
	{
		hunger = 0;
	}
	int maxFood;
	int food;
	int cost;	
	int hunger;
};

struct SheepComponent
{
};

struct WolfComponent
{
};



class FactorySystem : public System
{
public:
	FactorySystem()
	{
	}

	void CreateGrass()
	{
		World &world = GetWorld();

		if (ObjGrass::GetGrassNum() > 100)
			return;

		auto e = world.CreateEntity();

		ObjGrass* pGrass = new ObjGrass();

		e.AddComponent<ObjectTypeComponent>(Obj_Grass, pGrass);
		e.AddComponent<LifeComponent>();
		e.AddComponent<AgeComponent>(10);
		e.AddComponent<SpawnComponent>(2);
		e.AddComponent<FoodComponent>(1);
		e.Group("grass");

		pGrass->OnBorned();

#ifdef DETAIL_LOG
		std::cout << "Create grass:"<< e.GetIndex() << std::endl;
#endif
	}

	void CreateSheep()
	{
		World &world = GetWorld();
		auto e = world.CreateEntity();

		auto pObj = new BaseObject();

		e.AddComponent<ObjectTypeComponent>(Obj_Sheep, pObj);
		e.AddComponent<LifeComponent>();
		e.AddComponent<AgeComponent>(20);
		e.AddComponent<SpawnComponent>(5);
		e.AddComponent<FoodComponent>(3);
		e.AddComponent<HungerComponent>(10,1);
		e.AddComponent<SheepComponent>();
		e.Group("sheep");

		pObj->OnBorned();

#ifdef DETAIL_LOG
		std::cout << "Create sheep:" << e.GetIndex() << std::endl;
#endif
	}

	void CreateWolf()
	{
		World &world = GetWorld();
		auto e = world.CreateEntity();

		auto pObj = new BaseObject();

		e.AddComponent<ObjectTypeComponent>(Obj_Sheep, pObj);
		e.AddComponent<LifeComponent>();
		e.AddComponent<AgeComponent>(20);
		e.AddComponent<SpawnComponent>(10);
		e.AddComponent<FoodComponent>(5);
		e.AddComponent<HungerComponent>(10, 2);
		e.AddComponent<WolfComponent>();
		e.Group("wolf");

		pObj->OnBorned();

#ifdef DETAIL_LOG
		std::cout << "Create sheep:" << e.GetIndex() << std::endl;
#endif
	}
};

class SpawnSystem : public System
{
public:
	SpawnSystem()
	{
		RequireComponent<LifeComponent>();
		RequireComponent<SpawnComponent>();
		RequireComponent<ObjectTypeComponent>();
	}

	void Update()
	{
		for (auto e : GetEntities())
		{
			auto &life = e.GetComponent<LifeComponent>();
			if (!life.alive)
				continue;

			// spawn
			auto &spawn = e.GetComponent<SpawnComponent>();
			auto &obj = e.GetComponent<ObjectTypeComponent>();

			spawn.spawnCounter++;
			if (spawn.spawnCounter >= spawn.spawnInterval)
			{
				spawn.spawnCounter = 0;

				if (spawn.foodEnough)
				{
					// 食物是否充足
					if (obj.objType == Obj_Grass)
					{
						GetWorld().GetSystemManager().GetSystem<FactorySystem>().CreateGrass();
					}
					if (obj.objType == Obj_Sheep)
					{
						GetWorld().GetSystemManager().GetSystem<FactorySystem>().CreateSheep();
					}
					if (obj.objType == Obj_Wolf)
					{
						GetWorld().GetSystemManager().GetSystem<FactorySystem>().CreateWolf();
					}
				}
			}
		}
	}
};

class AgeSystem : public System
{
public:
	AgeSystem()
	{
		RequireComponent<LifeComponent>();
		RequireComponent<AgeComponent>();
		RequireComponent<ObjectTypeComponent>();
	}

	void Update()
	{
		for (auto e : GetEntities())
		{
			if (!e.IsAlive())
				continue;
			auto &life = e.GetComponent<LifeComponent>();
			if (!life.alive)
				continue;

			//
			auto &age = e.GetComponent<AgeComponent>();

			age.age++;
			if (age.age > age.maxAge)
			{
				Utils::KillObj(e);
			}
		}
	}
};

class FoodCostSystem : public System
{
public:
	FoodCostSystem()
	{	
		RequireComponent<LifeComponent>();
		RequireComponent<ObjectTypeComponent>();
		RequireComponent<HungerComponent>();
	}

	void Update()
	{
		for (auto e : GetEntities())
		{
			if (!e.IsAlive())
				continue;
			auto &life = e.GetComponent<LifeComponent>();
			if (!life.alive)
				continue;

			auto &hunger = e.GetComponent<HungerComponent>();

			if (hunger.food <= 0)
			{
				hunger.hunger++;

				if (e.HasComponent<SpawnComponent>())
				{
					auto &spawn = e.GetComponent<SpawnComponent>();
					spawn.foodEnough = false;
				}

				// 饿死
				if (hunger.hunger > 5)
				{
					Utils::KillObj(e);
				}
			}
			else
			{
				hunger.food -= hunger.cost;
			}
		}
	}
};

class SheepSystem : public System
{
public:
	SheepSystem()
	{
		RequireComponent<LifeComponent>();
		RequireComponent<AgeComponent>();
		RequireComponent<ObjectTypeComponent>();
		RequireComponent<HungerComponent>();
		RequireComponent<SheepComponent>();
	}

	void Update()
	{
		// 吃草
		for (auto e : GetEntities())
		{
			if (!e.IsAlive())
				continue;
			auto &life = e.GetComponent<LifeComponent>();
			if (!life.alive)
				continue;

			// find grass to eat
			Entity grass;
			if( !FindGrassToEat(grass) )			
				continue;			

			auto &hunger = e.GetComponent<HungerComponent>();
			auto &food = grass.GetComponent<FoodComponent>();

			hunger.food += food.food;
			if (hunger.food > hunger.maxFood)
				hunger.food = hunger.maxFood;

			if (hunger.food > hunger.maxFood/2 && e.HasComponent<SpawnComponent>())
			{
				auto &spawn = e.GetComponent<SpawnComponent>();
				spawn.foodEnough = true;
			}

			Utils::KillObj(grass);
		}
	}

	bool FindGrassToEat(Entity& ret)
	{
		return Utils::FindFood(ret, "grass");
	}
};

class WolfSystem : public System
{
public:
	WolfSystem()
	{
		RequireComponent<LifeComponent>();
		RequireComponent<AgeComponent>();
		RequireComponent<ObjectTypeComponent>();
		RequireComponent<HungerComponent>();
		RequireComponent<WolfComponent>();
	}

	void Update()
	{
		// 吃sheep
		for (auto e : GetEntities())
		{
			if (!e.IsAlive())
				continue;
			auto &life = e.GetComponent<LifeComponent>();
			if (!life.alive)
				continue;

			auto &hunger = e.GetComponent<HungerComponent>();
			if (hunger.food >= hunger.maxFood / 2)
				continue;

			// find grass to eat
			Entity foodE;
			if (!FindSheepToEat(foodE))
				continue;

			
			auto &food = foodE.GetComponent<FoodComponent>();

			hunger.food += food.food;
			if (hunger.food > hunger.maxFood)
				hunger.food = hunger.maxFood;

			if (hunger.food > hunger.maxFood / 2 && e.HasComponent<SpawnComponent>())
			{
				auto &spawn = e.GetComponent<SpawnComponent>();
				spawn.foodEnough = true;
			}

			Utils::KillObj(foodE);
		}
	}

	bool FindSheepToEat(Entity& ret)
	{
		return Utils::FindFood(ret, "sheep");
	}
};


bool Utils::FindFood(Entity& ret, const char* foodType)
{
	for (auto e : GetWorld().GetEntityGroup(foodType))
	{
		if (!e.IsAlive())
			continue;
		auto &life = e.GetComponent<LifeComponent>();
		if (!life.alive)
			continue;
		ret = e;
		return true;
	}
	return false;
}

void Utils::KillObj(Entity& e)
{
	auto &life = e.GetComponent<LifeComponent>();
	auto &objType = e.GetComponent<ObjectTypeComponent>();

	life.alive = false;
	e.Kill();

	objType.pObj->OnDead();

#ifdef DETAIL_LOG
	std::cout << "Kill obj:" << e.GetIndex() << std::endl;
#endif
}


void InitSpawnObjs(FactorySystem &factory, int grass, int sheep, int wolf)
{
	{
		for (int i = 0; i < grass; i++)
			factory.CreateGrass();
	}
	{
		for (int i = 0; i < sheep; i++)
			factory.CreateSheep();
	}
	{
		for (int i = 0; i < wolf; i++)
			factory.CreateWolf();
	}
}

void UpdateWorld(World &world)
{
	world.Update();
	world.GetSystemManager().GetSystem<SpawnSystem>().Update();
	world.GetSystemManager().GetSystem<AgeSystem>().Update();
	world.GetSystemManager().GetSystem<FoodCostSystem>().Update();
	world.GetSystemManager().GetSystem<SheepSystem>().Update();
	world.GetSystemManager().GetSystem<WolfSystem>().Update();
}

struct SimulateResult
{
	int grass;
	int sheep;
	int wolf;
	int rounds;
};

struct SimulateResult curResult;
struct SimulateResult bestResult;

SimulateResult GetBestResult(SimulateResult left, SimulateResult right)
{
	if (left.rounds > right.rounds)
		return left;
	return right;
}

void RunToEnd(World &world)
{
#ifdef DETAIL_LOG
	std::cout << "Init grass:" << world.GetEntityGroup("grass").size() 
		<< " sheep:" << world.GetEntityGroup("sheep").size()
		<< " wolf:" << world.GetEntityGroup("wolf").size()
		<< std::endl;
#endif

	curResult.grass = world.GetEntityGroup("grass").size();
	curResult.sheep = world.GetEntityGroup("sheep").size();
	curResult.wolf = world.GetEntityGroup("wolf").size();

	int round = 0;
	while (true)
	{
		round++;
		UpdateWorld(world);

#ifdef DETAIL_LOG
		std::cout 
			<< "round:" << round
			<< " grass:" << world.GetEntityGroup("grass").size()
			<< " sheep:" << world.GetEntityGroup("sheep").size()
			<< " wolf:" << world.GetEntityGroup("wolf").size()
			<< std::endl;
#endif

		bool hasWolf = world.GetEntityGroup("wolf").size() > 0;
		bool hasSheep = world.GetEntityGroup("sheep").size() > 0;
		if (!hasWolf && !hasSheep)
			break;		
	}
	std::cout << "All end.Runned " << round << " rounds." << std::endl;
	curResult.rounds = round;

	bestResult = GetBestResult(curResult, bestResult);
}

void RunWorldOnce()
{
	g_pWorld = new World;
	ObjGrass::InitGrassNum();

	World &world = GetWorld();

	world.GetSystemManager().AddSystem<FactorySystem>();
	world.GetSystemManager().AddSystem<SpawnSystem>();
	world.GetSystemManager().AddSystem<AgeSystem>();
	world.GetSystemManager().AddSystem<FoodCostSystem>();
	world.GetSystemManager().AddSystem<SheepSystem>();
	world.GetSystemManager().AddSystem<WolfSystem>();

	FactorySystem &factory = world.GetSystemManager().GetSystem<FactorySystem>();


	int grass = 50 + rand() % 50;
	int sheep = 1 + rand() % 10;
	int wolf = 1 + rand() % 5;

	std::cout		
		<< " grass:" << grass
		<< " sheep:" << sheep
		<< " wolf:" << wolf
		<< std::endl;

	InitSpawnObjs(factory, grass, sheep, wolf);
	RunToEnd(world);
}

void InitRandSeed()
{
	time_t timep;
	time(&timep);
	srand(timep);
}

int grasslands_main()
{
	const int times = 300;

	InitRandSeed();
	
	bestResult.rounds = 0;
	for( int i = 0 ; i < times; i ++ )
		RunWorldOnce();	


	std::cout << "Best result: " << std::endl;
	std::cout
		<< " grass:" << bestResult.grass
		<< " sheep:" << bestResult.sheep
		<< " wolf:" << bestResult.wolf
		<< " rounds:" << bestResult.rounds
		<< std::endl;
    return 0;
}
