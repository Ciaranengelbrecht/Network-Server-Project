import threading
import json
import random
import subprocess
import tempfile
import os
import re
import socket
import sys
import io

"""

Python program to define a QuestionBank class and initialize two independant instances for JAVA and C
Includes all relevant functions for generating and checking in either Java or C
A python QB could also be initialized from this class but is not for this proj

"""


class QuestionBank:
    def __init__(self, file_path):
        # initialize questions and language from specified json C or Java file, usually in the same directory
        with open(file_path) as f:
            self.data = json.load(f)
        self.questions = self.data['questions']
        self.language = self.data['language']


    def get_question_by_id(self, questionID):
        # retrieves question using ID, questionID's defined explicitly in README.md
        for question in self.questions:
            if str(question['id']) == str(questionID):
                return question
        return None


    def generate_sequence(self, num_questions=5):
        # generates a random sequence of 5 valid IDs
        question_ids = []
        for question in self.questions:
            question_ids.append(int(question['id']))
        return random.sample(question_ids, int(num_questions))


    def isCorrectAnswer(self, questionID, answer):
        # returns True if answer to question is correct
        question = self.get_question_by_id(questionID)
        if question is None:  # Invalid ID
            return None
        qType = question['type']
        if qType == 'mcq':  # multi-choice question
            return str(question['answer']) == str(answer)
        elif qType == 'prog_py':  # Python programming
            return self.check_code_c(answer, question['solution'])
        elif qType == 'prog_java':  # Java programming
            return self.check_code_java(answer, question['solution'])
        elif qType == 'prog_c':  # C programming
            return self.check_code_c(answer, question['solution'])
        else:  # This should never happen assuming the types are correct in the text files
            raise ValueError(f"Invalid question type: {qType}")



#---------------Java QB related functions------------------

    def run_java_code(self, java_code):
        # runs java code (given the code has proper class structure, main function etc.), returns output
        try:
            class_name = self.extract_class_name(java_code)
            if class_name is None:
                print("Invalid Java code: Could not find a class with a main method.")
                return None

            with tempfile.TemporaryDirectory() as temp_dir: # Create a temporary directory
                java_file_path = os.path.join(temp_dir, f'{class_name}.java') # Save the Java code to a file
                with open(java_file_path, 'w') as java_file:
                    java_file.write(java_code)


                compile_process = subprocess.run( # Compile the Java code using javac
                    ['javac', java_file_path], capture_output=True, text=True)
                if compile_process.returncode != 0:
                    print(f"Compilation error: {compile_process.stderr}")
                    return None

                try: # Run the Java code using java
                    run_process = subprocess.run(
                        ['java', '-cp', temp_dir, class_name], capture_output=True, text=True, timeout=2)
                    if run_process.returncode != 0:
                        print(f"Runtime error: {run_process.stderr}")
                        return None
                except subprocess.TimeoutExpired: # Output an error if the user's code takes longer that 2 secondsto compile
                    print("Java program took too long to finish. Stopping the process.")
                    return None

                return run_process.stdout #return output

        except Exception as e: # other errors
            print(f"Error: {e}")
            return None


    def extract_class_name(self, java_code):
        # extracts class name from java code (given the code has proper class structure, main function etc.)
        pattern = re.compile(r'public\s+class\s+(\w+)')
        match = pattern.search(java_code)
        if match:
            return match.group(1)
        return None


    def check_code_java(self, user_code, solution_code):
        # executes user and solution Java code, returns true if identical output
        attemptOut = self.run_java_code(clean_and_fix_code(user_code))
        solutionOut = self.run_java_code(solution_code)
        return (str(attemptOut) == str(solutionOut))



