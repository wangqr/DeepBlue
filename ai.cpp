#include "basic.h"
#include<time.h>
#include<signal.h>
#include<limits.h>
#include<stdio.h>
#include<iostream>
#include<cmath>
#include<vector>
#include<vector>
#include<stack>
#include<queue>
#include<map>
#include<set>

// TODO use multi-thread to speed up
//#include<pthread.h>

#define AI_VERSION "wangqr_0.0.1.1005"

using namespace teamstyle16;
using std::vector;
using std::cout;
using std::endl;
using std::abs;
using std::stack;
using std::queue;
using std::pair;
using std::map;
using std::set;
using std::make_pair;

const GameInfo *INFO = Info();
inline int Distance(Position pos1,Position pos2)				{return abs(pos1.x-pos2.x)+abs(pos1.y-pos2.y);}
inline int max(const int& x, const int& y)					{return x>y?x:y;}
inline int min(const int& x, const int& y)					{return x<y?x:y;}
inline bool PxyEq(const Position& a,const Position& b)		{return a.x==b.x && a.y==b.y;}
const char * GetTeamName()									{return AI_VERSION;}
inline bool InSight (const State& a, Position pos)			{return Distance(a.pos,pos)<=kProperty[a.type].sight_ranges[pos.z]+INFO->weather;}
inline bool InSight (Position pos)							{for(int i=0;i<INFO->element_num;++i){const State& a=*INFO->elements[i];if(a.team != INFO->team_num) continue;if(InSight(a,pos))return true;}return false;}
inline bool InFireRange(const State& a, Position pos)		{return Distance(a.pos,pos)<=kProperty[a.type].fire_ranges[pos.z];}
inline bool InMap(int x,int y)								{return 0<=x && x<INFO->x_max && 0<=y && y<INFO->y_max;}
inline bool InMap(Position pos)								{return InMap(pos.x,pos.y);}
Position operator+ (const Position& a, const Position& b)	{Position temp;temp.x=a.x+b.x;temp.y=a.y+b.y;temp.z=-1;return temp;}
Position operator- (const Position& a, const Position& b)	{Position temp;temp.x=a.x-b.x;temp.y=a.y-b.y;temp.z=-1;return temp;}
struct PosComp												{bool operator() (const Position& a, const Position& b)	{return a.x<b.x ||(a.x==b.x && (a.y<b.y || (a.y==b.y && a.z<b.z)));}};
const Position neighbour[4]={{1,0,-1},{-1,0,-1},{0,1,-1},{0,-1,-1}};
const Position baseNeighbour[12]={{2,0,-1},{2,1,-1},{2,-1,-1},{-2,0,-1},{-2,1,-1},{-2,-1,-1},{0,2,-1},{1,2,-1},{-1,2,-1},{0,-2,-1},{1,-2,-1},{-1,-2,-1}};
struct ExtMapType
{
	MapType map_type;
};
vector<vector<ExtMapType> > MapInfo;

struct Unit;map<int,Unit*> Units; // [IMPORTANT] All data is here.

int RouteDis(Position pos1, Position pos2) // [BFS] find distance via ocean
{
	if(pos1.x==pos2.x && pos1.y==pos2.y)return 0;
	queue<pair<int,int> >bfs;
	vector<vector<int> > vis;
	vis.resize(INFO->x_max);
	for(int i=0;i<INFO->x_max;++i)
	{
		vis[i].resize(INFO->y_max,0);
	}
	vis[pos1.x][pos1.y]=1;
	bfs.push(make_pair(pos1.x,pos1.y));
	vis[pos2.x][pos2.y]=-1;
	bfs.push(make_pair(pos2.x,pos2.y));
	while(!bfs.empty())
	{
		int x=bfs.front().first,y=bfs.front().second,z=vis[x][y];
		bfs.pop();
		for (int i=0;i<4;++i){
			int p=x+neighbour[i].x,q=y+neighbour[i].y;
			if(InMap(p,q) && vis[p][q]*z<0){
				p=vis[p][q];
				for(int i=0;i<INFO->x_max;++i)
				{
					for(int j=0;j<INFO->y_max;++j)
					{
						vis[i][j]=0;
					}
				}
				return abs(z)+abs(p)-1;
			}
			else if(InMap(p,q) && vis[p][q]==0 && MapInfo[p][q].map_type==OCEAN )
			{
				vis[p][q]=z>0?z+1:z-1;
				bfs.push(make_pair(p,q));
			}
		}
	}
	return -1;
}

