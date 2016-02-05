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

#ifndef REVERSE_TUNNEL_H_
#define REVERSE_TUNNEL_H_

#include <QtCore>

QTextStream& qStdOut();

/** Crea un tunel SSH inverso con un servidor remoto, que está activo durante un tiempo. Esta conexión se vuelve
 * a abrir periodicamente. En el servidor debe existir un script llamado dolog.sh que debe poder ser ejecutado por
 * el usuario del tunel sin problemas.
 */
class ReverseTunnel : public QObject {
  Q_OBJECT
 public:
  static ReverseTunnel * instance();
  virtual ~ReverseTunnel();

  void setConnectionData(QString host, QString user, QString passwd,
                         QString port);
  bool setHardwareId(const QString& hw_id);

  void sendFromSignal(const QObject* receiver, const char* member);
  void unSendFromSignal(const QObject* receiver, const char* member);

public slots:
  bool start(int reconnection_time, int active_time);  //!< Inicia la ejecución periódica del tunel. Puede ejecutarse directamente o como SLOT. No inicia si hay una conexión anterior abierta.
  bool start(std::vector<QTime> connection_times, int active_time);
  bool stop();  //!< Para la ejecución periódica del tunel. Puede ejecutarse directamente o como SLOT.
  void setStatus(const QString& status);
  bool standaloneConnection(int active_time);  //!< Para la ejecución de un sólo tunel inverso.

 protected:
  ReverseTunnel();

private slots:
  bool sendMessage(const QString& message);
  void makeConnection();  //!< Realiza el tunel ssh con el servidor. Esta no se realiza si existe una conexión anterior abierta.
  void setFinished(int exit_code, QProcess::ExitStatus exit_status);  //!< Gestiona la salida de cada conexión ssh.

 private:
  static ReverseTunnel * singleton;  //!< Variable para asegurarse una sóla instancia del tunel inverso (singleton)
  static bool instanciated;

  //parametros de conexion
  QString user;
  QString pass;
  QString host;
  QString port;
  QString name;
  QString id;

  //objetos
  QProcess * connection, *send_message;
  QTimer * timer;

  QString program;
  QStringList args;

  bool started;
  bool running;
  QMutex command_mutex;

  QMutex message_mutex;
  QString status_message;

  int active_time;
  //para cuando estamos conectando a horas concretas
  std::vector<QTime> connection_times;

  void createCommand(QString message, int active_time = 0);
  uint calculeNextConnectionTime();
  QString readHDDId(const QString& dispositivo_hdd);
};

#endif /* REVERSE_TUNNEL_H_ */
