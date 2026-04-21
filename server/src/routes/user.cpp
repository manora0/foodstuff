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
  
  
}