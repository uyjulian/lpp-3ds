/*----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#------  This File is Part Of : ----------------------------------------------------------------------------------------#
#------- _  -------------------  ______   _   --------------------------------------------------------------------------#
#------ | | ------------------- (_____ \ | |  --------------------------------------------------------------------------#
#------ | | ---  _   _   ____    _____) )| |  ____  _   _   ____   ____   ----------------------------------------------#
#------ | | --- | | | | / _  |  |  ____/ | | / _  || | | | / _  ) / ___)  ----------------------------------------------#
#------ | |_____| |_| |( ( | |  | |      | |( ( | || |_| |( (/ / | |  --------------------------------------------------#
#------ |_______)\____| \_||_|  |_|      |_| \_||_| \__  | \____)|_|  --------------------------------------------------#
#------------------------------------------------- (____/  -------------------------------------------------------------#
#------------------------   ______   _   -------------------------------------------------------------------------------#
#------------------------  (_____ \ | |  -------------------------------------------------------------------------------#
#------------------------   _____) )| | _   _   ___   ------------------------------------------------------------------#
#------------------------  |  ____/ | || | | | /___)  ------------------------------------------------------------------#
#------------------------  | |      | || |_| ||___ |  ------------------------------------------------------------------#
#------------------------  |_|      |_| \____|(___/   ------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Licensed under the GPL License --------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Copyright (c) Nanni <lpp.nanni@gmail.com> ---------------------------------------------------------------------------#
#- Copyright (c) Rinnegatamante <rinnegatamante@gmail.com> -------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Credits : -----------------------------------------------------------------------------------------------------------#
#-----------------------------------------------------------------------------------------------------------------------#
#- Smealum for ctrulib and ftpony src ----------------------------------------------------------------------------------#
#- StapleButter for debug font -----------------------------------------------------------------------------------------#
#- Lode Vandevenne for lodepng -----------------------------------------------------------------------------------------#
#- Jean-loup Gailly and Mark Adler for zlib ----------------------------------------------------------------------------#
#- Special thanks to Aurelio for testing, bug-fixing and various help with codes and implementations -------------------#
#-----------------------------------------------------------------------------------------------------------------------*/
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <3ds.h>
#include "include/luaplayer.h"
#include "include/utils.h"
#include "include/syscalls_norm.h"

// Different kind of syscalls
typedef Result (*func_1x)();
typedef Result (*func_2x_type1)(u8 arg, u32 arg2);

// Different kind of databases for parser optimization
typedef struct
{
	char* name;
	func_1x callback;
}func_1x_db;
typedef struct
{
	char* name;
	func_2x_type1 callback_type1; // argsize 5 (1+4)
}func_3x_db;

// No args syscalls
func_1x_db db_1x[]=
{
	{"amInit", amInit}, // 0
	{"amExit", amExit_norm}, // 1
};

// Two args syscalls
func_3x_db db_3x[]=
{
	// args size = 5 (1+4)
	{"AM_FinishCiaInstall", (func_2x_type1)AM_FinishCiaInstall}, // 0
	{"AM_StartCiaInstall", (func_2x_type1)AM_StartCiaInstall}, // 1
};

static int lua_service(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
		if(argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	const char* srv = luaL_checkstring(L,1);
	Handle tmp;
	srvGetServiceHandle(&tmp, srv);
	lua_pushboolean(L, tmp);
	return 1;
}

static int lua_execall(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
		if(argc > 3 || argc < 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	const char* call = luaL_checkstring(L,1);
	
	// Parsing known syscalls
	if (argc == 1){
		u32 known_syscalls = sizeof(db_1x) / sizeof(func_1x_db);
		for (int i=0;i<known_syscalls;i++){
			if (strcmp(db_1x[i].name,call) == 0){
				lua_pushinteger(L, db_1x[i].callback());
				return 1;
			}
		}
	}else if (argc == 3){
		u32 known_syscalls = sizeof(db_3x) / sizeof(func_3x_db);
		for (int i=0;i<known_syscalls;i++){
			if (strcmp(db_3x[i].name,call) == 0){
				
				// Parsing arguments according to syscall
				if (i <= 1){
					u8 arg1 = luaL_checkinteger(L, 2);
					u32 arg2 = luaL_checkinteger(L, 3);
					lua_pushinteger(L, db_3x[i].callback_type1(arg1,arg2));
				}
				return 1;
			}
		}
	}
	return luaL_error(L, "unknown syscall.");
}

static int lua_readword(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
		if(argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	u32* word = (u32*)luaL_checkinteger(L, 1);
	lua_pushinteger(L, *word);
	return 1;
}

static int lua_getfh(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
		if(argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	Handle hdl = luaL_checkinteger(L, 1);
	fileStream* result = (fileStream*)malloc(sizeof(fileStream));
	result->isRomfs = false;
	result->handle = (u32)hdl;
	result->magic = 0xBEEFDEAD;
	lua_pushinteger(L,(u32)result);
	return 1;
}

static int lua_alloc(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
		if(argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	u32 size = (u32)luaL_checkinteger(L, 1);
	u8* memblock = (u8*)malloc(size);
	lua_pushinteger(L, (u32)memblock);
	return 1;
}

static int lua_free(lua_State *L){
	int argc = lua_gettop(L);
	#ifndef SKIP_ERROR_HANDLING
		if(argc != 1) return luaL_error(L, "wrong number of arguments.");
	#endif
	void* offset = (void*)luaL_checkinteger(L, 1);
	free(offset);
	return 0;
}

//Register our Core Functions
static const luaL_Reg Core_functions[] = {
	{"checkService",		lua_service},
	{"execCall",			lua_execall},
	{"getHandle",			lua_getfh},
	{"readWord",			lua_readword},
	{"free",				lua_free},
	{"alloc",				lua_alloc},
	{0, 0}
};

void luaCore_init(lua_State *L) {
	lua_newtable(L);
	luaL_setfuncs(L, Core_functions, 0);
	lua_setglobal(L, "Core");
}