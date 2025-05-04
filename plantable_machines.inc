/*
* Machines that can be planted
* Players or entities near machines get affected
* Machines have self properties like health which can be adjusted
* Real time collision checking so they're planted properly without colliding world objects
*/

//============================================================

#include <a_samp>

#include <streamer>

#include <YSI\y_iterate>

#include <colandreas>

//============================================================



#if defined _plantable_machines_included
	#endinput
#endif
#define _plantable_machines_included


#define INVALID_MACHINE_ID 					-1
	
#define MAX_MACHINES						1000
	
#define BREACKEFFECT_MODELID 				18679
	
#define Z_THRESHOLD_TO_PLANT 				2.0
	
#define MDIST_FROM_PLAYER_WHEN_PLANTING 	3.0
	
#define MACHINE_MAX_NAME_LEN 				200

//============================================================

enum E_MACHINE_INFO
{
	E_MACHINE_PLAYER, 						//player who initiated the machine (is going to plant it)
	E_MACHINE_NAME[MACHINE_MAX_NAME_LEN], 	//machine name
		//-----below cells for attached obj 
	bool: E_MACHINE_PLAYER_ATTACHED, 		//check if player is attached to machine (yet to drop) or not, true means obj att to player, false means planted...
	E_MACHINE_ATTACH_OBJECT, 				//id to be used for attaching object
		//-----Planted machine vars
	E_MACHINE_MODEL, 						//model id of the machine
	E_MACHINE_OBJECTID, 					//id to use when planting machine on ground
	E_MACHINE_BREAKOBJECTID[4], 			//id to use for showing breaking obj when machine is destroyed
	bool: E_MACHINE_CP_SHOW, 				//to check if CP needs to be shown around machine or not
	Float: E_MACHINE_CURRENT_HEALTH, 		//machine current health
	Float: E_MACHINE_MAX_HEALTH, 			//machine maximum health
	E_MACHINE_CREATION_TIME, 				//time it should take to plant machine on ground
	E_MACHINE_AREA_TICKRATE, 				//time interval for checking if any player is inside area
	Float: E_MACHINE_POS_X, 				//machines coords
	Float: E_MACHINE_POS_Y, 				//machines coords
	Float: E_MACHINE_POS_Z, 				//machines coords
	Float: E_MACHINE_ROT_X, 				//machines coords
	Float: E_MACHINE_ROT_Y, 				//machines coords
	Float: E_MACHINE_ROT_Z, 				//machines coords
	STREAMER_TAG_CP: E_MACHINE_CP, 			//id to use for CP when machine planted on ground
	STREAMER_TAG_AREA: E_MACHINE_AREA, 		//area to create when machine planted on ground
	Float: E_MACHINE_RADIUS, 				//machines detection radius
	Text3D: E_MACHINE_TEXT 					//id for 3d text used to show machines health
};

//============================================================
//WARNING: Machines Info Stored in this array, changing these values affects machine functionality...
new
	g_MachineInfo[MAX_MACHINES][E_MACHINE_INFO],
	Iterator: Iter_Machines<MAX_MACHINES>;

//============================================================

//callbacks to be used in main script...
forward OnPlayerEnterMachineRange(playerid,index);

forward OnPlayerExitMachineRange(playerid,index);

forward OnPlayerShootMachine(playerid,index,weaponid);

forward OnMachineDestroyed(playerid,index);

forward OnPlayerInsideAreaPerTickRate(playerid,index);

//============================================================

