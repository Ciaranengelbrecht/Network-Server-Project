# cits3002-project
The CITS3002 Computer Networks Project

# Instructions for Running
- Compile using 'make' from the top directory
- Instructions for running qb and tm are in their respective folder, the QB must be run first
- Before running the TM config.txt must contain the user's local ip in the correct format with no quotations ie: QB_SERVER_ADDRESS = 172.21.120.126   
        

# Checklist

## Project Tasks
- [x] Develop a 3-tier network-based system.
    - [x] The system must include a Test-Manager (TM), Question-Banks (QBs), and a standard web-browser.
    - [x] The TM authenticates a student's session and navigates the student through a sequence of multi-choice questions and programming challenges.
    - [x] The QBs generate a sequence of multi-choice questions and programming challenges for each student and marks students' attempts.
    - [x] Communication between the web-browser and TM will require the HTTP and HTML protocols.
    - [x] Design and implement a protocol between your TM and QBs.
 
 - [x] Implement a login system for pre-registered students.
    - [x] Students must be able to navigate through a sequence of 10 questions which are either multi-choice questions or ones setting a short programming challenge.
    - [x] Students must be able to see their progress, their mark to-date, or logout.

- [x] Implement a marking system.
    - [x] Marks should decrease with each attempt: 3 marks for first attempt, 2 marks for the second, and so on.
    - [x] After 3 incorrect attempts at any question, the TM should report the 3rd attempt's incorrect output, and the correct output.

 - [x] Ensure that the TM and QBs do not have 'understanding' of each other's functions.
    - [x] The QBs should choose questions randomly from their bank of questions or generate repeatably randomized questions.
 
 - [x] Ensure the system can support two or more students simultaneously attempting questions.

 ## Project Contraints
  - [x] TM and QB software must execute on different computers (hardware), and must not assume access to any shared (networked) files.
  - [x] TM and QB must be written in two different programming languages (selected from Java, C, and Python).
  - [x] System should execute two distinct QB instances, each supporting a different programming language.
  - [x] TM does not need to produce a fancy web-interface - basic HTML involving forms, a textarea, radio-buttons, and checkboxes will be sufficient.
  - [x] TM does not need to support full user or password management - just logging in and out.
  - [x] The system must be developed for Linux, or macOS (or a combination).
  - [x] Employ the core networking functions of your chosen programming languages and not employ 3rd-party frameworks or resources.

# Citations:
  - https://realpython.com/intro-to-python-threading/
  - https://docs.python.org/3/library/socket.html

