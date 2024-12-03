# Webkiosk Cached

## Architecture Imagined
<img src="media/architecture.jpeg" alt="Example Image" width="500">

## Project Status
- [v] Basic Curl and dotenv Setup Completed
- [v] Connect DB and fetch username password from DB and store responses in DB
```
Due to memory allocation issues the data is not tunnelling properly between function.
The username for example went wrong from fetch_data function in fetch_data.c to store_html_response function in db.c
```
- Webkiosk picking static files from folder and text/html from DB and sending the response
- Make deamon service to keep on updating the webkiosk data in DB
- Show last updated time on webkiosk
- HOST on webkiosk.singhropar.com

## Build Process
- Make a .env file with params 
```
USERNAME=<username>
PASSWORD=<password>
```

- Run ``` make ``` command and then ```./main```