// To destroy the machine
stock Machine_Destroy(index)
{
	if (!Iter_Contains(Iter_Machines, index))
		return 0;

	Iter_Remove(Iter_Machines, index);


	if (IsValidDynamicObject(g_MachineInfo[index][E_MACHINE_OBJECTID]) ) 
	{
		DestroyDynamicObject(g_MachineInfo[index][E_MACHINE_OBJECTID]);
	}

	if(IsPlayerAttachedObjectSlotUsed(g_MachineInfo[index][E_MACHINE_PLAYER], g_MachineInfo[index][E_MACHINE_ATTACH_OBJECT]))
	{
		RemovePlayerAttachedObject(g_MachineInfo[index][E_MACHINE_PLAYER], g_MachineInfo[index][E_MACHINE_ATTACH_OBJECT]);
	}

	if (IsValidDynamic3DTextLabel(g_MachineInfo[index][E_MACHINE_TEXT]) ) 
	{
		DestroyDynamic3DTextLabel(g_MachineInfo[index][E_MACHINE_TEXT]);
	}

	//what to reset this value to??
	if (IsValidDynamicCP(g_MachineInfo[index][E_MACHINE_CP]) )
	{
		DestroyDynamicCP(g_MachineInfo[index][E_MACHINE_CP]);
	}


	if ( IsValidDynamicArea(g_MachineInfo[index][E_MACHINE_AREA]) )
	{
		DestroyDynamicArea(g_MachineInfo[index][E_MACHINE_AREA]);
	}

	//create breaking objects and set timer to remove them in 1 sec
	for(new BreakableObj = 0; BreakableObj < 4; BreakableObj++)
	{
		g_MachineInfo[index][E_MACHINE_BREAKOBJECTID][BreakableObj] = CreateDynamicObject(BREACKEFFECT_MODELID, g_MachineInfo[index][E_MACHINE_POS_X] + ( floatsin(-BreakableObj * 90.0, degrees) * 1.5 ), g_MachineInfo[index][E_MACHINE_POS_Y] + ( floatcos(-BreakableObj * 90.0, degrees) * 1.5 ), g_MachineInfo[index][E_MACHINE_POS_Z] - 1.0, 0.000000, 0.000000, 0.000000, -1, -1, -1, 300.00, 300.00);
	}
	Streamer_Update(g_MachineInfo[index][E_MACHINE_PLAYER]);
	SetTimerEx("Machine_helper_DestroyBreakObj", 1000, false, "i", index);

	CallLocalFunction("OnMachineDestroyed","ii",g_MachineInfo[index][E_MACHINE_PLAYER], index);

	//resetting all values
	g_MachineInfo[index][E_MACHINE_PLAYER] = INVALID_PLAYER_ID;
	g_MachineInfo[index][E_MACHINE_PLAYER_ATTACHED] = false;

	g_MachineInfo[index][E_MACHINE_TEXT] = Text3D:INVALID_3DTEXT_ID;
	g_MachineInfo[index][E_MACHINE_MODEL] = INVALID_STREAMER_ID;

	g_MachineInfo[index][E_MACHINE_CP_SHOW] = false;
	g_MachineInfo[index][E_MACHINE_POS_X] = 0.0;
	g_MachineInfo[index][E_MACHINE_POS_Y] = 0.0; 
	g_MachineInfo[index][E_MACHINE_POS_Z] = 0.0;

	g_MachineInfo[index][E_MACHINE_ROT_X] = 0.0;
	g_MachineInfo[index][E_MACHINE_ROT_Y] = 0.0; 
	g_MachineInfo[index][E_MACHINE_ROT_Z] = 0.0;

	g_MachineInfo[index][E_MACHINE_RADIUS] = 0.0;

	strdel(g_MachineInfo[index][E_MACHINE_NAME], 0, MACHINE_MAX_NAME_LEN-1);

	g_MachineInfo[index][E_MACHINE_CREATION_TIME] = 0;

	return 1;
}

forward Machine_helper_DestroyBreakObj(index);

//called after machine is destroyed by timer to destroy break objects...
public Machine_helper_DestroyBreakObj(index)
{
	for(new BreakableObj = 0; BreakableObj < 4; BreakableObj++)
	{
		if (IsValidDynamicObject(g_MachineInfo[index][E_MACHINE_BREAKOBJECTID][BreakableObj]) ) 
		{
			DestroyDynamicObject(g_MachineInfo[index][E_MACHINE_BREAKOBJECTID][BreakableObj]);
		}
	}
}


forward Machine_AreaTickRateCalled(index);

