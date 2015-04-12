#include "basic.h"
#include<iostream>
#include<cmath>
#include<vector>
#include<vector>
#include<stack>
#include<queue>
#include<map>
#include<set>
#include<time.h>

#define AI_VERSOIN "wangqr_0.0.1.1002"

// Remove this line if you konw C++, and don't want a dirty namespace
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

#define INF 1000
const char * GetTeamName()
{
    return AI_VERSOIN;  // Name of your team
}

enum CommandType
{
	//Produce
	PRODUCE,
	//Suppply
	SUPPLYFORT,
	SUPPLYBASE,
	SUPPLYUNIT,
	//Attack
	ATTACKUNIT,
	ATTACKFORT,
	ATTACKBASE,
	//Fix
    FIX,
	//ChangeDest
	FORWARD,
	CARGOSUPPLY,
	RETURNBASE,
	//Cancel
	CANCEL,
    kCommandTypes
};

void init();//��ʼ������������ĺ���
void enemy_init();//����Info()��ʼ����ʱ�ĵ���
void Supply_Repair(int i);/////�ò���ȼ�ϻ��ߵ�ҩ��ʱ�򣬻���Ѫ��̫����Ҫ�ػ���ά��
Position minus(Position pos1,Position pos2,int fire_range);//����pos1��pos2ǡ�����fire_range��һ��λ��
int if_in(int i,int *a,int len);//�ж�ĳ�����Ƿ���������
int GetBase(int team);//��ü�����Է����ص�����
int distance(Position pos1, Position pos2);//����֮��ľ���
int DisToBase(Position pos1, Position pos2);  ////���ص����صľ��룬 pos2��ʾ�������Ͻ�
int if_alive(int operand);//�ж�ĳ��λ�Ƿ��Դ��
int max(int x, int y);//���ֵ
int min(int x, int y);//��Сֵ
int GetNear(Position pos, ElementType type);//��þ��λ�������ĳ���͵�λ������
int if_command(int i,CommandType type,ElementType target = kElementTypes);//�ж϶Ըó�Ա�ĸ������Ƿ�����´����Ѿ��´���߼���ָ���򲻿ɣ�
void Cargo_Supply(int index);//���ڻغϿ�ʼʱ���Ϊ1�ĵ�λ���в���
void MoveCargo(int i);//���ڳ��������Ȼû����������䴬��ȥ��Դ���ռ������߻ػ��ز��������ߴ���ǰ��
void Carrier_Supply(int i);//��ĸ����ߵ�λ�Ĳ���
void Attack(int index);//Ѱ������ڵĻ��ػ�ݵ㣬��ѡ��Ѫ�����ٵĵ�λ����,�������˶��������ǡ����̵ĵط�
void BaseAct(int index);//���ص�ά�ޣ�����
void BaseProduce(int index);///���ص�����
void Forward(int index);////Ѱ������ĵо�ǰ������������䴬
void DefineUnit();
Position findOcean(Position pos);//�ҵ�pos����Ĳ��ٴ�½�ĵط�
int AwayToLand(Position pos);//�ж�ĳ��λ���ǲ����ٴ�½

const GameInfo *INFO = Info();
int enemy_num;
State *enemy_element;
int (*command)[2];  //���غϷ�����ָ�һ��Ч��ָ�һ���ƶ�ָ��
double parameter[4];

int BaseMineCargo = -2, BaseFuelCargo = -2;  //�����������ص�     ��ʼ��Ϊ-2��-1��ʾ������
int base_defender[3] = {-2, -2, -2};      //���ػ��ص����𽢣���ʼ��Ϊ -2��-1��ʾ������

vector<int>Enemy_Indexes;

int ProduceState = -1;
int DISTANCE_NEAR_BASE = 20;
int BEGINGAME = 0;
Position BaseDefendPos;//�����������ص����𽢵�λ��
//Position PassWay;  //˫�����ؼ����Ҫͨ��

void AIMain()
{  
	TryUpdate();
	//�غϿ�ʼʱ�ĳ�ʼ��
	init();
	DefineUnit();
	for(int i=0;i<INFO->element_num;i++)
		if(INFO->elements[i]->team == INFO->team_num)
		{
			const State *Element = INFO->elements[i];
			Supply_Repair(i);//�ò�����ά�޾�ȥ������ά��
	
			//TryUpdate();
			//enemy_init();
			if(Element->type != BASE)
				Attack(i);        //����

			if(Element->type == BASE)
				BaseAct(i);
			else if(Element->type == CARGO)
			{
				Cargo_Supply(i);   //���佢����
				MoveCargo(i);      //���佢�˶�
			}	
			else if(Element->type == CARRIER)
			{
				Carrier_Supply(i);
			}
			else ;

			Forward(i);       //ǰ��
		}
	//Update();
	delete []enemy_element;
}

