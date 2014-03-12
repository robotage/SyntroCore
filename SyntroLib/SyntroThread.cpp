//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroLib
//
//  SyntroLib is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroLib is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with SyntroLib.  If not, see <http://www.gnu.org/licenses/>.
//

#include "SyntroThread.h"
#include "LogWrapper.h"

/*!
    \class SyntroThread
    \brief SyntroThread is a lightweight wrapper for threading.
	\inmodule SyntroLib
 
	SyntroThread is a thin wrapper layer that simplifies the use of QThread. It provides a 
	Windows-style message passing queue and associated functions. The 
	normal sequence of events is to create a SyntroThread, perform any external 
	initialization that might be required and then to call resumeThread(). 
	To kill the thread, call emit finished().

	SyntroThread would normally be subclassed to provide customized thread functionality. Apart from 
	standard Qt type functions, initThread() can be used to perform initialization and finishThread()
	can be used to perform clean up before the thread is destroyed.
*/

/*!
	\internal
*/

SyntroThreadMsg::SyntroThreadMsg(QEvent::Type nEvent) : QEvent(nEvent)
{
}

/*!
	This is the constructor for SyntroThread. It may be called with \a threadName, a 
	name for the thread – this can be useful for debugging.
*/

SyntroThread::SyntroThread(const QString& threadName, const QString& logTag)
{
	m_name = threadName;
	m_logTag = logTag;
	m_event = QEvent::registerEventType();					// get an event number
}

/*!
	\internal
*/

SyntroThread::~SyntroThread()
{
}

/*!
	Calling the constructor for SyntroThread doesn’t actually start the thread 
	running to allow for the caller to perform any necessary initialization.
	resumeThread() should be called when everything is ready for the thread to initialize and run. 
	resumeThread() calls initThread() to get the thread started.
*/

void SyntroThread::resumeThread()
{
	m_thread = new InternalThread;

	moveToThread(m_thread);

	connect(m_thread, SIGNAL(started()), this, SLOT(internalRunLoop()));

	connect(this, SIGNAL(internalEndThread()), this, SLOT(cleanup()));

	connect(this, SIGNAL(internalKillThread()), m_thread, SLOT(quit()));

	connect(m_thread, SIGNAL(finished()), m_thread, SLOT(deleteLater()));

	connect(m_thread, SIGNAL(finished()), this, SLOT(deleteLater()));

	installEventFilter(this);

	m_thread->start();
}

/*!
	cleanup receives the finished signal, calls finishing() and then emits quit to kill everything.
*/

void SyntroThread::cleanup()
{
	finishThread();
	emit internalKillThread();
}

/*!
	This function should be called to terminate and delete the thread. It emits the endThread() signal
	that initiates the close down and deletion.
*/

void SyntroThread::exitThread()
{
	emit internalEndThread();
}

/*!
	\internal
*/

void SyntroThread::internalRunLoop()
{
	initThread();
	emit running();
}


/*!
	intiThread() is called as part of the resumeThread() processing and should be used for 
	any initialization that the thread needs to perform prior to normal execution.
*/

void SyntroThread::initThread()
{
}

/*!
	processMessage is called when there is a message to be processed on the thread’s message queue. 
	\a msg contains the message. This function should be overridden in order to process the messages. 
	The function should return true if the messages was processed by the function and no further 
	processing should be performed on this message, false if not.
*/

bool SyntroThread::processMessage(SyntroThreadMsg* msg)
{
	logDebug(QString("Message on default PTM - %1").arg(msg->type()));
	return true;
}

/*!
	This function can be called to place a message for thread on its queue. A message consists of three values:

	\list
	\li \a message. This is a code that identifies the message. See SyntroThread.h for examples.
	\li \a intParam. This is an integer parameter that is passed with the message.
	\li \a ptrParam. This is a pointer parameter that is passed with the message.
	\endlist

	Control is returned to the caller immediately as this function just queues the message for later processing by the thread.
*/

void SyntroThread::postThreadMessage(int message, int intParam, void *ptrParam)
{
	SyntroThreadMsg *msg = new SyntroThreadMsg((QEvent::Type)m_event);
	msg->message = message;
	msg->intParam = intParam;
	msg->ptrParam = ptrParam;
	qApp->postEvent(this, msg);
}

/*!
	\internal
*/

bool SyntroThread::eventFilter(QObject *obj, QEvent *event)
 {
	 if (event->type() == m_event) {
		processMessage((SyntroThreadMsg *)event);
		return true;
	}

	//	Just do default processing 
    return QObject::eventFilter(obj, event);
 }