//called based on machines tick rate every n ms
public Machine_AreaTickRateCalled(index)
{
	if (Iter_Contains(Iter_Machines, index))
	{
		//as long as machine isnt destroyed keep calling the function
		foreach(new i : Player)
		{
			new Float: x,Float: y,Float: z;

			GetPlayerPos(i, x,y,z);

			if(IsPlayerInDynamicArea(i,g_MachineInfo[index][E_MACHINE_AREA], .recheck = 0) && !(CA_RayCastLine(x, y, z, g_MachineInfo[index][E_MACHINE_POS_X],g_MachineInfo[index][E_MACHINE_POS_Y],g_MachineInfo[index][E_MACHINE_POS_Z], x, y, z)) )
			{
				CallLocalFunction("OnPlayerInsideAreaPerTickRate", "ii", i,index);
			}
		
		}

		SetTimerEx("Machine_AreaTickRateCalled", g_MachineInfo[index][E_MACHINE_AREA_TICKRATE], false,"i", index);

	}
}

//to see if machine is being used or not...
stock Machine_IsValid(index)
{
	if(Iter_Contains(Iter_Machines, index) /*&& g_MachineInfo[index][E_MACHINE_PLAYER] == playerid*/)
	{
		return 1;
	}
	else
	{
		return 0;
	}
}

Float: CalculateDistance3D(Float: x1, Float: y1, Float: z1, Float: x2, Float: y2, Float: z2)
{
    // Calculate the differences in each dimension
    new Float:dx = x2 - x1;
    new Float:dy = y2 - y1;
    new Float:dz = z2 - z1;

	// Calculate the distance using the Euclidean formula for 3D space
    return floatsqroot(dx * dx + dy * dy + dz * dz);
}