//��ʼ������������ĺ���
void init()
{
	if(BEGINGAME == 0)
	{
		parameter[0] = 0.3; //Ѫ��
		parameter[1] = 0.3; //����
		parameter[2] = 0.3; //ȼ��
		parameter[3] = 0.4; //��ҩ
	
		const State *Base = INFO->elements[GetBase(INFO->team_num)];
		const State *EnemyBase = INFO->elements[GetBase(1-INFO->team_num)];
		BaseDefendPos.x = Base->pos.x+5*(Base->pos.x < EnemyBase->pos.x?1:-1);
		BaseDefendPos.y = Base->pos.y+5*(Base->pos.y < INFO->y_max/2?1:-1);
		BaseDefendPos.z = 1;
		BaseDefendPos = findOcean(BaseDefendPos);

		//PassWay.x = (Base->pos.x + EnemyBase->pos.x)/2;
		//PassWay.y = (Base->pos.y + EnemyBase->pos.y)/2;
		//PassWay.z = 1;

		BEGINGAME = 1;
	}
	/*if(BEGINGAME == 1)
		BEGINGAME == 2;*/
	command = new int[INFO->element_num][2];    //�����б��ʼ��
	for(int i = 0; i<INFO->element_num; i++)
		for(int j=0;j<2;j++)
			command[i][j] = -1; 
	enemy_init();
}

//����Info()��ʼ����ʱ�ĵ���
void enemy_init()
{
	//�з���λ���������з���λ�������ݵ㣬�������󳡺�����
	enemy_num =0;
	for(int i=0;i<Info()->element_num;i++)
		if(Info()->elements[i]->team != Info()->team_num  
			&& Info()->elements[i]->type != MINE && Info()->elements[i]->type != OILFIELD)
		{
			enemy_num += 1;
			Enemy_Indexes.push_back(i);
		}	
	enemy_element = new State[enemy_num];
	for(int i=0; i<enemy_num; i++)
		{
			enemy_element[i] = *(Info()->elements[Enemy_Indexes[i]]);
		}
	Enemy_Indexes.clear();
}

//�ҵ�pos����Ĳ��ٴ�½�ĵط�
Position findOcean(Position pos)
{
	Position sea;
	if(AwayToLand(pos))
		return pos;
	for(int i=0; i<9; i++)
	{
		sea.x = i%3;
		sea.y = i/3;
		if(AwayToLand(sea))
			return sea;
	}
	return pos;
}

