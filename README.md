## Project description
This project is a webapp with a server written in c++ using the httplib header library and htmx for the webpage front end. The main goal of this project is to allow users to view meals, add them to and schedule them in mealplans, add meals to collections, and create grocery lists from mealplans and collections. Currently the **Project only works with one user** because most of the functions run under the assumption that the user_id is 1.

## How to run the app (setup steps)
### Requirements
#### OpenSSL
The server requires that open ssl is installed 

> #### Winget
> winget install ShiningLight.OpenSSL  

> #### chocolatey
> choco install openssl  

> #### verify install:
> openssl version

#### SQLite
The server is built to use sqlite3, a sqlite3 lib is provided

#### SSL Keys
The server uses openSSL which requires SSL keys. Since there is no certified domain the keys need to be generated manually

> openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes  

> [!NOTE] Your browser will warn you that the domain is dangerous because the keys are self signed. Since the keys were signed by us there is no problem. Click advanced and proceed to the webpage

How to set up the database (schema + seed data)
Default test users (if applicable)