//takes player pos input, before planting we check if this is valid point to plant or not...
stock Machine_helper_IsValidPlant(index,Float: x, Float: y, Float: z, Float:ang = 0.0)
{

	/*========================================================
	
	//We need to do TWO checks to be collision safe here...

	1) the player z and machine supposed z diff should be below certain threshold...
	So player cant plant against top of buildings or fences but slope is allowed but not too much...

	2)cast a line from player pos to obj pos and if line results pos between then theres an object in between so dont let them plant...

	3) after getting obj pos 
		--| ray explode from centre of obj to radius of obj
		--| get all collided points
		--| discard collided points below obj bounding box MinZ
		--| get min dist from centre collided point
		--| if this point is inside bounding box means, its colliding with the world
	========================================================*/


	new Float:Machine_AssumedX, Float:Machine_AssumedY,Float:Machine_AssumedZ;

	Machine_AssumedX = x + ( floatsin(-ang, degrees) * MDIST_FROM_PLAYER_WHEN_PLANTING );
	Machine_AssumedY = y + ( floatcos(-ang, degrees) * MDIST_FROM_PLAYER_WHEN_PLANTING );

	//finding point on ground in front of player
	CA_FindZ_For2DCoord(Machine_AssumedX,Machine_AssumedY,Machine_AssumedZ);

	//##################### POINT 1 CHECKING ######################################
	/*
	1)new way of z collision checking
		--|machine_assumedZ is on ground point z now...
		--|get bounding box of model, it gives max and min z offset to centre of object
		--|we need to move UP from ground to Z distance away to make it plant on ground
	*/
	new Float:ModelMaxX,Float:ModelMaxY,Float:ModelMaxZ,Float:ModelMinX,Float:ModelMinY,Float:ModelMinZ;

	CA_GetModelBoundingBox(g_MachineInfo[index][E_MACHINE_MODEL],ModelMinX, ModelMinY, ModelMinZ, ModelMaxX, ModelMaxY, ModelMaxZ);


	//now move MinZ away from ground point...Minz is negative so multiply it with -1
	new Float:Ground_Z = Machine_AssumedZ;

	Machine_AssumedZ = Machine_AssumedZ + ( -1.0 * ModelMinZ);

	//##################### POINT 3 CHECKING ######################################	

	/*
	3) after getting obj pos 
		--| ray explode from centre of obj to radius of obj
		--| get all collided points
		--| discard collided points below obj bounding box MinZ
		--| get min dist from centre collided point
		--| if this point is inside bounding box means, its colliding with the world
	*/

	//get obj size radius first
	new Float:ObjRadius = CalculateDistance3D(ModelMaxX,ModelMaxY,ModelMaxZ,Machine_AssumedX,Machine_AssumedY,Machine_AssumedZ);

	//now ray explode using this radius

	new Float: Collided_Points[1000][3];

	new CollidedPointsCount = CA_RayCastExplode(Machine_AssumedX, Machine_AssumedY, Machine_AssumedZ, ObjRadius,20.0, Collided_Points);
	
	//for points below Minz Discard them and get min dist point

	new Float:MinCollidedX,Float:MinCollidedY,Float:MinCollidedZ;

	new Float:ObjCollidedPointDist = ObjRadius + 1.0;
	for(new i = 0; i < CollidedPointsCount; i++)
	{
		//if collided point z is above machine_planted ground Z AND  
		if(Collided_Points[i][2] > Ground_Z \
			&&  \
			ObjCollidedPointDist > CalculateDistance3D(Collided_Points[i][0],Collided_Points[i][1],Collided_Points[i][2],Machine_AssumedX,Machine_AssumedY,Machine_AssumedZ) )
		{
			MinCollidedX = Collided_Points[i][0];
			MinCollidedY = Collided_Points[i][1];
			MinCollidedZ = Collided_Points[i][2];

			ObjCollidedPointDist = CalculateDistance3D(Collided_Points[i][0],Collided_Points[i][1],Collided_Points[i][2],Machine_AssumedX,Machine_AssumedY,Machine_AssumedZ);
		}
	}

	new bool:ObjTouchingWithWorld = false;
	//now check if min dist point inside bounding sphere
	if( (Machine_AssumedX + ModelMinX) <= MinCollidedX  && MinCollidedX  <= (Machine_AssumedX + ModelMaxX) &&\
		(Machine_AssumedY + ModelMinY) <= MinCollidedY  && MinCollidedY  <= (Machine_AssumedY + ModelMaxY) &&\
		(Machine_AssumedZ + ModelMinZ) <= MinCollidedZ  && MinCollidedZ <= (Machine_AssumedZ + ModelMaxZ))
	{
		ObjTouchingWithWorld = true;
	}

	//#############################################################################	
	new Float: DummyX,Float:DummyY,Float:DummyZ;
	
	//z threshold checking and line cast block obj checking...
	if(floatabs(Machine_AssumedZ - z) <= Z_THRESHOLD_TO_PLANT  \
		&& !(CA_RayCastLine(x, y, z, Machine_AssumedX,Machine_AssumedY,Machine_AssumedZ, DummyX, DummyY, DummyZ)) \
		&& ObjTouchingWithWorld == false
	)
	{
		Machine_helper_ChangePosValues(index,Float:Machine_AssumedX,Float:Machine_AssumedY,Float:Machine_AssumedZ);
		return 1;
	}

	return 0;
}
stock Machine_helper_ChangePosValues(index,Float:x,Float:y,Float:z)
{
	g_MachineInfo[index][E_MACHINE_POS_X] = x;
	g_MachineInfo[index][E_MACHINE_POS_Y] = y;
	g_MachineInfo[index][E_MACHINE_POS_Z] = z;
	
}

