#include "user.hpp"

void register_user_routes(httplib::Server& svr, sqlite3* db) {
    svr.Get("/user-data", [db](const httplib::Request& req, httplib::Response& res) {
      if (!req.has_param("id")) { res.status = 400; return; }
      int user_id = std::stoi(req.get_param_value("id"));

      std::cout << "Fetching data for user_id: " << user_id << std::endl;

      sqlite3_stmt* stmt;
      sqlite3_prepare_v2(db, R"(
        select u.username, r.title from user u
        left join mealplan m on u.user_id = m.user_id
        left join mealplan_recipe mr on m.plan_id = mr.plan_id
        left join recipe r on mr.recipe_id = r.recipe_id
        where u.user_id = ?)", -1, &stmt, nullptr);
      
      sqlite3_bind_int(stmt, 1, user_id);

      if (sqlite3_step(stmt) == SQLITE_ROW) {
        std::string username = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        res.set_content("<p>Logged in as: " + username + "</p>", "text/html");
        res.set_content(res.body + "<h2>Meal Plan:</h2>", "text/html");
        do {
          const char* recipe_title = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
          if (recipe_title) {
            res.set_content(res.body + "<p>" + recipe_title + "</p>", "text/html");
          }
        } while (sqlite3_step(stmt) == SQLITE_ROW);

      } else {
        std::cout << "No user found with user_id: " << user_id << std::endl;
        res.status = 404;
        res.set_content("User not found", "text/html");
      }
      sqlite3_finalize(stmt);
  });
  
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
  
}