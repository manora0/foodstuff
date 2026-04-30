#include "user.hpp"

void register_user_routes(httplib::Server& svr, sqlite3* db) {
//================================================================================================================================================
    svr.Get("/user-data", [db](const httplib::Request& req, httplib::Response& res) {
      if (!req.has_param("id")) { res.status = 400; return; }
      int user_id = std::stoi(req.get_param_value("id"));

      std::cout << "Fetching data for user_id: " << user_id << std::endl;

      sqlite3_stmt* stmt;
      sqlite3_prepare_v2(db, R"(
        select u.username, m.name, r.title from user u
        left join mealplan m on u.user_id = m.user_id
        left join mealplan_recipe mr on m.plan_id = mr.plan_id
        left join recipe r on mr.recipe_id = r.recipe_id
        where u.user_id = ?)", -1, &stmt, nullptr);
      
      sqlite3_bind_int(stmt, 1, user_id);

      if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        std::string html = "<p>Logged in as: " + username + "</p><h2>Meal Plans:</h2>";
        std::string current_plan;
        do {
          const char* meal_plan_name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
          const char* recipe_title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
          if (meal_plan_name && current_plan != meal_plan_name) {
            current_plan = meal_plan_name;
            html += "<h3>" + current_plan + "</h3><ul>";
          }
          if (recipe_title) {
            html += "<li>" + std::string(recipe_title) + "</li>";
          }
        } while (sqlite3_step(stmt) == SQLITE_ROW);
        if (!current_plan.empty()) html += "</ul>";
        res.set_content(html, "text/html");

      } else {
        std::cout << "No user found with user_id: " << user_id << std::endl;
        res.status = 404;
        res.set_content("User not found", "text/html");
      }
      sqlite3_finalize(stmt);
  });

//================================================================================================================================================

  svr.Post("/create-plan", [db](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("plan-name")) { res.status = 400; return; }
    std::string plan_name = req.get_param_value("plan-name");
    int user_id = 1; // For demo purposes, we use a fixed user_id
    std::string meal_name = plan_name.empty() ? "My Meal Plan" : plan_name;

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "INSERT INTO mealplan (user_id, name) VALUES (?, ?)", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, meal_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  });

//================================================================================================================================================

  svr.Delete("/schedule", [db](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("plan_id") || !req.has_param("day") || !req.has_param("meal")) {
      res.status = 400;
      return;
    }
    int plan_id = std::stoi(req.get_param_value("plan_id"));
    int day = std::stoi(req.get_param_value("day"));\
    std::string meal_type = req.get_param_value("meal");

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, R"(
      DELETE FROM mealplan_schedule WHERE plan_id = ? AND day = ? AND meal_type = ?)",
      -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, plan_id);
    sqlite3_bind_int(stmt, 2, day);
    sqlite3_bind_text(stmt, 3, meal_type.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    res.set_content("<p class=\" text-xs text-gray-300 italic\">Empty</p", "text/html");
  });

//================================================================================================================================================

  svr.Get("/schedule", [db](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("plan_id") || !req.has_param("day") || !req.has_param("meal")) {
      res.status = 400;
      return;
    }
    std::string plan_id_str = req.get_param_value("plan_id");
    if (plan_id_str.empty()) { res.status = 400; return; }
    int plan_id = std::stoi(plan_id_str);
    int day     = std::stoi(req.get_param_value("day"));
    std::string meal_type = req.get_param_value("meal");

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, R"(
      SELECT r.title, r.image_url, r.recipe_id
      FROM mealplan_schedule ms
      JOIN recipe r ON ms.recipe_id = r.recipe_id
      WHERE ms.plan_id = ? AND ms.day = ? AND ms.meal_type = ?
    )", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, plan_id);
    sqlite3_bind_int(stmt, 2, day);
    sqlite3_bind_text(stmt, 3, meal_type.c_str(), -1, SQLITE_STATIC);

    std::string html;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
      const char* title_raw     = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
      const char* image_url_raw = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      int recipe_id             = sqlite3_column_int(stmt, 2);
      std::string title     = title_raw     ? title_raw     : "";
      std::string image_url = image_url_raw ? image_url_raw : "";

      if (!image_url.empty())
        
      html += "<div id=\"cell-" + std::to_string(plan_id) + "-" + std::to_string(day) +
      "-" + meal_type + "\">"
          "<div class=\"relative group\">"
          "<img src=\"" + image_url + "\" class=\"w-full rounded mb-1 object-cover h-12\">"
          "<button @click=\"$dispatch('open-modal', {type: 'remove-recipe', recipe_id: " +
      std::to_string(recipe_id) +
          ", plan_id: " + std::to_string(plan_id) +
          ", day: " + std::to_string(day) +
          ", meal: '" + meal_type +
          "', cell: 'cell-" + std::to_string(plan_id) + "-" + std::to_string(day) +
      "-" + meal_type + "'})\""
          " class=\"absolute top-0 right-0 bg-red-500 text-white text-xs px-1 rounded-bl opacity-0 group-hover:opacity-100\">X</button>"
          "</div>"
          "<a href=\"/recipe/recipe.html?id=" + std::to_string(recipe_id) +
          "\" class=\"text-xs text-blue-500 hover:underline font-medium leading-tight\">" + title + "</a>"
          "</div>";
    } else {
      html += "<p class=\"text-xs text-gray-300 italic\">Empty</p>";
    }
    sqlite3_finalize(stmt);
    res.set_content(html, "text/html");
  });