//�ж�ĳ��λ���ǲ����ٴ�½
int AwayToLand(Position pos)
{
	return 1-Map(pos.x-1, pos.y-1)||Map(pos.x-1, pos.y)||Map(pos.x-1, pos.y+1)||Map(pos.x, pos.y-1)
			||Map(pos.x, pos.y)||Map(pos.x, pos.y+1)||Map(pos.x+1, pos.y-1)||Map(pos.x+1, pos.y)||Map(pos.x+1, pos.y);
}
/////�ò���ȼ�ϻ��ߵ�ҩ��ʱ�򣬻���Ѫ��̫����Ҫ�ػ���ά��
void Supply_Repair(int i)
{
	const State *Element = INFO->elements[i];
	if(Element->health < parameter[0] * kProperty[Element->type].health_max
		&& Element->health != -1
		&& if_command(i, RETURNBASE))
	{
		ChangeDest(Element->index, INFO->elements[GetBase(INFO->team_num)]->pos);
		command[i][1] = RETURNBASE;
		return;
	}
	else if(Element->type == BASE)
	{
		if(Element->metal < 0.4*kProperty[BASE].metal_max && if_alive(BaseMineCargo))
		{
			if(GetState(BaseMineCargo)->metal > (int)(0.4 * kProperty[CARGO].metal_max))
				ChangeDest(BaseMineCargo, Element->pos);
			else
			{
				/*int mine = GetNear(Element->pos, MINE);
				if(mine < 0)
					return;*/
				ChangeDest(BaseMineCargo, INFO->elements[GetNear(Element->pos, MINE)]->pos);
			}
			for(int index=0; index<INFO->element_num; index++)
				if(INFO->elements[index]->index == BaseMineCargo)
				{
					command[index][1] = RETURNBASE;
					return;
				}
		}
		if(Element->fuel < 0.4*kProperty[BASE].fuel_max && if_alive(BaseFuelCargo))
		{
			if(GetState(BaseFuelCargo)->fuel > 0.5*kProperty[CARGO].fuel_max)
				ChangeDest(BaseFuelCargo, Element->pos);
			else
			{
				/*int fuel = GetNear(Element->pos, OILFIELD);
				if(fuel <= 0)
					return;*/
				ChangeDest(BaseFuelCargo, INFO->elements[GetNear(Element->pos, OILFIELD)]->pos);
			}
			for(int index=0; index<INFO->element_num; index++)
				if(INFO->elements[index]->index == BaseFuelCargo)
				{
					command[index][1] = RETURNBASE;
					return;
				}
		}
	}
	else if(Element->type == CARRIER && ((Element->ammo <  parameter[1]*kProperty[Element->type].ammo_max && Element->ammo != -1)
			|| (Element->fuel < distance(Element->pos, INFO->elements[GetBase(INFO->team_num)]->pos) + 9)))
	{
		//�ػ���
		int base_index = GetBase(INFO->team_num);
		if(DisToBase(Element->pos, INFO->elements[base_index]->pos) < DISTANCE_NEAR_BASE)
			if(if_command(i, RETURNBASE))
			{
				ChangeDest(INFO->elements[i]->index, INFO->elements[base_index]->pos);
				command[i][1] = RETURNBASE;	
				return;
			}

		int cargo_index = GetNear(Element->pos, CARGO);
		if(cargo_index >= 0  && cargo_index != BaseMineCargo && cargo_index != BaseFuelCargo)
		{
			if(if_command(cargo_index, CARGOSUPPLY))
			{
				ChangeDest(INFO->elements[cargo_index]->index, Element->pos);
				command[cargo_index][1] = CARGOSUPPLY;	
				return;
			}
			else
			{
				ChangeDest(Element->index, INFO->elements[cargo_index]->pos);
				command[i][1] = CARGOSUPPLY;
				return;
			}
		}
	}
	else if(Element->type > FORT 
			&& ((Element->ammo <  parameter[1]*kProperty[Element->type].ammo_max && Element->ammo != -1)
			|| (Element->fuel < distance(Element->pos, INFO->elements[GetBase(INFO->team_num)]->pos) + 9))
			&& Element->type != CARGO)        //ȼ�ϲ���
	{
		//�ػ���
		int base_index = GetBase(INFO->team_num);
		if(DisToBase(Element->pos, INFO->elements[base_index]->pos) < DISTANCE_NEAR_BASE)
			if(if_command(i, RETURNBASE))
			{
				ChangeDest(INFO->elements[i]->index, INFO->elements[base_index]->pos);
				command[i][1] = RETURNBASE;	
				return;
			}

		int cargo_index = GetNear(Element->pos, CARGO);
		int carrier_index = GetNear(Element->pos, CARRIER);
		int index;

		if(cargo_index <= 0 && carrier_index <= 0)return; 
		else if(carrier_index <= 0) index = cargo_index;
		else if(cargo_index <= 0)index = carrier_index;
		else 
			index = distance(Element->pos, INFO->elements[cargo_index]->pos) > distance(Element->pos, INFO->elements[carrier_index]->pos) ? carrier_index : cargo_index;

		if(distance(Element->pos, INFO->elements[cargo_index]->pos) > distance(Element->pos, INFO->elements[index]->pos)
			&& (INFO->elements[index]->ammo > kProperty[Element->type].ammo_max - Element->ammo
				|| INFO->elements[index]->fuel > 0.7 * kProperty[Element->type].fuel_max - Element->fuel)
			&& index != BaseMineCargo && index != BaseFuelCargo)
	{
			if(if_command(cargo_index, CARGOSUPPLY))
			{
				ChangeDest(INFO->elements[index]->index, Element->pos);
				command[index][1] = CARGOSUPPLY;	
				return;
			}
			else
			{
				ChangeDest(Element->index, INFO->elements[index]->pos);
				command[i][1] = CARGOSUPPLY;
				return;
			}
		}
	}
	//else if(Element->type > FORT 
	//		&& Element->ammo <  parameter[1]*kProperty[Element->type].ammo_max
	//		&& Element->ammo != -1)
	//{
	//
	//	int cargo_index = GetNear(Element->pos, CARGO);
	//	if(if_command(i, CARGOSUPPLY)
	//		&& DisToBase(Element->pos, INFO->elements[GetBase(1 - INFO->team_num)]->pos) < kProperty[BASE].fire_ranges[kProperty[Element->type].level])
	//	{
	//		Position Pos =  minus(Element->pos, INFO->elements[GetBase(1 - INFO->team_num)]->pos, 
	//							kProperty[BASE].fire_ranges[kProperty[Element->type].level]+1);
	//		ChangeDest(Element->index, Pos);
	//		command[i][1] = CARGOSUPPLY;
	//		if(if_command(cargo_index, CARGOSUPPLY))
	//		{
	//			ChangeDest(INFO->elements[cargo_index]->index, Pos);
	//			command[cargo_index][1] = CARGOSUPPLY;	
	//			return;
	//		}
	//	}

	//	if(cargo_index >= 0  && cargo_index != BaseMineCargo && cargo_index != BaseFuelCargo)
	//	{
	//		if(if_command(cargo_index, CARGOSUPPLY))
	//		{
	//			ChangeDest(INFO->elements[cargo_index]->index, Element->pos);
	//			command[cargo_index][1] = CARGOSUPPLY;	
	//			return;
	//		}
	//		else
	//		{
	//			ChangeDest(Element->index, INFO->elements[cargo_index]->pos);
	//			command[i][1] = CARGOSUPPLY;
	//			return;
	//		}
	//	}
	//}
	else;
}

