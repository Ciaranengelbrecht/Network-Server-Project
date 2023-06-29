import socket

"""

Creates a basic TCP server that listens on localhost port 3002 for incoming connections from the TM.
It creates a socket object and binds it to a specified address and port
Server continuously waits for clients to connect, receives data from them, sends a response back, and then closes the connection

"""


HOST = '127.0.0.2'  # Standard loopback interface address (localhost)
PORT = 3002        # Port to listen on (non-privileged ports are > 1023)

# Create a socket object
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s: # with statement ensures the socket is properly closed after use
    # Bind the socket to a specific address and port
    s.bind((HOST, PORT))

    # Listen for incoming connections
    s.listen()

    print(f'Server listening on {HOST}:{PORT}')

    while True:
        # Wait for a client to connect
        conn, addr = s.accept()
        print(f'Client connected: {addr}')

        # Receive data from the client
        data = conn.recv(1024)
        print(f'Received data: {data.decode()}')

        # Send a response back to the client
        response = 'Hello, client!'
        conn.sendall(response.encode())

        # Close the connection with the client
        conn.close()
