Executing C Test Manager:

After compiling with make and after the QB is already running

Go to config.txt in the tm directory and enter you're local ip (printed by qb)
Inside config.txt the user's local ip must be in the correct format with no quotations ie: QB_SERVER_ADDRESS = 172.21.120.126

Inside the tm directory start the TM with: ./TM

After seeing that the qb server has successfully connected to the TM, inside a browser go to: http://localhost:8080/login.html

Login with any credentials saved inside the credentials.txt file