//����pos1��pos2ǡ�����fire_range��һ��λ��
Position minus(Position pos1,Position pos2,int fire_range)
{
	Position target;
	target.x = pos1.x > pos2.x ? pos1.x+(fire_range - distance(pos1,pos2))/2 : pos1.x -(fire_range - distance(pos1,pos2))/2;
	target.y = pos1.y > pos2.y ? pos1.y+((fire_range - distance(pos1,pos2))+1)/2 : pos1.y -((fire_range - distance(pos1,pos2))+1)/2;
	target.z = pos1.z;
	return target;

}

//�ж�ĳ�����Ƿ���������
int if_in(int i,int *a,int len)
{
	for(int j=0;j<len;j++)
		if(i==a[j])
			return 1;
	return 0;
}

//��ü�����Է����ص�����
int GetBase(int team)
{
	int i;
	for(i=0; i<INFO->element_num; i++)
		if(INFO->elements[i]->type == BASE && INFO->elements[i]->team == team)
			return i; 
	return -1;
}

//����֮��ľ���
int distance(Position pos1, Position pos2)
{
	return abs(pos1.x - pos2.x) + abs(pos1.y - pos2.y);
}

//���ص����صľ���
int DisToBase(Position pos1, Position pos2)  //pos2��ʾ�������Ͻ�
{
	int min = 1000;
	if(pos1.x < pos2.x)//��Ϊ�ſ�
	{
		if(pos1.y > pos2.y)
			return abs(pos1.x - pos2.x) + abs(pos1.y - pos2.y);
		else if(pos1.y < pos2.y - 2)
			return abs(pos1.x - pos2.x) + abs(pos1.y - pos2.y + 2);
		else 
			return abs(pos1.x - pos2.x);
	}
	else if(pos1.x > pos2.x +2)
	{
		if(pos1.y > pos2.y)
			return abs(pos1.x - pos2.x - 2) + abs(pos1.y - pos2.y);
		else if(pos1.y < pos2.y - 2)
			return abs(pos1.x - pos2.x - 2) + abs(pos1.y - pos2.y + 2);
		else 
			return abs(pos1.x - pos2.x - 2);
	}
	else
	{
		if(pos1.y > pos2.y)
			return abs(pos1.y - pos2.y);
		else if(pos1.y < pos2.y -2)
			return abs(pos1.y - pos2.y +2);
		else 
			return 0;
	}
}

//�ж�ĳ��λ�Ƿ��Դ��
int if_alive(int operand)
{
	for(int i=0;i<INFO->element_num;i++)
		if(INFO->elements[i]->index == operand)
			return 1;
	return 0;
}

//���ֵ
int max(int x, int y)
{
	return x>y?x:y;
}

//��Сֵ
int min(int x, int y)
{
	return x<y?x:y;
}

//��þ��λ�������ĳ���͵�λ������
int GetNear(Position pos, ElementType type)
{
	int i,near_index = -1,min_distance = 1000;
	for(i=0;i<INFO->element_num;i++)
	{
		const State *target = INFO->elements[i];
		if(target->type == type &&  distance(target->pos, pos)<min_distance)
		{
			if( type == OILFIELD && 
				distance(target->pos, INFO->elements[GetBase(1 - INFO->team_num)]->pos) > kProperty[BASE].sight_ranges[SURFACE]
				&& (target->fuel > 0 || (target->fuel == 0 
					&& distance(target->pos, INFO->elements[GetBase(INFO->team_num)]->pos) > 2 * kProperty[BASE].sight_ranges[SURFACE]))) //���ﻹ��ʯ��
			{
				near_index = i;
				min_distance = distance(target->pos, pos);
			}
			else if(type == MINE  && 
				distance(target->pos, INFO->elements[GetBase(1 - INFO->team_num)]->pos) > kProperty[BASE].sight_ranges[SURFACE]
				&& (target->metal > 0|| (target->metal == 0
					&& distance(target->pos, INFO->elements[GetBase(INFO->team_num)]->pos) > 2 * kProperty[BASE].sight_ranges[SURFACE])))//��֤�󳡻��н���
			{
				near_index = i;
				min_distance = distance(target->pos, pos);
			}
			else if(target->team == INFO->team_num && type != OILFIELD && type!=MINE)
			{
				near_index = i;
				min_distance = distance(target->pos, pos);
			}
			else ;
		}
	}
	return near_index;
}