#---------------C QB related functions------------------

    def execute_c_code(self, c_code):
        # runs C code (given the code has proper structure, main function etc.), returns output
        with tempfile.NamedTemporaryFile(suffix=".c", delete=False) as c_file: # Create a temporary file to store the C code
            c_file.write(c_code.encode())
            c_file.flush()

        try: # Compile the C code using GCC
            executable = c_file.name.replace(".c", ".exe")
            compile_process = subprocess.run(
                ["gcc", c_file.name, "-o", executable], check=True)
        except subprocess.CalledProcessError:
            os.remove(c_file.name)
            return None # Return None if there is a compilation error

        try: # Execute the compiled C code and capture the output
            result = subprocess.check_output(executable, timeout=2)
            stdout = result.decode("utf-8")
        except subprocess.CalledProcessError:
            stdout = None # Return None if there is an error while executing the compiled code
        except subprocess.TimeoutExpired:
            print("C prog took too long to finish. Stopping process.")
            stdout = None

        os.remove(c_file.name) # Clean up the temporary files
        os.remove(executable)

        return stdout


    def check_code_c(self, user_code, solution_code):
        # executes user and solution C code, returns true if identical output
        cleaned = clean_and_fix_code(user_code) # execute the user_code and solution_code using the execute_c_code function
        user_output = self.execute_c_code(cleaned)
        solution_output = self.execute_c_code(solution_code)

        return str(user_output) == str(solution_output) # compare the outputs and return True if they are identical, otherwise return False



#---------------Python QB related functions------------------

    def check_code_py(self, user_code, solution_code):
        # executes given and solution Python code, returns true if identical output, Python QB
        if user_code == "#":
            return False

        user_output, solution_output = "", ""
        old_stdout = sys.stdout # Save the current stdout

        try: # for running user's code
            sys.stdout = io.StringIO() # Redirect stdout to a string buffer
            exec(user_code) # Try to execute the user code, potential security issue
            user_output = sys.stdout.getvalue() # Capture the output
        except Exception as e:
            print(f"An error occurred while executing the students code: {e}", file=old_stdout) # student's code failed to be compiled
            return False
        finally:
            sys.stdout = old_stdout # Restore stdout

        try: # for running solution python code
            sys.stdout = io.StringIO() # Redirect stdout to a string buffer
            exec(solution_code) # Try to execute the solution code, potential security issue
            solution_output = sys.stdout.getvalue() # Capture the output
        except Exception as e:
            print(f"An error occurred while executing the solution code: {e}", file=old_stdout) # solution's code failed to be compiled, shouldn't occur
            return False
        finally:
            sys.stdout = old_stdout # Restore stdout

        return user_output == solution_output # Compare the outputs




#---------------Non class functions for initializing and TM comms------------------


def clean_and_fix_code(code):
    # Replace <br/> with \n
    cleaned = code.replace('<br/>', '\n')
    pattern = r'(#include[^\n]*\n?)' # Ensure newline after include statements
    cleaned_and_fixed = re.sub(pattern, r'\1\n', cleaned)

    return cleaned_and_fixed


def handle_request_questions(qb, message):
    # receives request_question msg from TM, sends question
    questionSequence = message['questionsequence'] # 1. Extract necessary information from the message, e.g., question type, language (this should be an array of ints)

    questions = []
    for qid in questionSequence:
        question = qb.get_question_by_id(qid)
        if question is not None:
            questions.append(question)

    response = {
        "type": "questions",
        "questions": questions
    }

    response_json = json.dumps(response)

    return response_json


def handle_request_random_sequence(qb, message):
    # receives request_random_sequence msg from TM, sends random sequence of IDs
    num_questions = int(message['num_questions'])
    sequence = qb.generate_sequence(num_questions)

    response = { # Create the response message
        "type": "random_sequence",
        "sequence": sequence
    }

    response_json = json.dumps(response) # Convert the response to JSON format

    return response_json


