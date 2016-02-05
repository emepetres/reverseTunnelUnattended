/*******************************************************************************
 The MIT License (MIT)

 Copyright (c) 2016 Javier Carnero

 This file is part of ReverseTunnelUnattended.

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of DunaWatchdog and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 *******************************************************************************/

#include <sys/ioctl.h>
#include <linux/hdreg.h>
#include <fcntl.h>
//#include <string.h>
//#include <stdio.h>
#include <unistd.h>

#include "reverse_tunnel.h"

#define MSEC_PER_DAY 86400000

bool ReverseTunnel::instanciated = false;
ReverseTunnel * ReverseTunnel::singleton = 0;

ReverseTunnel * ReverseTunnel::instance() {
  if (!instanciated) {
    singleton = new ReverseTunnel();
    instanciated = true;
  }

  return singleton;
}

ReverseTunnel::ReverseTunnel()
    : started(false),
      running(false),
      active_time(0) {
  connection = new QProcess(this);
  send_message = new QProcess(this);
  timer = new QTimer(this);
  status_message = "";

  ////////escribimos la clave publica del servidor en la maquina, por si es la primera vez que nos conectamos///////////
  args << QDir::homePath() + "/.ssh/known_hosts";
  connection->start("rm", args);  //borramos archivo known_hosts
  connection->waitForFinished();
  args.clear();

  QString known_hosts = QDir::homePath() + "/.ssh/known_hosts";
  QString command = "ssh-keyscan -H -p " + port + " " + host + " >> "
      + known_hosts;
  system(command.toUtf8().data());  //escribimos un nuevo archivo known_hosts con la clave publica del host
  qStdOut() << "Archivo de servidores conocidos reescrito.";
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

  //obtenemos nombre de la maquina e identificador
  char hostname[1024];
  hostname[1023] = '\0';
  gethostname(hostname, 1023);
  name = hostname;
  QString _id = readHDDId("/dev/sda");
  if (_id == "")
    ReverseTunnel::id = _id;
  else
    ReverseTunnel::id = "no_id";

  ////caja de conexiones//////////////////
  connect(timer, SIGNAL(timeout()), this, SLOT(makeConnection()));
connect(connection, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(setFinished(int, QProcess::ExitStatus)));
}

void ReverseTunnel::setConnectionData(QString host, QString user,
                                      QString passwd, QString port = "22") {
this->host = host;
this->user = user;
this->pass = passwd;
this->port = port;
}

ReverseTunnel::~ReverseTunnel() {
instanciated = false;

timer->stop();
connection->terminate();
connection->close();
send_message->terminate();
send_message->close();
delete connection;
delete send_message;
delete timer;
}

void ReverseTunnel::createCommand(QString message, int active_time) {
//sshpass -p passwd ssh -R 0.0.0.0:0:localhost:22 user@host -p port "./dolog.sh name id [message]; sleep active_time"
QString host_data = QString(user) + QString("@") + QString(host);
program = QString("sshpass");

args.clear();
args << "-p" << pass;
args << "ssh" << "-R" << "0.0.0.0:0:localhost:22" << host_data << "-p" << port;

args << "./dolog.sh" << name << id;
if (message != "")
  args << "\"\t" + message.remove('\"') + "\"";

if (active_time > 0) {
  args << ";";
  args << "sleep" << QString::number(active_time);
}

//log->debug("Comando de tunel inverso: %1 %2", program, args.join(" "));
}

bool ReverseTunnel::start(int reconnection_time, int _active_time) {
if (!started) {
  active_time = _active_time;
  timer->setSingleShot(false);

  timer->start(reconnection_time * 1000);
  started = true;
  return true;
} else {
  qCritical()
      << "El tunel inverso está iniciado, no se puede volver a iniciar.";
  return false;
}
}

bool ReverseTunnel::start(std::vector<QTime> _connection_times,
                          int _active_time) {
if (!started) {
  active_time = _active_time;
  timer->setSingleShot(true);

  //obtenemos los milisegundos a la hora mas cercana
  connection_times = _connection_times;

  timer->start(calculeNextConnectionTime());
  started = true;
  return true;
} else {
  qCritical()
      << "El tunel inverso está iniciado, no se puede volver a iniciar.";
  return false;
}
}

bool ReverseTunnel::sendMessage(const QString& message) {
command_mutex.lock();
createCommand(message);
send_message->start(program, args);
command_mutex.unlock();

send_message->waitForFinished();

qStdOut() << "Mensaje enviado mediante tunel ssh.";
return true;
}

