Reverse Tunnel Unattended
=====

Brief description
-----------
SSH reverse tunnel on a free port from a device (*box* from now on) to a remote host (*server* from now on). On this server you can see through a file the last reverse tunnel of each connected *box* and its port. Then you can connect to the *box* from this *server*, or from another computer using the *server* as a bridge.

Install server
-----------
* We will use an user with ssh connection. On the *server*, copy **dolog.sh** to the home directory.
* Configure Gateway ports:
```
# GatewayPorts clientspecified >> /etc/ssh/sshd_config
```

Install box
-----------
* You can build it to work as a standalone connection, or a periodic connection. See *main.cpp*
* Edit **config.pri** with the *server* connection data.

```
$ qmake
$ make install
```

* Copy portable/reverse to the place where is going to be executed
* Install sshpass
* Enable reading permissions over '/dev/sda' for the user that is going to execute the executable.
* Execute using cron as you like (standalone connection every X minutes, or the periodic executable for example)

How to use it
-----------
* Connect to the *server*. Inside the user home you will see a file with this name: *HOST-HDD_ID.log*. At the end of this file you will see the last reverse tunnel and its PORT. The tunnel will be open for about four minutes.
* From any computer, connect to *box* using the last port you see in the *server* log file as follows:
```
$ ssh [box_user]@*server* -p PORT
```

TODO
-----------
* Use public/private key instead of password
