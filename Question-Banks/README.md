# Executing Python QB Instructions

The PY QB must be started before the TM

In the qb directory do: python3 qb.py



# Question Bank/s Info

# qb.py and json files
qb.py contains class definitions and functions for creating QB instances
The 2 QB's questions are taken from the JSON questions_java.txt and questions_c.txt files
Each text file contains 10 q's 5 MC q's and 5 programming/written answer q's which are randomly chosen

Each question is referenced by a unique questionID within the QB class functions as follows:
USES QID's 11-30 inclusive, NO PYTHON QUESTIONS ONLY JAVA AND C
Question ID's for each language:

Q ID: 0-9 = python questions (1-5 MC, 6-10 written) (inclusive)

Q ID 11-20 = Java q's (11-15 MC, 16-20 written)

Q ID 21-30 = C q's (21-25 MC, 26-30 written)


# qbserver.py
qbserver.py creates a basic TCP server that listens on localhost port 3002 for incoming connections from the TM.
It creates a socket object and binds it to a specified address and port
Server continuously waits for clients to connect, receives data from them, sends a response back, and then closes the connection



# Citations:
  - https://realpython.com/intro-to-python-threading/
  - https://docs.python.org/3/library/socket.html