//�ж϶Ըó�Ա�ĸ������Ƿ�����´����Ѿ��´���߼���ָ���򲻿ɣ�
int if_command(int i,CommandType type, ElementType target)
{
	const State *element = INFO->elements[i];
	if(type < FORWARD && type > command[i][0])  //���ƶ�ָ��
	{
		if(type == PRODUCE || type == FIX)     //������ά�޲���
		{
			if(element->type == BASE) return 1;
			return 0;
		}
		else if(type <= SUPPLYUNIT)      //��������
		{
			if(element->type >= FIGHTER 
				|| element->type == DESTROYER 
				|| element->type == SUBMARINE
				|| target == CARGO)
				return 0;
			return 1;
		}
		else if(target == kElementTypes)
			return 1;
		else if((kProperty[element->type].attacks[0] <= kProperty[target].defences[0] 
					|| kProperty[(int)target].defences[0] == -1)
			&& (kProperty[element->type].attacks[1] <= kProperty[target].defences[1]
					|| kProperty[(int)target].defences[1] == -1))
			return 0;
		else ;
		return 1;
	}
	if(type >= FORWARD && type > command[i][1])  //�ƶ�ָ��
	{   
		if(element->type <= FORT)
			return 0;//���غ;ݵ㲻���ƶ�
		return 1;
	}
	return 0;
}


//���ڻغϿ�ʼʱ���Ϊ1�ĵ�λ���в���
void Cargo_Supply(int index)
{
	const State *element, *target;
	element = INFO->elements[index];
	if(!if_command(index, SUPPLYUNIT))
		return;
	for(int j=0;j<INFO->element_num;j++)
	{
		target = INFO->elements[j];
		if(target->type == BASE && DisToBase(element->pos, target->pos)<=1
			&& target->team == INFO->team_num)
		{
			/*if((target->fuel + 0.5*element->fuel < kProperty[BASE].fuel_max
				&& element->fuel >= 0.4*kProperty[element->type].fuel_max)
			|| (target->metal + 0.7*element->metal < kProperty[BASE].metal_max
				&& element->metal >= 0.4*kProperty[element->type].metal_max))*/
			//{
				//Supply(INFO->elements[index], target->index, 0, element->fuel, element->fuel);
				command[index][0] = SUPPLYBASE;
				Supply(INFO->elements[index]->index, target->index, (int)(0.7*element->fuel), 0, element->metal);
			//}
				if(element->index == BaseMineCargo)
				{	
					/*int mine = GetNear(element->pos, MINE);
					if(mine < 0)
						return;*/
					ChangeDest(element->index, INFO->elements[GetNear(element->pos, MINE)]->pos);
					command[index][1] = CARGOSUPPLY;
					return;
				}

				if(element->index == BaseFuelCargo)
				{
					/*int fuel = GetNear(element->pos,OILFIELD);
					if(fuel < 0)
						return;*/
					ChangeDest(element->index, INFO->elements[GetNear(element->pos,OILFIELD)]->pos);
					command[index][1] = CARGOSUPPLY;
					return;
				}
				
				int MinFuel = 1000, UnitIndex = -1;
				for(int i=0; i<INFO->element_num; i++)
				{
					const State *unit = INFO->elements[i];
					if(unit->type > OILFIELD && unit->type != CARGO && unit->team == INFO->team_num
						&& unit->fuel < MinFuel)
					{
						MinFuel = unit->fuel;
						UnitIndex = i;
					}
				}
				if(UnitIndex != -1 && if_command(index, CARGOSUPPLY))
				{
					ChangeDest(element->index, INFO->elements[UnitIndex]->pos);
					command[index][1] = CARGOSUPPLY;
				}
				ChangeDest(element->index, INFO->elements[GetNear(element->pos, MINE)]->pos);
				command[index][1] = CARGOSUPPLY;
				return;
		}
		else if(target->type != CARGO &&
			distance(element->pos,target->pos) <= 1
			&& target->team == INFO->team_num)
		{
			if((target->fuel + 0.3*element->fuel < kProperty[target->type].fuel_max
				&& element->fuel >= 0.4*kProperty[element->type].fuel_max)
			|| (target->ammo + 0.3*element->ammo < kProperty[BASE].ammo_max
				&& element->ammo >= 0.3*kProperty[element->type].ammo_max))
			{
				command[index][0] = SUPPLYUNIT;
				Supply(INFO->elements[index]->index, INFO->elements[j]->index, kProperty[target->type].fuel_max - target->fuel,
						kProperty[target->type].ammo_max - target->ammo, 0);
				return;
			}
			//else if(target->type == FORT)command[index][0] = SUPPLYFORT;
			else continue;
		}
	}	
}

