import socket
import time

connected=False
while (not connected):
        try:
                sock = socket.socket(socket.AF_INET,socket.SOCK_STREAM)
                sock.setsockopt(socket.IPPROTO_TCP,socket.TCP_NODELAY,1)
                sock.connect(('127.0.0.1',55555))
                connected=True
        except socket.error,(value,message):
                print message

        if not connected:
                print "reconnect"

print "tcp self connection occurs!"
print "try to run follow command : "
print "netstat -an|grep 55555"
time.sleep(1800)