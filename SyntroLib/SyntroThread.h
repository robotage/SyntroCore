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

#ifndef		_SYNTROTHREAD_H_
#define		_SYNTROTHREAD_H_

#include "SyntroUtils.h"

//	Inter-thread message defs

//	Endpoint socket messages

#define	ENDPOINT_ONCONNECT_MESSAGE		(1)					// Endpoint socket messages
#define	ENDPOINT_ONCLOSE_MESSAGE		(2)	
#define	ENDPOINT_ONRECEIVE_MESSAGE		(3)	
#define	ENDPOINT_ONSEND_MESSAGE			(4)	

#define	ENDPOINT_MESSAGE_START			ENDPOINT_ONCONNECT_MESSAGE	// start of endpoint message range
#define	ENDPOINT_MESSAGE_END			ENDPOINT_ONSEND_MESSAGE		// end of endpoint message range

//#define	SYNTROTHREAD_TIMER_MESSAGE		(5)					// message used by the SyntroThread timer
#define	HELLO_ONRECEIVE_MESSAGE			(6)					// used for received hello messages
#define	HELLO_STATUS_CHANGE_MESSAGE		(7)					// send to owner when hello status changes for a device

//	This code is the first that can be used by any specific application

#define	SYNTRO_MSTART					(8)					// This is where application specific codes start

//	The SyntroThreadMsg data structure - this is what is passed in the task's message queue

class SYNTROLIB_EXPORT SyntroThreadMsg : public QEvent
{
public:
	SyntroThreadMsg(QEvent::Type nEvent);
	int		message;
	int		intParam;
	void	*ptrParam;
};

class InternalThread : public QThread
{
	Q_OBJECT

public:
	inline void msleep(unsigned long msecs) { QThread::msleep(msecs); }
};

class SYNTROLIB_EXPORT SyntroThread : public QObject
{
	Q_OBJECT

public:
	SyntroThread(const QString& threadName, const QString& logTag);
	virtual ~SyntroThread();

	virtual void postThreadMessage(int message, int intParam, void *ptrParam);	// post a message to the thread
	virtual void resumeThread();							// this must be called to get thread going
	
	void exitThread();										// called to close thread down

	bool isRunning();										// returns true if task no exiting

	InternalThread *thread() { return m_thread; }

public slots:
	void internalRunLoop();
	void cleanup();

signals:
	void running();											// emitted when everything set up and thread active
	void internalEndThread();								// this to end thread
	void internalKillThread();								// tells the QThread to quit
	
protected:
	virtual	bool processMessage(SyntroThreadMsg* msg);
	virtual void initThread();								// called by resume thread internally
	virtual void finishThread() {}							// called just before thread is destroyed

    inline void msleep(unsigned long msecs) { thread()->msleep(msecs); }
	bool eventFilter(QObject *obj, QEvent *event);

	int m_event;											// the event used for Syntro thread message

	QString m_logTag;

private:
	QString m_name;											// the task name - for debugging mostly
	InternalThread *m_thread;								// the underlying thread
};

#endif		//_SYNTROTHREAD_H_