void Carrier_Supply(int index)
{
	const State *element, *target;
	element = INFO->elements[index];
	if(!if_command(index, SUPPLYUNIT))
		return;
	for(int j=0; j<INFO->element_num; j++)
	{
		target = INFO->elements[j];
		if(target->type > OILFIELD && target->type != CARGO &&
			distance(element->pos,target->pos) <= 1
			&& target->team == INFO->team_num)
		{
			if((target->fuel + 0.3*element->fuel < kProperty[target->type].fuel_max
				&& element->fuel >= 0.4*kProperty[element->type].fuel_max)
			|| (target->ammo + 0.3*element->ammo < kProperty[BASE].ammo_max
				&& element->ammo >= 0.3*kProperty[element->type].ammo_max))
			{
				command[index][0] = SUPPLYUNIT;
				Supply(INFO->elements[index]->index, INFO->elements[j]->index, kProperty[target->type].fuel_max - target->fuel,
						kProperty[target->type].ammo_max - target->ammo, 0);
				return;
			}
			//else if(target->type == FORT)command[index][0] = SUPPLYFORT;
			else continue;
		}		
	}
}

//���ڳ��������Ȼû����������䴬��ȥ��Դ���ռ������߻ػ��ز��������ߴ���ǰ��
void MoveCargo(int i)        
{
	const State *Element = INFO->elements[i];
	const State *base = INFO->elements[GetBase(INFO->team_num)];
	if(Element->type == CARGO && if_command(i, RETURNBASE))
	{
		if(Element->fuel < 0.3 * kProperty[CARGO].fuel_max)          //ȼ�ϲ���
		{
			int fuel = GetNear(Element->pos,OILFIELD);
			if(fuel < 0)
				return;
			ChangeDest(Element->index, INFO->elements[fuel]->pos);
			command[i][1] = CARGOSUPPLY;
			return;
		}
		else if(Element->metal < 0.5 * kProperty[CARGO].metal_max)     //��������
		{	
			if(Element->index == BaseFuelCargo)
			{
				if(Element->fuel > 0.5 * kProperty[CARGO].fuel_max)
				{
					int mine = GetNear(Element->pos, MINE);
					if(mine < 0)
						return;
					ChangeDest(Element->index, INFO->elements[mine]->pos);
					command[i][1] = CARGOSUPPLY;
					return;
				}
				else
				{
					int fuel = GetNear(Element->pos, OILFIELD);
					if(fuel < 0)
						return;
					ChangeDest(Element->index, INFO->elements[fuel]->pos);
					command[i][1] = CARGOSUPPLY;
					return;
				}
			}
			int mine = GetNear(Element->pos, MINE);
			if(mine < 0)
				return;
			ChangeDest(Element->index, INFO->elements[mine]->pos);
			command[i][1] = CARGOSUPPLY;
			return;
		}
		else if((Element->index == BaseMineCargo || Element->index == BaseFuelCargo)
			&& ((base->metal + 0.8*Element->metal < kProperty[BASE].metal_max
				&& Element->metal > 0.4*kProperty[Element->type].metal_max)
			|| (base->fuel + (int)(0.7*Element->fuel) < kProperty[BASE].fuel_max
				&& Element->fuel >0.6*kProperty[Element->type].fuel_max)))
		{
			ChangeDest(Element->index, base->pos);
			command[i][1] = CARGOSUPPLY;
			return;
		}
		else ;
	}

	if(if_command(i, FORWARD))                   //�ǲ���ǰ�ߵĴ�
	{
        int MinAmmo = 1000;
        Position target;
        for(int j=0;j<INFO->element_num;j++)
			if(INFO->elements[j]->ammo != -1
				&& INFO->elements[j]->ammo < MinAmmo 
                && INFO->elements[j]->type != CARGO
                && INFO->elements[j]->team == INFO->team_num)
            {
                MinAmmo = INFO->elements[j]->ammo;
                target = INFO->elements[j]->pos;
            }
        if(MinAmmo != 1000)
        {
            ChangeDest(Element->index, target);   
            command[i][1] = FORWARD;
			return;
        }
    }
}