//called after player does the animation for planting (called by Machine_Helper_StartPlantProc())
//the machine pos must be preset before calling this!
stock Machine_PlantOnGroundByPlayer(index)
{
	if (!Iter_Contains(Iter_Machines, index))
		return 0;

	//while creating on ground, check if attached obj is present, if so delete
	//if (IsValidDynamicObject( g_MachineInfo[index][E_MACHINE_ATTACH_OBJECT]) )
	//{
	//	DestroyDynamicObject(g_MachineInfo[index][E_MACHINE_ATTACH_OBJECT]);
	//}

	if(IsPlayerAttachedObjectSlotUsed(g_MachineInfo[index][E_MACHINE_PLAYER], g_MachineInfo[index][E_MACHINE_ATTACH_OBJECT]))
	{
		RemovePlayerAttachedObject(g_MachineInfo[index][E_MACHINE_PLAYER], g_MachineInfo[index][E_MACHINE_ATTACH_OBJECT]);
	}


	//Object creation...
	//replace object arguments with func parameters later...
	g_MachineInfo[index][E_MACHINE_OBJECTID] = CreateDynamicObject(g_MachineInfo[index][E_MACHINE_MODEL], g_MachineInfo[index][E_MACHINE_POS_X] , g_MachineInfo[index][E_MACHINE_POS_Y], g_MachineInfo[index][E_MACHINE_POS_Z] , g_MachineInfo[index][E_MACHINE_ROT_X], g_MachineInfo[index][E_MACHINE_ROT_Y], g_MachineInfo[index][E_MACHINE_ROT_Z], -1, -1, -1, 300.00, 300.00);
	 

	//3DText Health and machine name Creation...
	//position it above object so testlos = true works...

	new Float:ModelMaxX,Float:ModelMaxY,Float:ModelMaxZ,Float:ModelMinX,Float:ModelMinY,Float:ModelMinZ;

	CA_GetModelBoundingBox(g_MachineInfo[index][E_MACHINE_MODEL],ModelMinX, ModelMinY, ModelMinZ, ModelMaxX, ModelMaxY, ModelMaxZ);

	new text[200]; 

	format(text, sizeof text,"%s \n\n Health: \n %.2f/%.2f",g_MachineInfo[index][E_MACHINE_NAME], g_MachineInfo[index][E_MACHINE_MAX_HEALTH],g_MachineInfo[index][E_MACHINE_MAX_HEALTH]); 
	
	g_MachineInfo[index][E_MACHINE_TEXT] = CreateDynamic3DTextLabel(text, 0x00FF00FF,g_MachineInfo[index][E_MACHINE_POS_X] , g_MachineInfo[index][E_MACHINE_POS_Y], g_MachineInfo[index][E_MACHINE_POS_Z] + ModelMaxZ + 0.2, 100.0,.testlos = true);
		

	//CP creation
	if(g_MachineInfo[index][E_MACHINE_CP_SHOW] == true)
	{
		g_MachineInfo[index][E_MACHINE_CP] = CreateDynamicCPEx(g_MachineInfo[index][E_MACHINE_POS_X] , g_MachineInfo[index][E_MACHINE_POS_Y], g_MachineInfo[index][E_MACHINE_POS_Z]-2, .size = g_MachineInfo[index][E_MACHINE_RADIUS] + 3.8);
	}

	//Area Creation...
	g_MachineInfo[index][E_MACHINE_AREA] = CreateDynamicCylinder(g_MachineInfo[index][E_MACHINE_POS_X] , g_MachineInfo[index][E_MACHINE_POS_Y], .minz =  g_MachineInfo[index][E_MACHINE_POS_Z]-5, .maxz =  g_MachineInfo[index][E_MACHINE_POS_Z] + 10.0, .size = g_MachineInfo[index][E_MACHINE_RADIUS]);

	//make everything visible to all players instantly using update...
	foreach (new i : Player)
	{
		Streamer_Update(i);
	}

	//start ticking area callback...
	Machine_AreaTickRateCalled(index);

	return 1;
}


//to be called by timer, process till PlantingTimeCounter is 0 and applies anim with gametext
forward Machine_Helper_StartPlantProc(index,PlantingTimeCounter);

public Machine_Helper_StartPlantProc(index,PlantingTimeCounter)

{

	if(PlantingTimeCounter != 0)
	{
		if(PlantingTimeCounter == g_MachineInfo[index][E_MACHINE_CREATION_TIME]) //when called for first time after key press
		{
			//ClearAnimations(g_MachineInfo[index][E_MACHINE_PLAYER], .forcesync = 0);
		
			ApplyAnimation(g_MachineInfo[index][E_MACHINE_PLAYER], "BOMBER", "BOM_Plant", 4.1, true, 0, 0, 0, 0, .forcesync = 0);
		}
		
		new text[30];
		format(text, sizeof text, "~p~Planting:~n~ %d~p~", PlantingTimeCounter);
		GameTextForPlayer(g_MachineInfo[index][E_MACHINE_PLAYER], text, 1000, 3);

		SetTimerEx("Machine_Helper_StartPlantProc", 1000, false,"ii", index,PlantingTimeCounter-1);
	}
	else //timer reached 0, means now machine needs to be planted
	{
		ClearAnimations(g_MachineInfo[index][E_MACHINE_PLAYER], .forcesync = 0);

		new Float: x,Float: y,Float: z,Float: ang;

		GetPlayerPos(g_MachineInfo[index][E_MACHINE_PLAYER], x,y,z);
		GetPlayerFacingAngle(g_MachineInfo[index][E_MACHINE_PLAYER], ang);

		Machine_PlantOnGroundByPlayer(index);

	}
}
//when attaching obj to player, attach in free slot...
stock CountAttachedObjects(playerid)
{
    new count;
    for (new i = 0; i < 10; i++)
    {
        if (IsPlayerAttachedObjectSlotUsed(playerid, i))
        {
            count++;
        }
    }
    return count;
}