//================================================================================================================================================

  svr.Post("/create-collection", [db](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("collection-name")) { res.status = 400; return; }
    std::string collection_name = req.get_param_value("collection-name");
    int user_id = 1; // For demo purposes, we use a fixed user_id

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "INSERT INTO collection (user_id, name) VALUES (?, ?)", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, user_id);
    sqlite3_bind_text(stmt, 2, collection_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
  });

//================================================================================================================================================

  svr.Delete("/meal-plan", [db](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("plan_id")) { res.status = 400; return; }
    int plan_id = std::stoi(req.get_param_value("plan_id"));
    int user_id = 1;

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "DELETE FROM mealplan WHERE plan_id = ? AND user_id = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, plan_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db, "SELECT plan_id, name FROM mealplan WHERE user_id = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, user_id);

    std::string html = "";
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      int pid = sqlite3_column_int(stmt, 0);
      const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      html += "<option value=\"" + std::to_string(pid) + "\"" + (first ? " selected" : "") + ">" + std::string(name) + "</option>";
      first = false;
    }
    sqlite3_finalize(stmt);
    res.set_content(html, "text/html");
  });

//================================================================================================================================================

  svr.Get("/meal-plans", [db](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("user_id")) { res.status = 400; return; }
    int user_id = std::stoi(req.get_param_value("user_id"));

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "SELECT active_plan_id FROM user WHERE user_id = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, user_id);
    int active_plan_id = -1;
    if (sqlite3_step(stmt) == SQLITE_ROW)
      active_plan_id = sqlite3_column_int(stmt, 0);
    sqlite3_finalize(stmt);

    sqlite3_prepare_v2(db, "SELECT plan_id, name FROM mealplan WHERE user_id = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, user_id);

    std::string html = "";
    bool first = true;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
      int plan_id = sqlite3_column_int(stmt, 0);
      const char* name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
      bool selected = (active_plan_id != -1) ? (plan_id == active_plan_id) : first;
      html += "<option value=\"" + std::to_string(plan_id) + "\"" + (selected ? " selected" : "") + ">" + std::string(name) + "</option>";
      first = false;
    }
    sqlite3_finalize(stmt);
    res.set_content(html, "text/html");
  });

//================================================================================================================================================

  svr.Put("/user/active-plan", [db](const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("plan_id")) { res.status = 400; return; }
    int plan_id = std::stoi(req.get_param_value("plan_id"));
    int user_id = 1;

    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(db, "UPDATE user SET active_plan_id = ? WHERE user_id = ?", -1, &stmt, nullptr);
    sqlite3_bind_int(stmt, 1, plan_id);
    sqlite3_bind_int(stmt, 2, user_id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    res.status = 200;
  });

//================================================================================================================================================

  // svr.Post("/generate-grocery-list", [db](const httplib::Request& req, httplib::Response& res) {

  //   sqlite3_stmt* stmt;
  //   int user_id = 1; //demo
  //   int plan_id = 1; //demo

  //   sqlite3_prepare_v2(db, "insert into grocery_list (user_id) values (?)", -1, &stmt, nullptr);
  //   sqlite3_bind_int(stmt, 1, user_id);
  //   sqlite3_step(stmt);
  //   sqlite3_finalize(stmt);
  //   int list_id = (int)sqlite3_last_insert_rowid(db);

  //   sqlite3_prepare_v2(db, R"(
  //     select i.name, ri.unit, ri.quantity, r.title) from mealplan_schedule mp
  //     join recipe r on mp.recipe_id = r.recipe_id
  //     join recipe_ingredients ri on ri.recipe_id = r.recipe_id
  //     join ingredient i on ri.ingredient_id = i.ingredient_id
  //     group by r.title)", -1, stmt);
    
    
  // });
  
}