//Ѱ������ڵĻ��ػ�ݵ㣬��ѡ��Ѫ�����ٵĵ�λ����
//����ǻ��ء��ݵ㣻���߹�����ΧС�ڵ��˾���ǰ��
//���������Χ���ڵ��˾������
void Attack(int index)
{
	const State	*Element = INFO->elements[index];
	int health = 1000,enemy_index = -1;
	int FORTindex = -1;
	if(!if_command(index,ATTACKBASE))
		return;
	for(int i=0;i<enemy_num;i++)
	{
		State *enemy = &enemy_element[i];
		if(enemy->type == BASE && enemy->team == 1-INFO->team_num
			&& DisToBase(Element->pos, enemy->pos) <= kProperty[Element->type].fire_ranges[SURFACE]
			&& if_command(index, ATTACKBASE, BASE))
		{												//�з�����
			AttackUnit(Element->index,enemy->index);
			command[index][0] = ATTACKBASE;
			if(if_command(index, FORWARD))
			{
				ChangeDest(Element->index, minus(Element->pos,enemy->pos,
					kProperty[Element->type].fire_ranges[kProperty[enemy->type].level]));
				command[index][1] = FORWARD;
			}
			return;
		}
		if(distance(Element->pos, enemy->pos) <= kProperty[Element->type].fire_ranges[kProperty[enemy->type].level])
		{	
			if(enemy->type == FORT
				&& enemy->team == 1-INFO->team_num
				&& if_command(index,ATTACKFORT, FORT))                                     //�ݵ�
			{
				AttackUnit(Element->index,enemy->index);
				command[index][0] = ATTACKFORT;
				if(if_command(index,FORWARD))
				{
					ChangeDest(Element->index, enemy->pos);
					command[index][1] = FORWARD;
				}
				return;
			}
			if(enemy->type == FORT 
				&& enemy->type != 1-INFO->team_num
				&& if_command(index,ATTACKFORT,FORT))
			{
				FORTindex = enemy->index;
			}
			if(health > enemy->health 
					&& if_command(index, ATTACKUNIT, ElementType(enemy->type)))
			{
				health=enemy->health;					//Ѱ��Ѫ�����ٵĵз���λ
				enemy_index=i;
			}
		}
	}
	if(enemy_index != -1)
	{
		AttackUnit(Element->index, enemy_element[enemy_index].index);
		command[index][0] = ATTACKUNIT;
		if(if_command(index,FORWARD)
			&& !if_in(INFO->elements[index]->index, base_defender, 3))
		{
			//���Զ�ڵз�
			if(kProperty[Element->type].fire_ranges[kProperty[enemy_element[enemy_index].type].level]
				> kProperty[enemy_element[enemy_index].type].fire_ranges[kProperty[Element->type].level])
				ChangeDest(Element->index, minus(Element->pos,enemy_element[enemy_index].pos,
					kProperty[Element->type].fire_ranges[kProperty[enemy_element[enemy_index].type].level]));
			//��̽��ڵз�
			else 
				ChangeDest(Element->index, enemy_element[enemy_index].pos);
			command[index][1] = FORWARD;
		}
		else if(if_command(index,FORWARD)
			&& if_in(INFO->elements[index]->index, base_defender, 3))
		{
			ChangeDest(Element->index, BaseDefendPos);
			command[index][1] = FORWARD;
		}
		return;
	}
	if(FORTindex != -1)
	{
		AttackUnit(Element->index, enemy_element[FORTindex].index);
		command[index][0] = ATTACKUNIT;
		if(if_command(index,FORWARD))
		{
			if(!if_in(INFO->elements[index]->index, base_defender, 3))
				ChangeDest(Element->index, enemy_element[FORTindex].pos);
			else
				ChangeDest(Element->index, BaseDefendPos);
			command[index][1] = FORWARD;
		}
	}
	
}

//���ص�ά�ޣ�����
void BaseAct(int index)
{
	const State *target;
	if(BaseMineCargo != -1 && !if_alive(BaseMineCargo))
	{
		Produce(CARGO);
		command[index][0] = PRODUCE;
		BaseMineCargo = -1;
		return;
	}
	if(BaseFuelCargo != -1 && !if_alive(BaseFuelCargo))
	{
		Produce(CARGO);
		command[index][0] = PRODUCE;
		BaseFuelCargo = -1;
		return;
	}

	//FIX
	for(int i=0;i<INFO->element_num;i++)
	{
		target = INFO->elements[i];
		if(DisToBase(target->pos, INFO->elements[index]->pos)<=1 
			&& target->index != INFO->elements[index]->index
			&& target->team == INFO->team_num)
		{
			if(target->health < 0.4*kProperty[target->type].health_max
				&& target->health != -1)
			{
				if(if_command(index, FIX))
				{	
					Fix(INFO->elements[index]->index, INFO->elements[i]->index);
					command[index][0] = FIX;
					return;
				}
			}
		}
	}

	Attack(index);
	//SUPPLY
	for(int i=0;i<INFO->element_num;i++)
	{
		target = INFO->elements[i];
		if(DisToBase(target->pos, INFO->elements[index]->pos)<=1 
			&& target->index != INFO->elements[index]->index
			&& target->type != CARGO
			&& target->team == INFO->team_num)
		{
			if(target->fuel < parameter[2]*kProperty[target->type].fuel_max
					|| target->ammo < parameter[3]*kProperty[target->type].ammo_max)
			{
				if(if_command(index, SUPPLYUNIT))
				{	
					Supply(INFO->elements[index]->index, INFO->elements[i]->index, kProperty[target->type].fuel_max-target->fuel, 
                                                        kProperty[target->type].ammo_max-target->ammo, 0);
					command[index][0] = SUPPLYUNIT;
					return;
				}
			}
		}
	}
	BaseProduce(index);
}

