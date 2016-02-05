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

#include <QCoreApplication>
#include <QtCore>
#include <iostream>
#include "reverse_tunnel.h"

#ifndef HOST
#define HOST "test.com"
#endif
#ifndef USER
#define USER "user"
#endif
#ifndef PASSWD
#define PASSWD "passwd"
#endif
#ifndef PORT
#define PORT "22"
#endif

QString leerIdentificador(QString device);

int main(int argc, char *argv[])
{
	QCoreApplication a(argc, argv);

    //codificación de cadenas
    QTextCodec *linuxCodec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(linuxCodec);

    ReverseTunnel * periodic_tunnel = ReverseTunnel::instance();

    periodic_tunnel->setConnectionData(HOST,USER,PASSWD,PORT);

    periodic_tunnel->setHardwareId(leerIdentificador("/dev/sda"));
    periodic_tunnel->setStatus("Hello!");

    if (periodic_tunnel->standaloneConnection(240))
    {
      qStdOut() << "Tunel abierto en screen \"reverse_tunnel\"; activo durante 4 minutos.\n\n"
    			"CONECTESE LO ANTES POSIBLE. La sesión de screen no terminará hasta que se cierre manualmente o se apague el equipo.";
    }
    else
    	qCritical() << "No se pudo abrir el tunel.";

    return 0;

//    periodic_tunnel->start(600, 300);

//    std::vector<QTime> connection_times;
//    connection_times.push_back(QTime(15, 55));
//    connection_times.push_back(QTime(16, 46));
//    periodic_tunnel->start(connection_times, 60);

    return a.exec();
}

QString leerIdentificador(QString device)
{
	QString id;

	QProcess proceso;

	//Ejecucion del proceso hwinfo
	QString command = "hwinfo --disk --only %1";
	command = command.arg(device);
	proceso.start(command);

	if(!proceso.waitForStarted() || !proceso.waitForFinished())
	{
		proceso.close();
		return "";
	}

	QString salida = proceso.readAllStandardOutput();
	proceso.close();

	//Filtrar la salida para obtener las lineas significantes
	QStringList lineas_info;
	QStringList lineas = salida.split('\n');

	QRegExp re_info("^\\s*(Model|Serial ID): (.*)$");

	for(int i = 0; i < lineas.length(); i++)
	{
		if(re_info.indexIn(lineas.at(i)) > -1)
			lineas_info << re_info.cap(2);
	}

	if(lineas_info.length() != 2)
	{
		qWarning() << QString("El dispositivo %1 no puede identificarse.").arg(device);
		return "";
	}

	//Develolver el identificador: Unique ID - Parent ID
	id = lineas_info.at(0) + " - " + lineas_info.at(1);

	return id;
}