def handle_submit_answer(qb, message):
    #handling communication with TM
    questionID = message['questionID'] # 1. Extract necessary information from the message, e.g., question ID, submitted answer
    answer = message['answer']

    # 2. Evaluate the submitted answer
    evaluation = qb.isCorrectAnswer(questionID, answer)
    if evaluation is None:  # just means the questionID is meant for a different QB which is fine
        return json.dumps({'error': 'Invalid qID'})

    # 3. Create a JSON message with the evaluation result (correct or incorrect)
    answer_evaluation_data = {
        "type": "answer_evaluation",
        "questionID": int(questionID),
        "isCorrect": evaluation
    }

    response_json = json.dumps(answer_evaluation_data) # Convert the response to JSON format

    return response_json


def handle_request_answer(qb, message):
    #handling communication with TM
    questionID = message['questionID']
    question = qb.get_question_by_id(questionID)

    if question is None:
        return json.dumps({'error': 'Invalid question ID'})

    answer_data = {
        "type": "answer",
        "questionID": questionID,
        "answer": str(question['answer']) if question['type'] == "mcq" else str(question['output'])
    }

    response_json = json.dumps(answer_data) # Convert the response to JSON format

    return response_json


# Making instances
java_qb = QuestionBank('questions_java.txt')
c_qb = QuestionBank('questions_c.txt')
#py_qb = QuestionBank('questions_py.txt')


def get_local_ip():
    #retrieves local IP
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    try:
        s.connect(('192.255.255.255', 1))
        IP = s.getsockname()[0]
    except:
        IP = '127.0.0.1'
    finally:
        s.close()
    return IP



local_ip = get_local_ip()
print(f'ensure config.txt states: {local_ip}') #reminder to change ip


def start_server(qb, host=local_ip, port=3002):
    #Start the QB server
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind((host, port))
        s.listen()
        print(f'{qb.language} QB listening on {host}:{port}...')
        while True:
            conn, addr = s.accept()
            with conn:
                print(f'Connected by {addr}')
                while True:
                    data = conn.recv(1024)
                    if not data:
                        break

                    try:
                        request = data.decode('utf-8')
                    except UnicodeDecodeError:
                        request = data.decode('latin-1')

                    if "request_questions" in request:
                        try:
                            request_dict = json.loads(request)
                            response = handle_request_questions(qb, request_dict)
                            print(f'Response sent: {response}')
                            conn.sendall(response.encode())
                        except json.JSONDecodeError:
                            conn.sendall(json.dumps({'error': 'Invalid JSON format'}).encode())
                    elif "request_random_sequence" in request:
                        try:
                            request_dict = json.loads(request)
                            response = handle_request_random_sequence(qb, request_dict)
                            print(f'Response sent: {response}')  # Print the response
                            conn.sendall(response.encode())
                        except json.JSONDecodeError:
                            conn.sendall(json.dumps({'error': 'Invalid JSON format'}).encode())
                    elif "submit_answer" in request:
                        print("RAW REQUEST:", request)
                        try:
                            request_dict = json.loads(request)
                            response = handle_submit_answer(qb, request_dict)
                            print(f'Response sent: {response}')  # Print the response
                            conn.sendall(response.encode())
                        except json.JSONDecodeError:
                            conn.sendall(json.dumps({'error': 'Invalid JSON format'}).encode())
                    elif "request_answer" in request:
                        try:
                            request_dict = json.loads(request)
                            response = handle_request_answer(qb, request_dict)
                            print(f'Response sent: {response}')  # Print the response
                            conn.sendall(response.encode())
                        except json.JSONDecodeError:
                            conn.sendall(json.dumps({'error': 'Invalid JSON format'}).encode())
                    else:
                        conn.sendall(json.dumps({'error': 'Invalid question ID'}).encode())

                    print("Received request:")
                    print(request)  # Print the received request (question ID) for debugging



#TM uses C and JAVA qb's, threading to allow seperate execution of the two QB's
threading.Thread(target=start_server, args=(c_qb, local_ip, 3002)).start()
threading.Thread(target=start_server, args=(java_qb, local_ip, 30022)).start()