///���ص�����
void BaseProduce(int index)
{
	if(!if_command(index, PRODUCE))return;

	/*if(BEGINGAME <= 5)
	{*/
    for(int i=0; i<3; i++)
		if(base_defender[i] != -1 && !if_alive(base_defender[i]))
		{
			Produce(DESTROYER);
			command[index][0] = PRODUCE;
			base_defender[i] = -1;
			return;
		}

	ProduceState += 1;
	//Produce(SUBMARINE);
	if(ProduceState%5 == 0)
	{
		Produce(SUBMARINE);
		command[index][0] = PRODUCE;
		return;
	}
	else if(ProduceState%5 == 1)
	{
		Produce(FIGHTER);
		command[index][0] = PRODUCE;
		return;
	}
	else if(ProduceState%5 == 2)
	{
		Produce(DESTROYER);
		command[index][0] = PRODUCE;
		return;
	}
	else if(ProduceState%10 == 3)
	{
		Produce(SCOUT);
		command[index][0] = PRODUCE;
		return;
	}
	else if(ProduceState%10 == 8)
	{
		Produce(CARGO);
		command[index][0] = PRODUCE;
		return;
	}
	else if(ProduceState%5 == 4)
	{
		Produce(CARRIER);
		command[index][0] = PRODUCE;
		return;
	}
	else;
	
}

void DefineUnit() //flag = 0,1,2��ӦBaseMineCargo, BaseFuelCargo, base_defender
 //i��Ӧbase_defender�ı��
{
    const State *Element, *base; 
    base = INFO->elements[GetBase(INFO->team_num)];

	for(int i=0; i<3; i++)
		if(base_defender[i] <= 0 || !if_alive(base_defender[i]))
		{
			int Distance = 1000, index = -1;
			for(int j=0; j<INFO->element_num; j++)
			{
				Element = INFO->elements[j];
				if(Element->team == INFO->team_num && Distance > distance(Element->pos, base->pos) 
					&& Element->type == DESTROYER && !if_in(Element->index, base_defender, 3))
				{
					Distance = distance(Element->pos, base->pos);
					index = INFO->elements[j]->index;
				}
			 }
			if(Distance != 1000)
				base_defender[i] = index;
		}

	if(BaseMineCargo <= 0 || !if_alive(BaseMineCargo))
	{
		int Distance = 1000, index = -1;
		for(int j=0; j<INFO->element_num; j++)
		{
			Element = INFO->elements[j];
			if(Element->team == INFO->team_num && Element->type == CARGO 
				&& Distance > distance(Element->pos, base->pos) && Element->index != BaseFuelCargo)
			{
				Distance = distance(Element->pos, base->pos);
				index = INFO->elements[j]->index;
			}
		}
		if(Distance != 1000)
			BaseMineCargo = index;
	}
	if(BaseFuelCargo <= 0 || !if_alive(BaseFuelCargo))
	{
		int Distance = 1000, index = -1;
		for(int j=0; j<INFO->element_num; j++)
		{
			Element = INFO->elements[j];
			if(Element->team == INFO->team_num && Element->type == CARGO
				&& Distance > distance(Element->pos, base->pos) && Element->index != BaseMineCargo)
			{
				Distance = distance(Element->pos, base->pos);
				index = INFO->elements[j]->index;
			}
			}
		if(Distance != 1000)
			BaseFuelCargo = index;
	}
}
////Ѱ������ĵо�ǰ������������䴬�ʹ�
void Forward(int index)
{
	const State *Element = INFO->elements[index];
	int Distance=1000; 
	Position target;
	if(!if_command(index, FORWARD))return;
	if(Element->type == BASE || Element->type == FORT 
        || Element->type == CARGO)
		return;
	else if(if_in(INFO->elements[index]->index, base_defender, 3)
		&& if_command(index, FORWARD))
	{
		ChangeDest(Element->index, BaseDefendPos);
		command[index][1] = FORWARD;
		return;
	}

	else
	{
		for(int i=0;i<enemy_num;i++)
			if(distance(Element->pos, enemy_element[i].pos) < Distance
				&& if_command(index, ATTACKUNIT, ElementType(enemy_element[i].type)))
			{
				Distance = distance(Element->pos,enemy_element[i].pos);
				target = enemy_element[i].pos;
			}
	}
	if(Distance != 1000)
	{
		ChangeDest(Element->index, target);
		command[index][1] = FORWARD;	
	}
	
	Position enemyBase = INFO->elements[GetBase(1-INFO->team_num)]->pos;
	ChangeDest(Element->index, enemyBase);
	command[index][1] = FORWARD;
}