//initial func: call to start machine process for that player and attach obj to player...
stock Machine_Create(playerid,MachineName[],modelid,Float:health, Float:radius,bool:IsCheckPointRequired = false,CheckPlayerInAreaTimeTickRate = 1000,CreationTime = 2)
{

	new free_index = Iter_Free(Iter_Machines);

	if (free_index == -1)
		return 0;

	Iter_Add(Iter_Machines, free_index);


	g_MachineInfo[free_index][E_MACHINE_PLAYER] = playerid;
	strins(g_MachineInfo[free_index][E_MACHINE_NAME], MachineName,0);
	//strcpy(g_MachineInfo[free_index][E_MACHINE_NAME], MachineName); 
	g_MachineInfo[free_index][E_MACHINE_PLAYER_ATTACHED] = true;
	g_MachineInfo[free_index][E_MACHINE_CP_SHOW] = IsCheckPointRequired;
	g_MachineInfo[free_index][E_MACHINE_MODEL] = modelid;
	g_MachineInfo[free_index][E_MACHINE_CURRENT_HEALTH] = health;
	g_MachineInfo[free_index][E_MACHINE_MAX_HEALTH] = health;
	g_MachineInfo[free_index][E_MACHINE_RADIUS] = radius;
	g_MachineInfo[free_index][E_MACHINE_CREATION_TIME] = CreationTime;
	g_MachineInfo[free_index][E_MACHINE_AREA_TICKRATE] = CheckPlayerInAreaTimeTickRate;
	

	//create attached object but only visible to player...
	//g_MachineInfo[free_index][E_MACHINE_ATTACH_OBJECT] = CreateDynamicObject(g_MachineInfo[free_index][E_MACHINE_MODEL], 0.11499,  0.282715,  1.509998, 0.0,0.0,0.0,.playerid = playerid);

	g_MachineInfo[free_index][E_MACHINE_ATTACH_OBJECT] = CountAttachedObjects(playerid);

	SetPlayerAttachedObject(g_MachineInfo[free_index][E_MACHINE_PLAYER], CountAttachedObjects(playerid), g_MachineInfo[free_index][E_MACHINE_MODEL], 17, 1.0,MDIST_FROM_PLAYER_WHEN_PLANTING - 1.0, 0.0,0.0,90.0,0.0);

	new Float:ang;
	GetPlayerFacingAngle(playerid, ang);
	
	//attach object to player
	//needs  YSF in order to attach to player dynamically...
	//AttachDynamicObjectToPlayer(g_MachineInfo[free_index][E_MACHINE_ATTACH_OBJECT], playerid, 0.0,MDIST_FROM_PLAYER_WHEN_PLANTING - 1.0, 1.0, 0.0,0.0,0.0);

	return free_index;
}


//to be called to decrease/increase machine health, it also checks if machines needs to be destroyed...
stock Machine_AddHealth(index,Float: HealthAmtToAdd)
{

	g_MachineInfo[index][E_MACHINE_CURRENT_HEALTH] = g_MachineInfo[index][E_MACHINE_CURRENT_HEALTH]  + HealthAmtToAdd;

	g_MachineInfo[index][E_MACHINE_CURRENT_HEALTH] = clamp(floatround(g_MachineInfo[index][E_MACHINE_CURRENT_HEALTH]), 0, floatround(g_MachineInfo[index][E_MACHINE_MAX_HEALTH]) );

	Machine_helper_UpdateTextBox(index);

	if(g_MachineInfo[index][E_MACHINE_CURRENT_HEALTH] < 1.0)
	{
		Machine_Destroy(index);		
	}

}

