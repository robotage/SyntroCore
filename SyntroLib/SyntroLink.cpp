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

#include "SyntroLib.h"

//#define SYNTROLINK_TRACE

//	CSyntroMessage
//

SyntroMessageWrapper::SyntroMessageWrapper()
{
	m_next = NULL;
	m_cmd = 0;
	m_len = 0;
	m_msg = NULL;
	m_ptr = NULL;
	m_bytesLeft = 0;
}

SyntroMessageWrapper::~SyntroMessageWrapper()
{
	if (m_msg != NULL) {
		free(m_msg);
		m_msg = NULL;
	}
}

//	Public routines
//

void SyntroLink::send(int cmd, int len, int priority, SYNTRO_MESSAGE *syntroMessage)
{
	SyntroMessageWrapper *wrapper;

#ifdef SYNTROLINK_TRACE
	TRACE3("Send - cmd = %d, len = %d, priority= %d", cmd, len, priority);
#endif

	QMutexLocker locker(&m_TXLock);

	wrapper = new SyntroMessageWrapper();
	wrapper->m_len = len;
	wrapper->m_msg = syntroMessage;
	wrapper->m_ptr = (unsigned char *)syntroMessage;
	wrapper->m_bytesLeft = len;

//	set up SYNTROMESSAGE header 

	syntroMessage->cmd = cmd;
	syntroMessage->flags = priority;
	syntroMessage->spare = 0;
	SyntroUtils::convertIntToUC4(len, syntroMessage->len);
	computeChecksum(syntroMessage);

	addToTXQueue(wrapper, priority);
}

bool SyntroLink::receive(int priority, int *cmd, int *len, SYNTRO_MESSAGE **syntroMessage)
{
	SyntroMessageWrapper *wrapper;

	QMutexLocker locker(&m_RXLock);

	if ((wrapper = getRXHead(priority)) != NULL) {
		*cmd = wrapper->m_cmd;
		*len = wrapper->m_len;
		*syntroMessage = wrapper->m_msg;
		wrapper->m_msg = NULL;
#ifdef SYNTROLINK_TRACE
		TRACE3("Receive - cmd = %d, len = %d, priority = %d", *cmd, *len, priority);
#endif
		delete wrapper;
		return true;
	}
	return false;
}

int SyntroLink::tryReceiving(SyntroSocket *sock)
{
	int bytesRead;
	int len;
	SyntroMessageWrapper *wrapper;

	QMutexLocker locker(&m_RXLock);

	while (1) {
		if (m_RXSM) {										// still waiting for message header
			bytesRead = sock->sockReceive((unsigned char *)&m_syntroMessage + sizeof(SYNTRO_MESSAGE) - m_RXIPBytesLeft , m_RXIPBytesLeft);
			if (bytesRead <= 0)
				return 0;
			m_RXIPBytesLeft -= bytesRead;
			if (m_RXIPBytesLeft == 0) {						// got complete SYNTRO_MESSAGE header
				if (!checkChecksum(&m_syntroMessage)) {
					logError(QString("Incorrect header cksm"));
					flushReceive(sock);
					continue;
				}
#ifdef SYNTROLINK_TRACE
				TRACE2("Received hdr %d %d", m_syntroMessage.cmd, SyntroUtils::convertUC4ToInt(m_syntroMessage.len));
#endif
				m_RXIPPriority = m_syntroMessage.flags & SYNTROLINK_PRI;
				len = SyntroUtils::convertUC4ToInt(m_syntroMessage.len);
				if (m_RXIP[m_RXIPPriority] == NULL) {		// nothing in progress at this priority
					m_RXIP[m_RXIPPriority] = new SyntroMessageWrapper();
					m_RXIP[m_RXIPPriority]->m_cmd = m_syntroMessage.cmd;
					if (len > (int)sizeof(SYNTRO_MESSAGE)) {
						m_RXIP[m_RXIPPriority]->m_msg = (SYNTRO_MESSAGE *)malloc(len);
						m_RXIP[m_RXIPPriority]->m_ptr = (unsigned char *)m_RXIP[m_RXIPPriority]->m_msg + sizeof(SYNTRO_MESSAGE); // we've already received that
					}
				}
				wrapper = m_RXIP[m_RXIPPriority];
				wrapper->m_len = len;
				if (len == sizeof(SYNTRO_MESSAGE)) {		// no message part
					addToRXQueue(wrapper, m_RXIPPriority);
					m_RXIP[m_RXIPPriority] = NULL;
					resetReceive(m_RXIPPriority);
					continue;
				}
				else
				{											// is a message part 
					if ((m_syntroMessage.cmd < SYNTROMSG_HEARTBEAT) || (m_syntroMessage.cmd > SYNTROMSG_MAX)) {
						logError(QString("Illegal cmd %1").arg(m_syntroMessage.cmd));
						flushReceive(sock);
						resetReceive(m_RXIPPriority);
						continue;
					}
					if (len >= SYNTRO_MESSAGE_MAX) {
						logError(QString("Illegal length message cmd %1, len %2").arg(m_syntroMessage.cmd).arg(len));
						flushReceive(sock);
						resetReceive(m_RXIPPriority);
						continue;
					}
					wrapper->m_bytesLeft = len - sizeof(SYNTRO_MESSAGE);	// since we've already received that
					m_RXSM = false;
				}
			}
		} else {											// now waiting for data
			wrapper = m_RXIP[m_RXIPPriority];
			bytesRead = sock->sockReceive(wrapper->m_ptr, wrapper->m_bytesLeft);
			if (bytesRead <= 0)
				return 0;
			wrapper->m_bytesLeft -= bytesRead;
			wrapper->m_ptr += bytesRead;
			if (wrapper->m_bytesLeft == 0) {				// got complete message
				addToRXQueue(m_RXIP[m_RXIPPriority], m_RXIPPriority);
				m_RXIP[m_RXIPPriority] = NULL;
				m_RXSM = true;
				m_RXIPMsgPtr = (unsigned char *)&m_syntroMessage;
				m_RXIPBytesLeft = sizeof(SYNTRO_MESSAGE);
			}
		}
	}
}