int my_team;
map<Position,int,PosComp> metalPos;
map<Position,int,PosComp> fuelPos;
set<Position,PosComp> oceanNearMyBase;

Position FindMetalPos(Position pos){
	Position a={0,0,-1};
	if(metalPos.empty())return a;
	a=metalPos.begin()->first;
	int dis=RouteDis(pos,a);
	for(map<Position,int,PosComp>::iterator i=metalPos.begin();i!=metalPos.end();++i){
		int t=RouteDis(pos,i->first);
		if(t<dis){
			a=i->first;
			dis=t;
		}
	}
	return a;
}

Position FindFuelPos(Position pos){
	Position a={0,0,-1};
	if(fuelPos.empty())return a;
	a=fuelPos.begin()->first;
	int dis=RouteDis(pos,a);
	for(map<Position,int,PosComp>::iterator i=fuelPos.begin();i!=fuelPos.end();++i){
		int t=RouteDis(pos,i->first);
		if(t<dis){
			a=i->first;
			dis=t;
		}
	}
	return a;
}

Position FindWayHome(Position pos){
	Position a=*oceanNearMyBase.begin();
	int dis=RouteDis(pos,a);
	for(set<Position,PosComp>::iterator i=oceanNearMyBase.begin();i!=oceanNearMyBase.end();++i){
		int t=RouteDis(pos,*i);
		if(t<dis){
			a=*i;
			dis=t;
		}
	}
	return a;
}

int start=0;

enum UnitSignal
{
	UNDER_ATTACK
};

struct Job
{
	enum {ATTACK,REPAIR,SUPPLY,COLLECT} type;
	Position pos;
	int target;
};

enum GlobalSignal
{
	CARGO_UNDER_ATTACT,
	BASE_UNDER_ATTACT,
	UNIT_LOST,
	UNIT_GENERATED,
};

set<GlobalSignal> GSig;
inline void raise_gsig(GlobalSignal a){GSig.insert(a);}
inline void erase_gsig(GlobalSignal a){GSig.erase(a);}

struct Unit : public State
{
	set<UnitSignal> sig;
	queue<Job> job;
	int last_seen;
	virtual void sig_add(UnitSignal a){sig.insert(a);}
	virtual void sig_remove(UnitSignal a){sig.erase(a);}
	virtual void DoJob(){
		if(!job.empty()){
			Job a=job.front();
			if(a.type==a.ATTACK){
				if(!PxyEq(destination,a.pos)){
					ChangeDest(index,a.pos);
				}
				if(InFireRange(*this,Units[a.target]->pos))
					AttackUnit(index,a.target);
			}
		}
	}
	virtual bool movable(){return false;};
	virtual void InitState(const State& a){
		index=a.index;
		type=a.type;
		pos=a.pos;
		team=a.team;
		visible=a.visible;
		health=a.health;
		fuel=a.fuel;
		ammo=a.ammo;
		metal=a.metal;
		destination=a.destination;
		last_seen=INFO->round;
	}
	virtual void update_state(const State& a){
		//if(index!=a.index)return;
		if(movable() && a.visible){
			pos=a.pos;
			if(health>a.health) sig_add(UNDER_ATTACK);
			health=a.health;
			if(team==INFO->team_num){
				fuel=a.fuel;
				ammo=a.ammo;
				metal=a.metal;
			}
			last_seen=INFO->round;
		}
		else if(!movable()){
			if(type==BASE){
				if(team==INFO->team_num){
					if(health>a.health){
						sig_add(UNDER_ATTACK);
						raise_gsig(BASE_UNDER_ATTACT);
					}
					else{
						sig_remove(UNDER_ATTACK);
					}
					health=a.health;
					fuel=a.fuel;
					metal=a.metal;
					last_seen=INFO->round;
				}
				else{
					if(health>a.health){
						sig_add(UNDER_ATTACK);
					}
					else{
						sig_remove(UNDER_ATTACK);
					}
					health=a.health;
				}
			}
		}
	}
	virtual void simple_init(){}
};