stock Machine_helper_UpdateTextBox(index)
{
	new text[200]; 

	format(text, sizeof text,"%s \n\n Health: \n %.2f/%.2f", g_MachineInfo[index][E_MACHINE_NAME], g_MachineInfo[index][E_MACHINE_CURRENT_HEALTH],g_MachineInfo[index][E_MACHINE_MAX_HEALTH] ); 

	if (g_MachineInfo[index][E_MACHINE_CURRENT_HEALTH] > 20.0)
	{
		UpdateDynamic3DTextLabelText(g_MachineInfo[index][E_MACHINE_TEXT], 0x00FF00FF, text);
	} 
	else
	{
		UpdateDynamic3DTextLabelText(g_MachineInfo[index][E_MACHINE_TEXT], 0xFF0000FF, text);
	}
}

stock Machine_SetMaxHealth(index,Float: AmtToSet)
{
	g_MachineInfo[index][E_MACHINE_MAX_HEALTH]  = AmtToSet;

	Machine_helper_UpdateTextBox(index);
}

stock Machine_GetHealth(index)
{
	return g_MachineInfo[index][E_MACHINE_MAX_HEALTH];
}


stock Machine_GetCurrentHealth(index)
{
	return g_MachineInfo[index][E_MACHINE_CURRENT_HEALTH];
}

stock Machine_GetMachineOwnderID(index)
{
	return g_MachineInfo[index][E_MACHINE_PLAYER];
}
//----------------------------------------------------------------------------------------------

//callbacks effects and hooks
public OnPlayerDisconnect(playerid, reason)
{
	foreach (new i : Iter_Machines)
	{
		if( g_MachineInfo[i][E_MACHINE_PLAYER] == playerid )
		{
			Machine_Destroy(i);
		}
	}

	#if defined MACH_OnPlayerDisconnect
        return MACH_OnPlayerDisconnect(playerid, reason);
    #else
        return 1;
    #endif

}

#if defined _ALS_OnPlayerDisconnect
    #undef OnPlayerDisconnect
#else
    #define _ALS_OnPlayerDisconnect
#endif
#define OnPlayerDisconnect MACH_OnPlayerDisconnect
#if defined MACH_OnPlayerDisconnect
    forward MACH_OnPlayerDisconnect(playerid, reason);
#endif

//========================================

public OnPlayerDeath(playerid, killerid, reason)
{
	foreach (new i : Iter_Machines)
	{
		if( g_MachineInfo[i][E_MACHINE_PLAYER] == playerid )
		{
			Machine_Destroy(i);
		}
	}

	#if defined MACH_OnPlayerDeath
        return MACH_OnPlayerDeath(playerid, killerid, reason);
    #else
        return 1;
    #endif

}

#if defined _ALS_OnPlayerDeath
    #undef OnPlayerDeath
#else
    #define _ALS_OnPlayerDeath
#endif
#define OnPlayerDeath MACH_OnPlayerDeath
#if defined MACH_OnPlayerDeath
    forward MACH_OnPlayerDeath(playerid, killerid, reason);
#endif


//========================================


public OnPlayerKeyStateChange(playerid, newkeys, oldkeys)
{
	foreach (new i : Iter_Machines)
	{
		//if player machine attached to him and pressed the KEY, then player is trying to plant the object
		if ((newkeys & KEY_YES) && !(oldkeys & KEY_YES) &&  g_MachineInfo[i][E_MACHINE_PLAYER] == playerid && g_MachineInfo[i][E_MACHINE_PLAYER_ATTACHED] == true)
		{

			new Float: x,Float: y,Float: z,Float: ang;

			GetPlayerPos(playerid, x,y,z);
			GetPlayerFacingAngle(playerid, ang);
			//if player planting pos is valid then supposed obj points are copied in this func check,after start planting process...
			if(Machine_helper_IsValidPlant(i,x,y,z,ang) == 1)
			{

				//toggle off since obj is attached and we dont like player to spam Y to plant again and again...
				g_MachineInfo[i][E_MACHINE_PLAYER_ATTACHED] = false;
				Machine_Helper_StartPlantProc(i,g_MachineInfo[i][E_MACHINE_CREATION_TIME]);
			}//else tell player not to plant there
			else
			{
				GameTextForPlayer(playerid, "~r~Invalid Position!~r~", 1000, 3);
			}

		}

	}

	#if defined MACH_OnPlayerKeyStateChange
        return MACH_OnPlayerKeyStateChange(playerid, newkeys, oldkeys);
    #else
        return 1;
    #endif

}