int SyntroLink::trySending(SyntroSocket *sock)
{
	int bytesSent;
	int priority;
	SyntroMessageWrapper *wrapper;

	QMutexLocker locker(&m_TXLock);

	if (sock == NULL)
		return 0;

	priority = SYNTROLINK_HIGHPRI;

	while(1) {
		if (m_TXIP[priority] == NULL) {
			m_TXIP[priority] = getTXHead(priority);

			if (m_TXIP[priority] == NULL) {
				priority++;

				if (priority > SYNTROLINK_LOWPRI)
					return 0;								// nothing to do and no error

				continue;
			}
		}

		wrapper = m_TXIP[priority];
	
		bytesSent = sock->sockSend(wrapper->m_ptr, wrapper->m_bytesLeft);
		if (bytesSent <= 0)
			return 0;										// assume buffer full

		wrapper->m_bytesLeft -= bytesSent;
		wrapper->m_ptr += bytesSent;

		if (wrapper->m_bytesLeft == 0) {					// finished this message
			delete m_TXIP[priority];
			m_TXIP[priority] = NULL;
		}
	}
}


SyntroLink::SyntroLink(const QString& logTag)
{
	m_logTag = logTag;
	for (int i = 0; i < SYNTROLINK_PRIORITIES; i++) {
		m_TXHead[i] = NULL;
		m_TXTail[i] = NULL;
		m_RXHead[i] = NULL;
		m_RXTail[i] = NULL;
		m_RXIP[i] = NULL;
		m_TXIP[i] = NULL;
	}

	m_RXSM = true;
	m_RXIPMsgPtr = (unsigned char *)&m_syntroMessage;
	m_RXIPBytesLeft = sizeof(SYNTRO_MESSAGE);
}

SyntroLink::~SyntroLink(void)
{
	clearTXQueue();
	clearRXQueue();
}


void SyntroLink::addToTXQueue(SyntroMessageWrapper *wrapper, int priority)
{
	if (m_TXHead[priority] == NULL) {
		m_TXHead[priority] = wrapper;
		m_TXTail[priority] = wrapper;
		wrapper->m_next = NULL;
		return;
	}

	m_TXTail[priority]->m_next = wrapper;
	m_TXTail[priority] = wrapper;
	wrapper->m_next = NULL;
}


void	SyntroLink::addToRXQueue(SyntroMessageWrapper *wrapper, int priority)
{
	if (m_RXHead[priority] == NULL) {
		m_RXHead[priority] = wrapper;
		m_RXTail[priority] = wrapper;
		wrapper->m_next = NULL;
		return;
	}

	m_RXTail[priority]->m_next = wrapper;
	m_RXTail[priority] = wrapper;
	wrapper->m_next = NULL;
}


SyntroMessageWrapper *SyntroLink::getTXHead(int priority)
{
	SyntroMessageWrapper *wrapper;

	if (m_TXHead[priority] == NULL)
		return NULL;

	wrapper = m_TXHead[priority];
	m_TXHead[priority] = wrapper->m_next;
	return wrapper;
}

SyntroMessageWrapper *SyntroLink::getRXHead(int priority)
{
	SyntroMessageWrapper *wrapper;

	if (m_RXHead[priority] == NULL)
		return NULL;

	wrapper = m_RXHead[priority];
	m_RXHead[priority] = wrapper->m_next;
	return wrapper;
}


void SyntroLink::clearTXQueue()
{
	for (int i = 0; i < SYNTROLINK_PRIORITIES; i++) {
		while (m_TXHead[i] != NULL)
			delete getTXHead(i);

		if (m_TXIP[i] != NULL)
			delete m_TXIP[i];

		m_TXIP[i] = NULL;
	}
}

void SyntroLink::flushReceive(SyntroSocket *sock)
{
	unsigned char buf[1000];

	while (sock->sockReceive(buf, 1000) > 0)
		;
}

void SyntroLink::resetReceive(int priority)
{
	m_RXSM = true;
	m_RXIPMsgPtr = (unsigned char *)&m_syntroMessage;
	m_RXIPBytesLeft = sizeof(SYNTRO_MESSAGE);

	if (m_RXIP[priority] != NULL) {
		delete m_RXIP[priority];
		m_RXIP[priority] = NULL;
	}
}

void SyntroLink::clearRXQueue()
{
	for (int i = 0; i < SYNTROLINK_PRIORITIES; i++) {
		while (m_RXHead[i] != NULL)
			delete getRXHead(i);

		if (m_RXIP[i] != NULL)
			delete m_RXIP[i];

		m_RXIP[i] = NULL;
		resetReceive(i);
	}
}

void SyntroLink::computeChecksum(SYNTRO_MESSAGE *syntroMessage)
{
	unsigned char sum;
	unsigned char *array;

	sum = 0;
	array = (unsigned char *)syntroMessage;

	for (size_t i = 0; i < sizeof(SYNTRO_MESSAGE)-1; i++)
		sum += *array++;

	syntroMessage->cksm = -sum;
}

bool SyntroLink::checkChecksum(SYNTRO_MESSAGE *syntroMessage)
{
	unsigned char sum;
	unsigned char *array;

	sum = 0;
	array = (unsigned char *)syntroMessage;

	for (size_t i = 0; i < sizeof(SYNTRO_MESSAGE); i++)
		sum += *array++;

	return sum == 0;
}