struct Building : public Unit
{
	bool movable(){return false;}
};

struct Base : public Building
{
	Base():Building(){
		Position n;
		n.z=-1;
		for(int i=0;i<kElementTypes;++i){
			rally_point[i]=n;
		}
	}
	Position rally_point[kElementTypes]; // FIXME no use

};

Base* my_base;
Base* enemy_base;

struct Fort : public Building
{

};

struct Resources : public Unit
{
	bool movable(){return false;}
	bool run_out;
	Resources(){
		run_out=false;
	}
};

struct Mine : public Resources
{
	void update_state(const State& a){
		Resources::update_state(a);
		if(visible && metal==0) run_out=true;
		for(int i=1;i<4;++i){
			Position a=pos+neighbour[i];
			if(InMap(a) && MapInfo[a.x][a.y].map_type==OCEAN){
				if(run_out){
					metalPos.erase(a);
				}
				else{
					metalPos[a]=index;
				}
			}
		}
	}
};

struct OilField : public Resources
{
	void update_state(const State& a){
		Resources::update_state(a);
		if(visible && fuel==0) run_out=true;
		for(int i=1;i<4;++i){
			Position a=pos+neighbour[i];
			if(MapInfo[a.x][a.y].map_type==OCEAN){
				if(run_out){
					fuelPos.erase(a);
				}
				else{
					fuelPos[a]=index;
				}
			}
		}
	}
};

struct MovableUnit : public Unit
{
	bool movable(){return true;}
};

struct Ship : public MovableUnit
{

};

struct Submarine : public Ship
{

};

struct Destroyer : public Ship
{

};

struct Carrier : public Ship
{

};

struct Cargo : public Ship
{
	void DoJob(){
		if(!job.empty()){
			Job a=job.front();
			if(PxyEq(pos,a.pos)){
				if(a.type==a.COLLECT){
					if(Units[a.target]->type==OILFIELD){
						if(my_base->metal<kProperty[BASE].metal_max){
							a.pos=FindMetalPos(pos);
							a.target=metalPos[a.pos];
							job.pop();
							job.push(a);
						}
						else{
							a.type=a.SUPPLY;
							a.pos=FindWayHome(pos);
							a.target=my_base->index;
							job.pop();
							job.push(a);
						}
					}
					else if(Units[a.target]->type==MINE){
						a.type=a.SUPPLY;
						a.pos=FindWayHome(pos);
						a.target=my_base->index;
						job.pop();
						job.push(a);
					}
				}
				else{ // FIXME jump some process
					Position q=FindFuelPos(pos);
					int fn=RouteDis(pos,q)+5;
					//printf("    [DEBUG]  fn=%d, fuel=%d\n",fn,fuel);
					Supply(index,my_base->index,fuel-fn,0,metal);
					a.type=a.COLLECT;
					a.pos=q;
					a.target=fuelPos[q];
					job.pop();
					job.push(a);
				}
			}
			a=job.front();
			ChangeDest(index,a.pos);
		}
	}
	void simple_init(){
		Job a;
		Position q=FindFuelPos(pos);
		int fn=RouteDis(pos,q)+5;
		if(fn<fuel){
			Supply(index,my_base->index,fuel-fn,0,0);
		}
		else if(fn>fuel){
			Supply(my_base->index,index,fn-fuel,0,0);
		}
		a.type=a.COLLECT;
		a.pos=q;
		a.target=fuelPos[q];
		job.push(a);
	}
};

struct AirForce : public MovableUnit
{

};

struct Fighter : public AirForce
{

};

struct Scout : public AirForce
{

};

inline bool HasSig(const Unit& a,const UnitSignal& b){return a.sig.find(b)!=a.sig.end();}