#if defined _ALS_OnPlayerKeyStateChange
    #undef OnPlayerKeyStateChange
#else
    #define _ALS_OnPlayerKeyStateChange
#endif
#define OnPlayerKeyStateChange MACH_OnPlayerKeyStateChange
#if defined MACH_OnPlayerKeyStateChange
    forward MACH_OnPlayerKeyStateChange(playerid, newkeys, oldkeys);
#endif


//========================================

public OnPlayerEnterDynamicArea(playerid,areaid)
{

	foreach (new i : Iter_Machines)
	{
		if(areaid == g_MachineInfo[i][E_MACHINE_AREA])
		{
			CallLocalFunction("OnPlayerEnterMachineRange","ii", playerid,i);

		}
	}

	#if defined MACH_OnPlayerEnterDynamicArea
        return MACH_OnPlayerEnterDynamicArea(playerid,areaid);
    #else
        return 1;
    #endif

}

#if defined _ALS_OnPlayerEnterDynamicArea
    #undef OnPlayerEnterDynamicArea
#else
    #define _ALS_OnPlayerEnterDynamicArea
#endif
#define OnPlayerEnterDynamicArea MACH_OnPlayerEnterDynamicArea
#if defined MACH_OnPlayerEnterDynamicArea
    forward MACH_OnPlayerEnterDynamicArea(playerid,areaid);
#endif

//========================================

public OnPlayerLeaveDynamicArea(playerid,areaid)
{
	foreach (new i : Iter_Machines)
	{
		if(areaid == g_MachineInfo[i][E_MACHINE_AREA])
		{
			CallLocalFunction("OnPlayerExitMachineRange","ii", playerid,i);
		}
	}

	#if defined MACH_OnPlayerLeaveDynamicArea
        return MACH_OnPlayerLeaveDynamicArea(playerid,areaid);
    #else
        return 1;
    #endif

}

#if defined _ALS_OnPlayerLeaveDynamicArea
    #undef OnPlayerLeaveDynamicArea
#else
    #define _ALS_OnPlayerLeaveDynamicArea
#endif
#define OnPlayerLeaveDynamicArea MACH_OnPlayerLeaveDynamicArea
#if defined MACH_OnPlayerLeaveDynamicArea
    forward MACH_OnPlayerLeaveDynamicArea(playerid,areaid);
#endif



//========================================


public OnPlayerShootDynamicObject(playerid, weaponid,objectid, Float:x, Float:y, Float:z)
{
	

	foreach (new i : Iter_Machines)
	{
		if(objectid == g_MachineInfo[i][E_MACHINE_OBJECTID])
		{
			SendClientMessage(playerid, 0xFF00FF00, "Shot the machine!");
			CallLocalFunction("OnPlayerShootMachine","iii", playerid,i,weaponid);

		}
	}

	#if defined MACH_OnPlayerShootDynamicObject
        return MACH_OnPlayerShootDynamicObject(playerid, weaponid,objectid, Float:x, Float:y, Float:z);
    #else
        return 1;
    #endif

}

#if defined _ALS_OnPlayerShootDynamicObject
    #undef OnPlayerShootDynamicObject
#else
    #define _ALS_OnPlayerShootDynamicObject
#endif
#define OnPlayerShootDynamicObject MACH_OnPlayerShootDynamicObject
#if defined MACH_OnPlayerShootDynamicObject
    forward MACH_OnPlayerShootDynamicObject(playerid, weaponid,objectid, Float:x, Float:y, Float:z);
#endif

//========================================

