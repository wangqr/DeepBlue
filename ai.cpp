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

#define AI_VERSION "v1.0"

#define MAX_FIGHTER_WAITING 9

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
inline bool InSight (const State& a, const Position pos)		{return Distance(a.pos,pos)<=kProperty[a.type].sight_ranges[pos.z]+INFO->weather;}
inline bool InSight (const Position pos)						{for(int i=0;i<INFO->element_num;++i){const State& a=*INFO->elements[i];if(a.team != INFO->team_num) continue;if(InSight(a,pos))return true;}return false;}
inline bool InFireRange(const State& a, const Position pos)	{return Distance(a.pos,pos)<=kProperty[a.type].fire_ranges[pos.z];}
inline bool InFireRange(const State& a, const State& b)		{if((kProperty[a.type].attacks[0]==0 || kProperty[b.type].defences[0]==-1) &&(kProperty[a.type].attacks[1]==0 || kProperty[b.type].defences[1]==-1)) return false;return InFireRange(a,b.pos);}
inline bool InMap(int x,int y)								{return 0<=x && x<INFO->x_max && 0<=y && y<INFO->y_max;}
inline bool InMap(const Position& pos)						{return InMap(pos.x,pos.y);}
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
int my_team;
int RouteDis(Position pos1, Position pos2) // BFS find distance via ocean
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
map<Position,int,PosComp> metalPos;
map<Position,int,PosComp> fuelPos;
set<Position,PosComp> oceanNearMyBase;
int start=0;
struct UsedPos : public Position
{
	int round;
};
struct UsedPosComp
{
	bool operator() (const UsedPos& a, const UsedPos& b)	{
		if(a.round<b.round)
			return true;
		if(a.round>b.round)
			return false;
		return a.x<b.x ||(a.x==b.x && (a.y<b.y || (a.y==b.y && a.z<b.z)));
	}
};
map<UsedPos,int,UsedPosComp> usedPos;

enum UnitSignal
{
	UNDER_ATTACK
};

inline int GetDamage(const State& source, const State& target){
	int distance=Distance(source.pos,target.pos);
	int fire_range = kProperty[source.type].fire_ranges[kProperty[target.type].level];
	int modified_attacks[2];
	modified_attacks[0] = int((1 - float(distance - fire_range / 2) / (fire_range + 1)) * kProperty[source.type].attacks[0]);
	modified_attacks[1] = int((1 - float(distance - fire_range / 2) / (fire_range + 1)) * kProperty[source.type].attacks[1]);
	int fire_damage = max(0, modified_attacks[0] - kProperty[target.type].defences[0]);
	int torpedo_damage = max(0, modified_attacks[1] - kProperty[target.type].defences[1]);
	return fire_damage + torpedo_damage;
}

struct Job
{
	enum {ATTACK,REPAIR,SUPPLY,COLLECT} type;
	Position pos;
	int target;
};

enum GlobalSignal
{
	CARGO_UNDER_ATTACT,
	BASE_UNDER_ATTACK,
	UNIT_LOST,
	UNIT_GENERATED,
};
set<GlobalSignal> GSig;
inline void RaiseGsig(GlobalSignal a){GSig.insert(a);}
inline void EraseGsig(GlobalSignal a){GSig.erase(a);}
inline bool HasGSig(const GlobalSignal& a){
	return GSig.find(a)!=GSig.end();
}