void update(){
	TryUpdate();
	GSig.clear();
	if(start==0){ // Run once
		start=1;
		my_team=INFO->team_num;
		MapInfo.resize(INFO->x_max);
		for(int i=0;i<INFO->x_max;++i)
		{
			MapInfo[i].resize(INFO->y_max);
			for(int j=0;j<INFO->y_max;++j)
			{
				MapInfo[i][j].map_type=Map(i,j);
			}
		}
	}

	for(int i=0;i<INFO->element_num;++i){
		const State* j=INFO->elements[i];
		map<int,Unit*>::iterator t=Units.find(j->index);
		if(t==Units.end()){
			if(j->type==BASE){
				if(j->team==INFO->team_num){
					my_base=new Base;
					Units[j->index]=my_base;
					my_base->InitState(*j);
					for(int i=0;i<12;++i){
						Position a=baseNeighbour[i]+my_base->pos;
						if(InMap(a) && MapInfo[a.x][a.y].map_type==OCEAN)
							oceanNearMyBase.insert(a);
					}
				}
				else{
					enemy_base=new Base;
					Units[j->index]=enemy_base;
					enemy_base->InitState(*j);
				}
			}
			else if(j->type==FORT){
				Units[j->index]=new Fort;
				Units[j->index]->InitState(*j);
			}
			else if(j->type==MINE){
				Units[j->index]=new Mine;
				Units[j->index]->InitState(*j);
			}
			else if(j->type==OILFIELD){
				Units[j->index]=new OilField;
				Units[j->index]->InitState(*j);
			}
			else if(j->type==SUBMARINE){
				Units[j->index]=new Submarine;
				Units[j->index]->InitState(*j);
			}
			else if(j->type==DESTROYER){
				Units[j->index]=new Destroyer;
				Units[j->index]->InitState(*j);
			}
			else if(j->type==CARRIER){
				Units[j->index]=new Carrier;
				Units[j->index]->InitState(*j);
			}
			else if(j->type==CARGO){
				Units[j->index]=new Cargo;
				Units[j->index]->InitState(*j);
			}
			else if(j->type==FIGHTER){
				Units[j->index]=new Fighter;
				Units[j->index]->InitState(*j);
			}
			else if(j->type==SCOUT){
				Units[j->index]=new Scout;
				Units[j->index]->InitState(*j);
			}
		}
		else{
			t->second->update_state(*j);
		}
	}

	for(map<int,Unit*>::iterator i=Units.begin();i!=Units.end();++i){
		if(i->second->team==my_team && i->second->last_seen<INFO->round){
			delete i->second;
			Units.erase(i);
			i=Units.begin();
			raise_gsig(UNIT_LOST);
		}
	}


}

void AIMain()
{  
	update();

	// 
	// 
	// ==========[MAIN AI PART BEGIN]==========
	// 
	// 

	{
		int c=0;
		for(map<int,Unit*>::iterator i=Units.begin();i!=Units.end();++i){
			if(i->second->team==my_team && i->second->type==CARGO)
				++c;
		}
		for(int i=0;i<INFO->production_num;++i){
			if(INFO->production_list[i].unit_type==CARGO)
				++c;
		}
		while(c<1){
			Produce(CARGO);
			++c;
		}
	}

	int j=my_base->fuel;
	for(int i=0;i<INFO->production_num;++i){
		j-=(kProperty[INFO->production_list[i].unit_type].fuel_max
			-((INFO->production_list[i].unit_type==FIGHTER ||
			INFO->production_list[i].unit_type==SCOUT)?1:0));
	}
	for(int i=0;i<min(my_base->metal/kProperty[FIGHTER].cost,
		j/(kProperty[FIGHTER].fuel_max-1));++i){
			Produce(FIGHTER);
	}

	for(map<int,Unit*>::iterator i=Units.begin();i!=Units.end();++i){
		Unit& obj=*(i->second);
		if(obj.type==FIGHTER && obj.job.empty()){
			Job a;
			a.type=a.ATTACK;
			a.target=enemy_base->index;
			a.pos=obj.pos+(enemy_base->pos-my_base->pos);
			obj.job.push(a);
		}
		if(obj.type==CARGO && obj.job.empty()){
			obj.simple_init();
		}
	}
	//
	//
	// ==========[MAIN AI PART END]==========
	//
	//

	for(map<int,Unit*>::iterator i=Units.begin();i!=Units.end();++i){
		if(i->second->team==my_team)
			i->second->DoJob();
	}
}