bool ReverseTunnel::stop() {
if (started) {
  timer->stop();
  started = false;
  return true;
} else {
  return false;
}
}

bool ReverseTunnel::standaloneConnection(int active_time) {
if (!started) {
  timer->setSingleShot(false);
  disconnect(connection, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(setFinished(int, QProcess::ExitStatus)));

  //primero nos aseguramos que no hay otro screen "reverse_tunnel" abierto
  QStringList screen_args;
  screen_args << "-S" << "reverse_tunnel" << "-X" << "quit";
  connection->execute("screen", screen_args);
  //iniciamos el screen
  screen_args.clear();
  screen_args << "-d" << "-m" << "-S" << "reverse_tunnel";
  connection->execute("screen", screen_args);

  connect(connection, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(setFinished(int, QProcess::ExitStatus)));

  //ejecutamos el tunel sobre el screen
  running = true;
  command_mutex.lock();
  createCommand("standalone-connection", active_time);
  screen_args.clear();  //TODO realmente se puede ejecutar el proceso cuando se crea el screen, en el comando anterior
  screen_args << "-S" << "reverse_tunnel" << "-p" << "0" << "-X" << "exec"
      << program << args;
  connection->execute("screen", screen_args);
  command_mutex.unlock();

  return true;
} else
  return false;
}

void ReverseTunnel::makeConnection() {
if (!running) {
  running = true;

  command_mutex.lock();
  createCommand(status_message, active_time);
  connection->start(program, args);
  command_mutex.unlock();

  qStdOut() << "Tunel inverso iniciado.";
}
}

void ReverseTunnel::setFinished(int exit_code,
                                QProcess::ExitStatus exit_status) {
running = false;
if (exit_code || exit_status == QProcess::CrashExit) {
  QString error;
  switch (exit_code) {
    case 1:
      error = "Invalid command line argument";
      break;
    case 2:
      error = "Conflicting arguments given";
      break;
    case 3:
      error = "General runtime error";
      break;
    case 4:
      error = "Unrecognized response from ssh (parse error)";
      break;
    case 5:
      error = "Invalid/incorrect password";
      break;
    case 6:
      error =
          "Host public key is unknown. sshpass exits without confirming the new key";
      break;
    default:
      error = "Command crashed";
      break;
  }
  qCritical() << "Tunel inverso incorrecto: " << error << ".";
} else {
  qStdOut() << "Tunel inverso finalizado.";
  if (timer->isSingleShot()) {
    timer->start(calculeNextConnectionTime());
  }
}
}

uint ReverseTunnel::calculeNextConnectionTime() {
int next_connection_time = MSEC_PER_DAY;
for (uint i = 0; i < connection_times.size(); ++i) {
  int msecs = QTime::currentTime().msecsTo(connection_times[i]);
  if (msecs < 0)
    msecs += MSEC_PER_DAY;
  if (msecs < next_connection_time) {
    next_connection_time = msecs;
  }
}

return next_connection_time;
}

void ReverseTunnel::setStatus(const QString& status) {
message_mutex.lock();
status_message = status;
message_mutex.unlock();
}

QString ReverseTunnel::readHDDId(const QString& dispositivo_hdd) {
QString id_hdd;

struct hd_driveid id;

int fd = open(dispositivo_hdd.toStdString().c_str(), O_RDONLY | O_NONBLOCK);

if (fd < 0) {
  perror(dispositivo_hdd.toStdString().c_str());
}

if (!ioctl(fd, HDIO_GET_IDENTITY, &id)) {
  id_hdd = QString((char *) id.serial_no);
  id_hdd = id_hdd.replace(QRegExp("[^\\d\\w\\-]"), "");
}

return id_hdd;
}

bool ReverseTunnel::setHardwareId(const QString& hw_id) {
if (!started) {
  id = hw_id;
  id.remove('\"').remove(' ');
  id = '\"' + id + '\"';

  return true;
} else {
  qCritical()
      << "No se puede escribir el identificador de hardware del tunel inverso después de haberlo iniciado.";
  return false;
}
}

void ReverseTunnel::sendFromSignal(const QObject* sender, const char* signal) {
connect(sender, signal, this, SLOT(sendMessage(const QString&)));
}

void ReverseTunnel::unSendFromSignal(const QObject* sender,
                                     const char* signal) {
disconnect(sender, signal, this, SLOT(sendMessage(const QString&)));
}

QTextStream& qStdOut() {
static QTextStream ts(stdout);
return ts;
}
