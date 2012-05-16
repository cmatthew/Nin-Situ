import SocketServer
import socket
import string
class MyTCPHandler(SocketServer.BaseRequestHandler):
    """
    The RequestHandler class for our server.

    It is instantiated once per connection to the server, and must
    override the handle() method to implement communication to the
    client.
    """

    def handle(self):
        # self.request is the TCP socket connected to the client
        self.data = self.request.recv(1024).strip()
        try:
            print "{} wrote:".format(self.client_address[0])
        except:
            pass
        print self.data
        try:
            print "Data:", self.data
            port = int(self.data.strip()[:-1])
        except Exception, e:
            print e
            return
        s = socket.socket(
            socket.AF_INET, socket.SOCK_STREAM)
        tup = (self.client_address[0], port)
        print tup
        s.connect(tup)
        

        # just send back the same data
        self.request.sendall(self.data)

if __name__ == "__main__":
    
    HOST, PORT = "", 9999
    print "Starting Sever on", HOST, ":", PORT
    # Create the server, binding to localhost on port 9999
    server = SocketServer.TCPServer((HOST, PORT), MyTCPHandler)

    # Activate the server; this will keep running until you
    # interrupt the program with Ctrl-C
    server.serve_forever()
