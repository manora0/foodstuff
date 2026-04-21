#pragma once
#include"../../include/httplib.h"
#include<sqlite3.h>

void register_recipe_routes(httplib::Server& svr, sqlite3* db);