struct Unit : public State
{
	set<UnitSignal> sig;
	queue<Job> job;
	int last_command_send_round;
	int last_seen;
	bool did_action;
	virtual void sig_add(UnitSignal a){sig.insert(a);}
	virtual void sig_remove(UnitSignal a){sig.erase(a);}
	virtual void DoJob();
	virtual bool movable()=0;
	virtual bool fireable()=0;
	virtual void InitState(const State& a){
		index=a.index;
		type=a.type;
		pos=a.pos;
		team=a.team;
		health=a.health;
		fuel=a.fuel;
		if(type==OILFIELD && !a.visible) fuel=1000;
		ammo=a.ammo;
		metal=a.metal;
		if(type==MINE && !a.visible) metal=500;
		destination=a.destination;
		last_seen=INFO->round;
	}
	virtual void UpdateState(const State& a){
		if(!a.visible) return;
		last_seen=INFO->round;
		if(movable()){
			pos=a.pos;
			if(health>a.health) sig_add(UNDER_ATTACK);
			health=a.health;
			if(team==INFO->team_num){
				fuel=a.fuel;
				ammo=a.ammo;
				metal=a.metal;
			}
			UsedPos p;
			p.x=pos.x;
			p.y=pos.y;
			p.z=pos.z;
			p.round=INFO->round;
			printf("[DEBUG] used pos (%d,%d,%d,%d)\n",p.x,p.y,p.z,p.round);
			usedPos[p]=index;
		}
		else if(type==BASE){
			if(team==INFO->team_num){
				if(health>a.health){
					sig_add(UNDER_ATTACK);
					RaiseGsig(BASE_UNDER_ATTACK);
				}
				else{
					sig_remove(UNDER_ATTACK);
				}
				health=a.health;
				fuel=a.fuel;
				metal=a.metal;
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
		else if(type==OILFIELD){
			fuel=a.fuel;
		}
		else if(type==MINE){
			metal=a.metal;
		}
	}
	virtual void SimpleInit(){}
};
map<int,Unit*> Units;

inline bool HasSig(const Unit& a,const UnitSignal& b){return a.sig.find(b)!=a.sig.end();}

struct Building : public Unit
{
	bool movable(){return false;}
	bool fireable(){return true;}
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
	Position rally_point[kElementTypes]; // [FIXME] no use
};
Base* my_base;Base* enemy_base;


void Unit::DoJob(){
	if(!job.empty()){
		Job a=job.front();
		if(a.type==a.ATTACK){
			if(!PxyEq(destination,a.pos)){
				ChangeDest(index,a.pos);
			}
			if(InFireRange(*this,*Units[a.target])){
				if(GetDamage(*Units[index],*Units[a.target])>0){
					AttackUnit(index,a.target);
					Units[a.target]->health-=GetDamage(*Units[index],*Units[a.target]);
					last_command_send_round=INFO->round;
				}
			}
		}
	}
	if(fireable() && last_command_send_round<INFO->round){
		vector<int> enemy_in_sight;
		for(int i=0;i<INFO->element_num;++i){
			const State* j=INFO->elements[i];
			if(j->team==1-my_team && InFireRange(*this,*j)){
				enemy_in_sight.push_back(j->index);
			}
		}
		for(unsigned i=0;i<enemy_in_sight.size();++i){
			if(Units[enemy_in_sight[i]]->health<=0) continue;
			int t=GetDamage(*this,*Units[enemy_in_sight[i]]);
			if(t>0){
				AttackUnit(index,enemy_in_sight[i]);
				Units[enemy_in_sight[i]]->health-=t;
				break;
			}
		}
		/*
		else{
			enemy_in_sight.clear();
			for(int i=0;i<INFO->element_num;++i){
				const State* j=INFO->elements[i];
				if(j->team==2 && InFireRange(*this,*j)){
					enemy_in_sight.push_back(j->index);
				}
			}
			if(!enemy_in_sight.empty()){
				// [TODO] attacking order
				AttackUnit(index,enemy_in_sight.front());
			}
		}
		*/
	}
}
struct Fort : public Building
{

};

struct Resources : public Unit
{
	bool movable(){return false;}
	bool fireable(){return false;}
	bool run_out;
	Resources(){
		run_out=false;
	}
};

struct Mine : public Resources
{
	void UpdateState(const State& a){
		Resources::UpdateState(a);
		if(metal==0) run_out=true;
		for(int i=1;i<4;++i){
			Position a=pos+neighbour[i];
			a.z=SURFACE;
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
	void UpdateState(const State& a){
		Resources::UpdateState(a);
		if(fuel==0) run_out=true;
		for(int i=1;i<4;++i){
			Position a=pos+neighbour[i];
			a.z=SURFACE;
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

Position FindMetalPos(Position pos){
	Position a={0,0,-1};
	UsedPos q;
	int dis=INT_MAX;
	for(map<Position,int,PosComp>::iterator i=metalPos.begin();i!=metalPos.end();++i){
		int t=RouteDis(pos,i->first);
		q.x=i->first.x;
		q.y=i->first.y;
		q.z=i->first.z;
		q.round=INFO->round+(t-1)/kProperty[CARGO].speed;
		if(usedPos.find(q)!=usedPos.end()){
			printf("[DEBUG] find pos used by metal (%d,%d,%d)\n",q.x,q.y,q.z);
			continue;
		}
		if(t<dis){
			a=i->first;
			dis=t;
		}
	}
	return a;
}

Position FindFuelPos(Position pos){
	Position a={0,0,-1};
	UsedPos q;
	int dis=INT_MAX;
	for(map<Position,int,PosComp>::iterator i=fuelPos.begin();i!=fuelPos.end();++i){
		int t=RouteDis(pos,i->first);
		q.x=i->first.x;
		q.y=i->first.y;
		q.z=i->first.z;
		q.round=INFO->round+(t-1)/kProperty[CARGO].speed;
		if(usedPos.find(q)!=usedPos.end()){
			printf("[DEBUG] find pos used by fuel (%d,%d,%d)\n",q.x,q.y,q.z);
			continue;
		}
		if(t<dis){
			a=i->first;
			dis=t;
		}
	}
	return a;
}

Position FindWayHome(Position pos){
	Position a={0,0,-1};
	UsedPos q;
	int dis=INT_MAX;
	for(set<Position,PosComp>::iterator i=oceanNearMyBase.begin();i!=oceanNearMyBase.end();++i){
		int t=RouteDis(pos,*i);
		q.x=i->x;
		q.y=i->y;
		q.z=i->z;
		q.round=INFO->round+(t-1)/kProperty[CARGO].speed;
		if(usedPos.find(q)!=usedPos.end()){
			printf("[DEBUG] find home pos used (%d,%d,%d)\n",q.x,q.y,q.z);
			continue;
		}
		if(t<dis){
			a=*i;
			dis=t;
		}
	}
	printf("[DEBUG] finally decided (%d,%d,%d)\n",a.x,a.y,a.z);
	return a;
}

struct MovableUnit : public Unit
{
	bool movable(){return true;}
	virtual bool fireable(){return true;}
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
	bool fireable(){return false;}
	void DoJob(){
		if(!job.empty()){
			// [FIXME] crash when no oilfield / mine
			// [FIXME] some issue when resource point has low fuel / metal
			// [FIXME] crash when resource run out in midway
			// [FIXME] crash when mine is far away from base
			// [TODO] not best routine
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
				else{ // [FIXME] jump some process
					Position q=FindFuelPos(pos);
					int fn=RouteDis(pos,q)+20;
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
			for(map<UsedPos,int,UsedPosComp>::iterator i=usedPos.begin();i!=usedPos.end();++i){
				if(i->second==index)usedPos.erase(i);
			}
			UsedPos q;
			q.x=a.pos.x;
			q.y=a.pos.y;
			q.z=a.pos.z;
			q.round=INFO->round+(RouteDis(pos,a.pos)-1)/kProperty[type].speed;
			printf("[DEBUG] used pos (%d,%d,%d,%d)\n",q.x,q.y,q.z,q.round);
			usedPos[q]=index;
			++q.round;
			printf("[DEBUG] used pos (%d,%d,%d,%d)\n",q.x,q.y,q.z,q.round);
			usedPos[q]=index;
		}
	}
	void SimpleInit(){
		Job a;
		Position q=FindFuelPos(pos);
		UsedPos p;
		p.x=q.x;
		p.y=q.y;
		p.z=q.z;
		p.round=INFO->round+(RouteDis(pos,q)-1)/kProperty[CARGO].speed;
		printf("[DEBUG] used pos (%d,%d,%d,%d)\n",p.x,p.y,p.z,p.round);
		usedPos[p]=index;
		++p.round;
		printf("[DEBUG] used pos (%d,%d,%d,%d)\n",p.x,p.y,p.z,p.round);
		usedPos[p]=index;
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



void update(){
	TryUpdate();
	while(!usedPos.empty() && usedPos.begin()->first.round<INFO->round) usedPos.erase(usedPos.begin());
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
						a.z=SURFACE;
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
			t->second->UpdateState(*j);
		}
	}

	for(map<int,Unit*>::iterator i=Units.begin();i!=Units.end();++i){
		if(i->second->team==my_team && i->second->last_seen<INFO->round){
			delete i->second;
			Units.erase(i);
			i=Units.begin();
			RaiseGsig(UNIT_LOST);
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
	if(Distance(my_base->pos,enemy_base->pos)<=93){
		int j=my_base->fuel; // j: usable fuel
		for(int i=0;i<INFO->production_num;++i){
			j-=(kProperty[INFO->production_list[i].unit_type].fuel_max
				-((INFO->production_list[i].unit_type==FIGHTER ||
				INFO->production_list[i].unit_type==SCOUT)?1:0)-((INFO->production_list[i].unit_type==CARGO)?150:0));
		}
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
			while(j>=150 && c<min(3+INFO->round/5,oceanNearMyBase.size())){
				Produce(CARGO);
				++c;
				j-=150;
			}
		}
		for(int i=0;i<min(my_base->metal/kProperty[FIGHTER].cost,
			j/(kProperty[FIGHTER].fuel_max-1));++i){
				Produce(FIGHTER);
		}
		{									// if fighter_with_no_job>13 then attack a given pos
			unsigned fighter_with_no_job=0;
			for(map<int,Unit*>::iterator i=Units.begin();i!=Units.end();++i){
				Unit& obj=*(i->second);
				if(obj.type==FIGHTER && Distance(obj.pos,my_base->pos)<8){
					++fighter_with_no_job;
				}
			}
			int fighter_in_production_list=0;
			for(int i=0;i<INFO->production_num;++i){
				if(INFO->production_list[i].unit_type==FIGHTER && INFO->production_list[i].round_left<=0)
					++fighter_in_production_list;
			}
			if(fighter_with_no_job>=MAX_FIGHTER_WAITING  && !HasGSig(BASE_UNDER_ATTACK) && fighter_in_production_list>=4){
				for(map<int,Unit*>::iterator i=Units.begin();i!=Units.end();++i){
					Unit& obj=*(i->second);
					if(obj.type==FIGHTER && obj.job.empty()){
						Job a;
						a.type=a.ATTACK;
						a.target=enemy_base->index;
						a.pos=obj.pos+(enemy_base->pos-my_base->pos);
						obj.job.push(a);
					}
				}
			}
		}
		for(map<int,Unit*>::iterator i=Units.begin();i!=Units.end();++i){
			Unit& obj=*(i->second);
			if(obj.type==CARGO && obj.job.empty()){
				obj.SimpleInit();
			}
		}
	}
	else{
		int j=my_base->fuel; // j: usable fuel
		for(int i=0;i<INFO->production_num;++i){
			j-=(kProperty[INFO->production_list[i].unit_type].fuel_max
				-((INFO->production_list[i].unit_type==FIGHTER ||
				INFO->production_list[i].unit_type==SCOUT)?1:0)-((INFO->production_list[i].unit_type==CARGO)?150:0));
		}
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
			while(j>=150 && c<min(INFO->round<=1?6:12,oceanNearMyBase.size())){
				Produce(CARGO);
				++c;
				j-=150;
			}
		}
		int free_submarine=0;

		for(int i=0;i<min(my_base->metal/kProperty[SUBMARINE].cost,
			min(j/(kProperty[SUBMARINE].fuel_max),6));++i){
				Produce(FIGHTER);
		}
		{									// if fighter_with_no_job>13 then attack a given pos
			unsigned fighter_with_no_job=0;
			for(map<int,Unit*>::iterator i=Units.begin();i!=Units.end();++i){
				Unit& obj=*(i->second);
				if(obj.type==FIGHTER && obj.job.empty() && obj.last_seen==INFO->round){
					++fighter_with_no_job;
				}
			}
			int fighter_in_production_list=0;
			for(int i=0;i<INFO->production_num;++i){
				if(INFO->production_list[i].unit_type==FIGHTER && INFO->production_list[i].round_left<=1)
					++fighter_in_production_list;
			}
			if(fighter_with_no_job>=MAX_FIGHTER_WAITING  && !HasGSig(BASE_UNDER_ATTACK) && fighter_in_production_list>=4){
				for(map<int,Unit*>::iterator i=Units.begin();i!=Units.end();++i){
					Unit& obj=*(i->second);
					if(obj.type==FIGHTER && obj.job.empty()){
						Job a;
						a.type=a.ATTACK;
						a.target=enemy_base->index;
						a.pos=obj.pos+(enemy_base->pos-my_base->pos);
						obj.job.push(a);
					}
				}
			}
		}
		for(map<int,Unit*>::iterator i=Units.begin();i!=Units.end();++i){
			Unit& obj=*(i->second);
			if(obj.type==CARGO && obj.job.empty()){
				obj.SimpleInit();
			}
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


/*

处理伤害<0情形
预估敌方生命值
基地维修、补弹药
初始运输船向基地补给
初始运输船数量
资源空时不造运输船/运输船停至对方基地


临海的己方据点视为矿

*/

/*

先造运输船，6个，后期格子数
航母，6个，优先，外移
天气<=-3造侦察机（20回合）
飞机，4架，第10回合开始造
潜艇，12个，外移

航母尽量不要被据点打


*/