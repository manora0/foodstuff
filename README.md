## Project description
This project is a webapp with a server written in c++ using the httplib header library and htmx for the webpage front end. The main goal of this project is to allow users to view meals, add them to and schedule them in mealplans, add meals to collections, and create grocery lists from mealplans and collections. Currently the **Project only works with one user** because most of the functions run under the assumption that the user_id is 1.

## How to run the app (setup steps)
### Requirements
#### OpenSSL
The server requires that open ssl is installed 

##### Winget

```sh
winget source update
winget install ShiningLight.OpenSSL.Light
```

##### chocolatey

```sh
choco install openssl
```

##### verify install:

```sh 
openssl version
```

#### SSL Keys
The server uses openSSL which requires SSL keys. Since there is no certified domain the keys need to be generated manually.

**These Keys Must Be Put in the Build Directory**

```sh
openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 365 -nodes
```

> [!WARNING]
 Your browser will warn you that the domain is dangerous because the keys are self signed. Since the keys were signed by us there is no problem. Click advanced and proceed to the webpage

#### SQLite
The server is built to use sqlite3, the sqlite3 amalgum is provided and built alongside the server

## How to set up the database (schema + seed data)

This database works by scraping meals from AmericasTestKitchen using the recipe-scrapers python package. The included import script optionally scrapes the website but that takes about 7 - 8 minutes so this is an optional flag to refresh the data.

```sh
# cd into scripts folder
cd scripts

# install required python packages
pip install -r requirements

# run import script
python ./import.py
# run if you want to re-scrape the data (TAKES ABOUT 8 MINUTES TO A LONG TIME)
python ./import.py --scrape
```

#### Build
The server uses c++ with the httplib header library, so it needs to be built to run. It is important to include the ssl keys generated earlier in the build directory for this to work. The build also includes the building of sqlite so it might take longer than usual.
```sh
# from root
cd server
cmake -B build ..
cd build
make
```
> [!WARNING]
 Make sure to put **cert.pem** and **key.pem** generated earlier in this build folder


## Default test users (if applicable)

All default test users and data are included in the import script.
