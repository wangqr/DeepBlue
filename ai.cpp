// AI for TeamStyle 16 by team 随意吧
// You may use it under the BSD 3-Clause License

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
#include<algorithm>

#define AI_VERSION "whatever_v2.1-b1"

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
using std::sort;
using std::min;
using std::max;
using std::min_element;

const GameInfo *INFO = Info();
Position FindMetalPos(Position pos);
Position FindFuelPos(Position pos);
Position FindWayHome(Position pos);
inline int Distance(Position pos1,Position pos2)
{
    return abs(pos1.x-pos2.x)+abs(pos1.y-pos2.y);
}
inline int DistanceToBase(Position base, Position pos)
{
    if(pos.x < base.x-1)
    {
        if(pos.y > base.y+1)
            return abs(pos.x - base.x+1) + abs(pos.y - base.y-1);
        else if(pos.y < base.y - 1)
            return abs(pos.x - base.x+1) + abs(pos.y - base.y + 1);
        else
            return abs(pos.x - base.x+1);
    }
    else if(pos.x > base.x +1)
    {
        if(pos.y > base.y+1)
            return abs(pos.x - base.x - 1) + abs(pos.y - base.y-1);
        else if(pos.y < base.y - 1)
            return abs(pos.x - base.x - 1) + abs(pos.y - base.y + 1);
        else
            return abs(pos.x - base.x - 1);
    }
    else
    {
        if(pos.y > base.y+1)
            return abs(pos.y - base.y-1);
        else if(pos.y < base.y -1)
            return abs(pos.y - base.y +1);
        else
            return 0;
    }
}
inline bool PxyEq(const Position &a,const Position &b)
{
    return a.x==b.x && a.y==b.y;
}
const char *GetTeamName()
{
    return AI_VERSION;
}
inline bool InSight (const State &a, const Position pos)
{
    return Distance(a.pos,pos)<=kProperty[a.type].sight_ranges[pos.z]+INFO->weather;
}
inline bool InSight (const Position pos)
{
    for(int i=0; i<INFO->element_num; ++i)
    {
        const State &a=*INFO->elements[i];
        if(a.team != INFO->team_num) continue;
        if(InSight(a,pos))return true;
    }
    return false;
}
inline bool InFireRange(const State &a, const Position pos)
{
    return Distance(a.pos,pos)<=kProperty[a.type].fire_ranges[pos.z];
}
inline bool InFireRange(const State &a, const State &b)
{
    if((kProperty[a.type].attacks[0]==0 || kProperty[b.type].defences[0]==-1) &&(kProperty[a.type].attacks[1]==0 || kProperty[b.type].defences[1]==-1)) return false;
    if(a.type!=BASE &&b.type!=BASE)
    {
        return InFireRange(a,b.pos);
    }
    else if(b.type!=BASE)
    {
        return DistanceToBase(a.pos,b.pos)<=kProperty[BASE].fire_ranges[b.pos.z];
    }
    else if(a.type!=BASE)
    {
        return DistanceToBase(b.pos,a.pos)<=kProperty[a.type].fire_ranges[b.pos.z];
    }
    else
    {
        return Distance(a.pos,b.pos)-4<=kProperty[a.type].fire_ranges[b.pos.z];
    }
}
inline bool InMap(int x,int y)
{
    return 0<=x && x<INFO->x_max && 0<=y && y<INFO->y_max;
}
inline bool InMap(const Position &pos)
{
    return InMap(pos.x,pos.y);
}
Position operator+ (const Position &a, const Position &b)
{
    Position temp;
    temp.x=a.x+b.x;
    temp.y=a.y+b.y;
    temp.z=-1;
    return temp;
}
Position operator- (const Position &a, const Position &b)
{
    Position temp;
    temp.x=a.x-b.x;
    temp.y=a.y-b.y;
    temp.z=-1;
    return temp;
}
struct PosComp
{
    bool operator() (const Position &a, const Position &b)
    {
        return a.x<b.x ||(a.x==b.x && (a.y<b.y || (a.y==b.y && a.z<b.z)));
    }
};
const Position neighbour[4]= {{1,0,-1},{-1,0,-1},{0,1,-1},{0,-1,-1}};
const Position baseNeighbour[12]= {{2,0,-1},{2,1,-1},{2,-1,-1},{-2,0,-1},{-2,1,-1},{-2,-1,-1},{0,2,-1},{1,2,-1},{-1,2,-1},{0,-2,-1},{1,-2,-1},{-1,-2,-1}};
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
    for(int i=0; i<INFO->x_max; ++i)
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
        for (int i=0; i<4; ++i)
        {
            int p=x+neighbour[i].x,q=y+neighbour[i].y;
            if(InMap(p,q) && vis[p][q]*z<0)
            {
                p=vis[p][q];
                for(int i=0; i<INFO->x_max; ++i)
                {
                    for(int j=0; j<INFO->y_max; ++j)
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
set<Position,PosComp> oceanNearEnemyBase;
int start=0;
struct UsedPos : public Position
{
    int round;
};
struct UsedPosComp
{
    bool operator() (const UsedPos &a, const UsedPos &b)
    {
        if(a.round<b.round)
            return true;
        if(a.round>b.round)
            return false;
        return a.x<b.x ||(a.x==b.x && (a.y<b.y || (a.y==b.y && a.z<b.z)));
    }
};
map<UsedPos,int,UsedPosComp> usedPos;
map<Position,int,PosComp> occupiedPos;

Position GetFreePlaceNearBase(const State &base, int dis_min, bool ocean,int z)
{
    for(int k=dis_min; k<INFO->x_max+INFO->y_max; ++k)
    {
        for(int x=0; x<INFO->x_max; ++x)
            for(int y=0; y<INFO->y_max; ++y)
            {
                Position a;
                a.x=x;
                a.y=y;
                a.z=z;
                if(ocean && MapInfo[x][y].map_type!=OCEAN) continue;
                if(DistanceToBase(base.pos,a)!=k)
                {
                    continue;
                }
                if(occupiedPos.find(a)!=occupiedPos.end())
                    continue;
                return a;
            }
    }
    Position a= {0,0,-1};
    return a;
}

Position GetFreePlaceCarrier(const State &base, int z)
{
    for(int k=kProperty[CARRIER].fire_ranges[kProperty[BASE].level]; k>0; --k)
    {
        for(int x=0; x<INFO->x_max; ++x)
            for(int y=0; y<INFO->y_max; ++y)
            {
                Position a;
                a.x=x;
                a.y=y;
                a.z=z;
                if(MapInfo[x][y].map_type!=OCEAN) continue;
                if(DistanceToBase(base.pos,a)!=k)
                {
                    continue;
                }
                if(occupiedPos.find(a)!=occupiedPos.end())
                    continue;
                return a;
            }
    }
    Position a= {0,0,-1};
    return a;
}


enum UnitSignal
{
    UNDER_ATTACK
};

inline int GetDamage(const State &source, const State &target)
{
    int modified_attacks[2];
    if(source.type!=CARRIER)
    {
        int distance=Distance(source.pos,target.pos);
        int fire_range = kProperty[source.type].fire_ranges[kProperty[target.type].level];
        modified_attacks[0] = int((1 - float(distance - fire_range / 2) / (fire_range + 1)) * kProperty[source.type].attacks[0]);
        modified_attacks[1] = int((1 - float(distance - fire_range / 2) / (fire_range + 1)) * kProperty[source.type].attacks[1]);
    }
    else
    {
        modified_attacks[0] = kProperty[source.type].attacks[0];
        modified_attacks[1] = kProperty[source.type].attacks[1];
    }
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
inline void RaiseGsig(GlobalSignal a)
{
    GSig.insert(a);
}
inline void EraseGsig(GlobalSignal a)
{
    GSig.erase(a);
}
inline bool HasGSig(const GlobalSignal &a)
{
    return GSig.find(a)!=GSig.end();
}


struct Unit : public State
{
    set<UnitSignal> sig;
    queue<Job> job;
    int last_command_send_round;
    int last_seen;
    bool did_action;
    virtual void sig_add(UnitSignal a)
    {
        sig.insert(a);
    }
    virtual void sig_remove(UnitSignal a)
    {
        sig.erase(a);
    }
    void DoJob();
    virtual bool movable()=0;
    virtual bool fireable()=0;
    virtual void InitState(const State &a)
    {
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
        last_seen=-1;
        UpdateState(a);
    }
    virtual void UpdateState(const State &a)
    {
        if(!a.visible) return;
        last_seen=INFO->round;
        if(movable())
        {
            pos=a.pos;
            if(health>a.health) sig_add(UNDER_ATTACK);
            health=a.health;
            if(team==INFO->team_num)
            {
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
        else if(type==BASE)
        {
            if(team==INFO->team_num)
            {
                if(health>a.health)
                {
                    sig_add(UNDER_ATTACK);
                    RaiseGsig(BASE_UNDER_ATTACK);
                }
                else
                {
                    sig_remove(UNDER_ATTACK);
                }
                health=a.health;
                fuel=a.fuel;
                metal=a.metal;
            }
            else
            {
                if(health>a.health)
                {
                    sig_add(UNDER_ATTACK);
                }
                else
                {
                    sig_remove(UNDER_ATTACK);
                }
                health=a.health;
            }
        }
        else if(type==OILFIELD)
        {
            fuel=a.fuel;
        }
        else if(type==MINE)
        {
            metal=a.metal;
        }
    }
    virtual void SimpleInit() {}
};
map<int,Unit *> Units;

inline bool HasSig(const Unit &a,const UnitSignal &b)
{
    return a.sig.find(b)!=a.sig.end();
}

struct Building : public Unit
{
    bool movable()
    {
        return false;
    }
    bool fireable()
    {
        return true;
    }
};

struct Base : public Building
{
    Base():Building()
    {
        Position n;
        n.z=-1;
        for(int i=0; i<kElementTypes; ++i)
        {
            rally_point[i]=n;
        }
    }
    Position rally_point[kElementTypes]; // [FIXME] no use
};
Base *my_base;
Base *enemy_base;

inline void EraseOccupiedPos(int index)
{
    map<Position,int,PosComp>::iterator i;
    for(i=occupiedPos.begin(); i!=occupiedPos.end(); ++i)
    {
        while(i!=occupiedPos.end() && i->second==index)
        {
            occupiedPos.erase(i);
            i=occupiedPos.begin();
        }
    }
}

struct CompHealth
{
    bool operator()(int a,int b)
    {
        if(a==enemy_base->index)
            return true;
        else if(b==enemy_base->index)
            return false;
        else if(Units[a]->health < Units[b]->health)
            return true;
        else
            return false;
    }
} comp_obj;

inline int NeedSupplyByBase(const State &a)
{
    if(a.type==CARGO)
        return 0;
    else if(a.type==FIGHTER || a.type==SCOUT)
    {
        if(DistanceToBase(my_base->pos,a.pos)==0)
        {
            return kProperty[a.type].ammo_max-a.ammo;
        }
    }
    else if(DistanceToBase(my_base->pos,a.pos)<=1)
    {
        return kProperty[a.type].ammo_max-a.ammo;
    }
    return 0;
}

void Unit::DoJob()
{
    if(!job.empty())
    {
        Job a=job.front();
        if(a.type==a.ATTACK)
        {
            if(!PxyEq(destination,a.pos))
            {
                ChangeDest(index,a.pos);
            }
            if(InFireRange(*this,*Units[a.target]))
            {
                printf("[DEBUG] InFireRange\n");
                //if(GetDamage(*this,*Units[a.target])>0){
                {
                    printf("[DEBUG] PerformAttack\n");
                    AttackUnit(index,a.target);
                    Units[a.target]->health-=GetDamage(*Units[index],*Units[a.target]);
                    last_command_send_round=INFO->round;
                }
            }
            if(Units[a.target]->health<=0 && Units[a.target]->last_seen<INFO->round)
            {
                job.pop();
            }
        }
        if(type==CARGO)
        {
            // [FIXME] crash when no oilfield / mine
            // [FIXME] some issue when resource point has low fuel / metal
            // [FIXME] crash when resource run out in midway
            // [FIXME] crash when mine is far away from base
            // [TODO] not best routine
            Job a=job.front();
            if(PxyEq(pos,a.pos))
            {
                if(a.type==a.COLLECT)
                {
                    if(Units[a.target]->type==OILFIELD)
                    {
                        if(my_base->metal<200 && !metalPos.empty())
                        {
                            a.pos=FindMetalPos(pos);
                            a.target=metalPos[a.pos];
                            job.pop();
                            job.push(a);
                        }
                        else
                        {
                            a.type=a.SUPPLY;
                            a.pos=FindWayHome(pos);
                            a.target=my_base->index;
                            job.pop();
                            job.push(a);
                        }
                    }
                    else if(Units[a.target]->type==MINE)
                    {
                        a.type=a.SUPPLY;
                        a.pos=FindWayHome(pos);
                        a.target=my_base->index;
                        job.pop();
                        job.push(a);
                    }
                }
                else if(!fuelPos.empty())  // [FIXME] jump some process
                {
                    Position q=FindFuelPos(pos);
                    int fn=RouteDis(pos,q)+20;
                    Supply(index,my_base->index,fuel-fn,0,metal);
                    a.type=a.COLLECT;
                    a.pos=q;
                    a.target=fuelPos[q];
                    job.pop();
                    job.push(a);
                }
                else if(fuel)
                {
                    while(!job.empty())job.pop();
                    a.type=a.ATTACK;
                    a.target=enemy_base->index;
                    a.pos=GetFreePlaceNearBase(*enemy_base,1,true,SURFACE);
                    EraseOccupiedPos(index);
                    occupiedPos[a.pos]=index;
                    job.push(a);
                }
                else
                {
                    EraseOccupiedPos(index);
                    occupiedPos[pos]=index;
                }
            }
            
            a=job.front();
            ChangeDest(index,a.pos);
            map<UsedPos,int,UsedPosComp>::iterator i;
            for(i=usedPos.begin(); i!=usedPos.end(); ++i)
            {
                while(i!=usedPos.end() && i->second==index)
                {
                    usedPos.erase(i);
                    i=usedPos.begin();
                }
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
    if(fireable() && last_command_send_round<INFO->round)
    {
        vector<int> enemy_in_sight;
        for(int i=0; i<INFO->element_num; ++i)
        {
            const State *j=INFO->elements[i];
            if(j->team==1-my_team && InFireRange(*this,*j))
            {
                enemy_in_sight.push_back(j->index);
            }
        }
        for(unsigned i=0; i<enemy_in_sight.size(); ++i)
        {
            if(Units[enemy_in_sight[i]]->health<=0)
            {
                enemy_in_sight.erase(enemy_in_sight.begin()+i);
                --i;
                continue;
            }
            int t=GetDamage(*this,*Units[enemy_in_sight[i]]);
            if(t<=0)
            {
                enemy_in_sight.erase(enemy_in_sight.begin()+i);
                --i;
                continue;
            }
        }
        if(!enemy_in_sight.empty())
        {
        
            int j=*min_element(enemy_in_sight.begin(),enemy_in_sight.end(),comp_obj);
            AttackUnit(index,j);
            Units[j]->health-=GetDamage(*this,*Units[j]);
            last_command_send_round=INFO->round;
        }
    }
    if(type==BASE && last_command_send_round<INFO->round)
    {
        int supply_max=0;
        int supply_index=-1;
        for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
        {
            Unit &obj=*(i->second);
            if(NeedSupplyByBase(obj)>supply_max)
            {
                supply_max=NeedSupplyByBase(obj);
                supply_index=i->first;
            }
        }
        if(supply_max)
        {
            Supply(my_base->index,supply_index,kProperty[Units[supply_index]->type].fuel_max-Units[supply_index]->fuel,kProperty[Units[supply_index]->type].ammo_max-Units[supply_index]->ammo,0);
        }
    }
}
struct Fort : public Building
{

};

struct Resources : public Unit
{
    bool movable()
    {
        return false;
    }
    bool fireable()
    {
        return false;
    }
    bool run_out;
    Resources()
    {
        run_out=false;
    }
};

struct Mine : public Resources
{
    void UpdateState(const State &a)
    {
        Resources::UpdateState(a);
        if(metal==0) run_out=true;
        for(int i=1; i<4; ++i)
        {
            Position a=pos+neighbour[i];
            a.z=SURFACE;
            if(InMap(a) && MapInfo[a.x][a.y].map_type==OCEAN)
            {
                if(run_out)
                {
                    metalPos.erase(a);
                }
                else
                {
                    metalPos[a]=index;
                }
            }
        }
    }
};

struct OilField : public Resources
{
    void UpdateState(const State &a)
    {
        Resources::UpdateState(a);
        if(fuel==0) run_out=true;
        for(int i=1; i<4; ++i)
        {
            Position a=pos+neighbour[i];
            a.z=SURFACE;
            if(MapInfo[a.x][a.y].map_type==OCEAN)
            {
                if(run_out)
                {
                    fuelPos.erase(a);
                }
                else
                {
                    fuelPos[a]=index;
                }
            }
        }
    }
};

Position FindMetalPos(Position pos)
{
    Position a= {0,0,-1};
    UsedPos q;
    int dis=INT_MAX;
    for(map<Position,int,PosComp>::iterator i=metalPos.begin(); i!=metalPos.end(); ++i)
    {
        int t=RouteDis(pos,i->first);
        q.x=i->first.x;
        q.y=i->first.y;
        q.z=i->first.z;
        q.round=INFO->round+(t-1)/kProperty[CARGO].speed;
        if(usedPos.find(q)!=usedPos.end())
        {
            printf("[DEBUG] find pos used by metal (%d,%d,%d)\n",q.x,q.y,q.z);
            continue;
        }
        if(t<dis)
        {
            a=i->first;
            dis=t;
        }
    }
    return a;
}

Position FindFuelPos(Position pos)
{
    Position a= {0,0,-1};
    UsedPos q;
    int dis=INT_MAX;
    for(map<Position,int,PosComp>::iterator i=fuelPos.begin(); i!=fuelPos.end(); ++i)
    {
        int t=RouteDis(pos,i->first);
        q.x=i->first.x;
        q.y=i->first.y;
        q.z=i->first.z;
        q.round=INFO->round+(t-1)/kProperty[CARGO].speed;
        if(usedPos.find(q)!=usedPos.end())
        {
            printf("[DEBUG] find pos used by fuel (%d,%d,%d)\n",q.x,q.y,q.z);
            continue;
        }
        if(t<dis)
        {
            a=i->first;
            dis=t;
        }
    }
    return a;
}

Position FindWayHome(Position pos)
{
    Position a= {0,0,-1};
    UsedPos q;
    int dis=INT_MAX;
    for(set<Position,PosComp>::iterator i=oceanNearMyBase.begin(); i!=oceanNearMyBase.end(); ++i)
    {
        int t=RouteDis(pos,*i);
        q.x=i->x;
        q.y=i->y;
        q.z=i->z;
        q.round=INFO->round+(t-1)/kProperty[CARGO].speed;
        if(usedPos.find(q)!=usedPos.end())
        {
            printf("[DEBUG] find home pos used (%d,%d,%d)\n",q.x,q.y,q.z);
            continue;
        }
        if(t<dis)
        {
            a=*i;
            dis=t;
        }
    }
    printf("[DEBUG] finally decided (%d,%d,%d)\n",a.x,a.y,a.z);
    return a;
}

struct MovableUnit : public Unit
{
    bool movable()
    {
        return true;
    }
    virtual bool fireable()
    {
        return true;
    }
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
    bool fireable()
    {
        return false;
    }
    void SimpleInit()
    {
        Job a;
        a.pos=pos;
        a.type=a.SUPPLY;
        a.target=my_base->index;
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



void update()
{
    TryUpdate();
    while(!usedPos.empty() && usedPos.begin()->first.round<INFO->round) usedPos.erase(usedPos.begin());
    GSig.clear();
    if(start==0)  // Run once
    {
        start=1;
        my_team=INFO->team_num;
        MapInfo.resize(INFO->x_max);
        for(int i=0; i<INFO->x_max; ++i)
        {
            MapInfo[i].resize(INFO->y_max);
            for(int j=0; j<INFO->y_max; ++j)
            {
                MapInfo[i][j].map_type=Map(i,j);
            }
        }
    }
    
    for(int i=0; i<INFO->element_num; ++i)
    {
        const State *j=INFO->elements[i];
        map<int,Unit *>::iterator t=Units.find(j->index);
        if(t==Units.end())
        {
            if(j->type==BASE)
            {
                if(j->team==INFO->team_num)
                {
                    my_base=new Base;
                    Units[j->index]=my_base;
                    my_base->InitState(*j);
                    for(int i=0; i<12; ++i)
                    {
                        Position a=baseNeighbour[i]+my_base->pos;
                        a.z=SURFACE;
                        if(InMap(a) && MapInfo[a.x][a.y].map_type==OCEAN)
                            oceanNearMyBase.insert(a);
                    }
                }
                else
                {
                    enemy_base=new Base;
                    Units[j->index]=enemy_base;
                    enemy_base->InitState(*j);
                    for(int i=0; i<12; ++i)
                    {
                        Position a=baseNeighbour[i]+enemy_base->pos;
                        a.z=SURFACE;
                        if(InMap(a) && MapInfo[a.x][a.y].map_type==OCEAN)
                            oceanNearEnemyBase.insert(a);
                    }
                }
            }
            else if(j->type==FORT)
            {
                Units[j->index]=new Fort;
                Units[j->index]->InitState(*j);
            }
            else if(j->type==MINE)
            {
                Units[j->index]=new Mine;
                Units[j->index]->InitState(*j);
            }
            else if(j->type==OILFIELD)
            {
                Units[j->index]=new OilField;
                Units[j->index]->InitState(*j);
            }
            else if(j->type==SUBMARINE)
            {
                Units[j->index]=new Submarine;
                Units[j->index]->InitState(*j);
            }
            else if(j->type==DESTROYER)
            {
                Units[j->index]=new Destroyer;
                Units[j->index]->InitState(*j);
            }
            else if(j->type==CARRIER)
            {
                Units[j->index]=new Carrier;
                Units[j->index]->InitState(*j);
            }
            else if(j->type==CARGO)
            {
                Units[j->index]=new Cargo;
                Units[j->index]->InitState(*j);
            }
            else if(j->type==FIGHTER)
            {
                Units[j->index]=new Fighter;
                Units[j->index]->InitState(*j);
            }
            else if(j->type==SCOUT)
            {
                Units[j->index]=new Scout;
                Units[j->index]->InitState(*j);
            }
        }
        else
        {
            t->second->UpdateState(*j);
        }
    }
    
    for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
    {
        if(i->second->team==my_team && i->second->last_seen<INFO->round)
        {
            EraseOccupiedPos(i->second->index);
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
    if(Distance(my_base->pos,enemy_base->pos)<=93)
    {
        int j=my_base->fuel; // j: usable fuel
        for(int i=0; i<INFO->production_num; ++i)
        {
            j-=(kProperty[INFO->production_list[i].unit_type].fuel_max
                -((INFO->production_list[i].unit_type==FIGHTER ||
                   INFO->production_list[i].unit_type==SCOUT)?1:0)-((INFO->production_list[i].unit_type==CARGO)?150:0));
        }
        {
            unsigned c=0;
            for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
            {
                if(i->second->team==my_team && i->second->type==CARGO)
                    ++c;
            }
            for(int i=0; i<INFO->production_num; ++i)
            {
                if(INFO->production_list[i].unit_type==CARGO)
                    ++c;
            }
            while(j>=150 && c<min(unsigned(3+INFO->round/5),unsigned(oceanNearMyBase.size())) && (!fuelPos.empty()))
            {
                Produce(CARGO);
                ++c;
                j-=150;
            }
            if(c<min(unsigned(3+INFO->round/5),unsigned(oceanNearMyBase.size())) && !fuelPos.empty())
                j=0;
        }
        
        for(int i=0; i<min(my_base->metal/kProperty[FIGHTER].cost,
                           j/(kProperty[FIGHTER].fuel_max-1)); ++i)
        {
            Produce(FIGHTER);
        }
        {
            // if fighter_with_no_job>13 then attack a given pos
            unsigned fighter_with_no_job=0;
            for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
            {
                Unit &obj=*(i->second);
                if(obj.type==FIGHTER && obj.job.empty())
                {
                    ++fighter_with_no_job;
                }
            }
            unsigned fighter_in_production_list=0;
            for(int i=0; i<INFO->production_num; ++i)
            {
                if(INFO->production_list[i].unit_type==FIGHTER && INFO->production_list[i].round_left<=0)
                {
                    ++fighter_in_production_list;
                }
            }
            if(fuelPos.empty()||((fighter_with_no_job>=9)  && !HasGSig(BASE_UNDER_ATTACK) && (fighter_in_production_list>=4)))
            {
                printf("[DEBUG] fighter_with_no_job %d\n",fighter_with_no_job);
                printf("[DEBUG] fighter_in_production_list %d\n",fighter_in_production_list);
                for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
                {
                    Unit &obj=*(i->second);
                    if(obj.type==FIGHTER && obj.job.empty())
                    {
                        Job a;
                        a.type=a.ATTACK;
                        a.target=enemy_base->index;
                        a.pos=obj.pos+(enemy_base->pos-my_base->pos);
                        obj.job.push(a);
                    }
                }
            }
        }
        for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
        {
            Unit &obj=*(i->second);
            if(obj.team==my_team && obj.type==CARGO && obj.job.empty())
            {
                obj.SimpleInit();
            }
        }
    }
    else
    {
        int j=my_base->fuel; // j: usable fuel
        for(int i=0; i<INFO->production_num; ++i)
        {
            j-=(kProperty[INFO->production_list[i].unit_type].fuel_max
                -((INFO->production_list[i].unit_type==FIGHTER ||
                   INFO->production_list[i].unit_type==SCOUT)?1:0)-((INFO->production_list[i].unit_type==CARGO)?150:0));
        }
        {
            unsigned c=0;
            for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
            {
                if(i->second->team==my_team && i->second->type==CARGO)
                    ++c;
            }
            for(int i=0; i<INFO->production_num; ++i)
            {
                if(INFO->production_list[i].unit_type==CARGO)
                    ++c;
            }
            while(j>=150 && c<min(INFO->round<=1?6u:12u,unsigned(oceanNearMyBase.size())))
            {
                Produce(CARGO);
                ++c;
                j-=150;
            }
        }
        if(j<150)j=0;
        {
            unsigned c=0;
            for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
            {
                if(i->second->team==my_team && i->second->type==SCOUT)
                    ++c;
            }
            for(int i=0; i<INFO->production_num; ++i)
            {
                if(INFO->production_list[i].unit_type==SCOUT)
                    ++c;
            }
            while(j>=120 && c<((INFO->weather<-2 && INFO->round>=20)?1:0))
            {
                Produce(SCOUT);
                ++c;
                j-=120;
            }
            if(INFO->weather<-2 && INFO->round>=20 && c==0)
            {
                j=0;
            }
        }
        {
            unsigned c=0;
            for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
            {
                if(i->second->team==my_team && i->second->type==FIGHTER)
                    ++c;
            }
            for(int i=0; i<INFO->production_num; ++i)
            {
                if(INFO->production_list[i].unit_type==FIGHTER)
                    ++c;
            }
            while(j>=100 && c<((INFO->round>=10)?4:0))
            {
                Produce(FIGHTER);
                ++c;
                j-=100;
            }
            if(INFO->round>=10 && c<4)
            {
                j=0;
            }
        }
        {
            unsigned c=0;
            for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
            {
                if(i->second->team==my_team && i->second->type==CARRIER && i->second->job.empty())
                    ++c;
            }
            for(int i=0; i<INFO->production_num; ++i)
            {
                if(INFO->production_list[i].unit_type==CARRIER)
                    ++c;
            }
            while(j>=200 && c<6 && !fuelPos.empty())
            {
                Produce(CARRIER);
                ++c;
                j-=200;
            }
            if(c<6 && !fuelPos.empty())
            {
                j=0;
            }
        }
        if(Distance(my_base->pos,enemy_base->pos)<=115)
        {
            unsigned c=0;
            for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
            {
                if(i->second->team==my_team && i->second->type==SUBMARINE && i->second->job.empty())
                    ++c;
            }
            for(int i=0; i<INFO->production_num; ++i)
            {
                if(INFO->production_list[i].unit_type==SUBMARINE)
                    ++c;
            }
            while(j>=120 && c<12)
            {
                Produce(SUBMARINE);
                ++c;
                j-=120;
            }
            if(c<12)
            {
                j=0;
            }
        }
        unsigned c=0;
        for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
        {
            if(i->second->team==my_team && i->second->type==CARRIER && i->second->job.empty())
                ++c;
        }
        if(j && c>=6)
        {
            for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
            {
                Unit &obj=*(i->second);
                if(obj.type==SUBMARINE && obj.job.empty())
                {
                    Job a;
                    a.type=a.ATTACK;
                    a.target=enemy_base->index;
                    a.pos=GetFreePlaceNearBase(*enemy_base,1,true,SURFACE);
                    EraseOccupiedPos(obj.index);
                    occupiedPos[a.pos]=obj.index;
                    obj.job.push(a);
                }
                if(obj.type==CARRIER && obj.job.empty())
                {
                    Job a;
                    a.type=a.ATTACK;
                    a.target=enemy_base->index;
                    a.pos=GetFreePlaceCarrier(*enemy_base,SURFACE);
                    EraseOccupiedPos(obj.index);
                    occupiedPos[a.pos]=obj.index;
                    obj.job.push(a);
                }
            }
        }
        else
        {
            for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
            {
                Unit &obj=*(i->second);
                if(obj.type==SUBMARINE && DistanceToBase(my_base->pos,obj.pos)==1)
                {
                    Position pos=GetFreePlaceNearBase(*my_base,2,true,UNDERWATER);
                    EraseOccupiedPos(obj.index);
                    occupiedPos[pos]=obj.index;
                    ChangeDest(obj.index,pos);
                }
                if(obj.type==CARRIER && DistanceToBase(my_base->pos,obj.pos)==1)
                {
                    Position pos=GetFreePlaceNearBase(*my_base,2,true,SURFACE);
                    EraseOccupiedPos(obj.index);
                    occupiedPos[pos]=obj.index;
                    ChangeDest(obj.index,pos);
                }
            }
        }
        for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
        {
            Unit &obj=*(i->second);
            if(obj.team==my_team && obj.type==CARGO && obj.job.empty())
            {
                obj.SimpleInit();
            }
        }
        for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
        {
            Unit &obj=*(i->second);
            if(obj.team==1-my_team && InFireRange(obj,*my_base))
            {
                for(map<int,Unit *>::iterator j=Units.begin(); j!=Units.end(); ++j)
                {
                    Unit &t=*(i->second);
                    if(t.team==my_team && t.type==FIGHTER && t.job.empty() && !InFireRange(t,obj))
                    {
                        Job a;
                        a.type=a.ATTACK;
                        a.pos=obj.pos;
                        a.target=obj.index;
                        t.job.push(a);
                    }
                }
            }
        }
    }
    //
    //
    // ==========[MAIN AI PART END]==========
    //
    //
    
    for(map<int,Unit *>::iterator i=Units.begin(); i!=Units.end(); ++i)
    {
        if(i->second->team==my_team)
            i->second->DoJob();
    }
}


/*

运输船还是会卡住
临海的己方据点视为矿

优先打掉敌方基地附近的据点

*/

/*

航母尽量不要被据点打

*/