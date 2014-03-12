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

#ifndef _SYNTROLINK_H_
#define _SYNTROLINK_H_

//	The internal version of SYNTROMESSAGE

class SYNTROLIB_EXPORT SyntroMessageWrapper
{
public:
	SyntroMessageWrapper(void);
	~SyntroMessageWrapper(void);

	SYNTRO_MESSAGE *m_msg;									// message buffer pointer
	int m_len;												// total length
	SyntroMessageWrapper *m_next;							// pointer to next in chain
	int m_bytesLeft;										// bytes left to be received or sent
	unsigned char *m_ptr;									// pointer in m_pMsg while receiving or transmitting

//	for receive

	int m_cmd;												// the current command
};


//	The SyntroLink class itself

class SYNTROLIB_EXPORT SyntroLink
{
public:
	SyntroLink(const QString& logTag);
	~SyntroLink(void);

	void send(int cmd, int len, int priority, SYNTRO_MESSAGE *syntroMessage);
	bool receive(int priority, int *cmd, int *len, SYNTRO_MESSAGE **syntroMessage);

	int tryReceiving(SyntroSocket *sock);
	int trySending(SyntroSocket *sock);

protected:
	void clearTXQueue();
	void clearRXQueue();
	void resetReceive(int priority);
	void flushReceive(SyntroSocket *sock);
	SyntroMessageWrapper *getTXHead(int priority);
	SyntroMessageWrapper *getRXHead(int priority);
	void addToTXQueue(SyntroMessageWrapper *wrapper, int nPri);
	void addToRXQueue(SyntroMessageWrapper *wrapper, int nPri);
	void computeChecksum(SYNTRO_MESSAGE *syntroMessage);
	bool checkChecksum(SYNTRO_MESSAGE *syntroMessage);

	SyntroMessageWrapper *m_TXHead[SYNTROLINK_PRIORITIES];	// head of transmit list
	SyntroMessageWrapper *m_TXTail[SYNTROLINK_PRIORITIES];	// tail of transmit list
	SyntroMessageWrapper *m_RXHead[SYNTROLINK_PRIORITIES];	// head of receive list
	SyntroMessageWrapper *m_RXTail[SYNTROLINK_PRIORITIES];	// tail of receive list

	SyntroMessageWrapper *m_TXIP[SYNTROLINK_PRIORITIES];	// in progress TX object
	SyntroMessageWrapper *m_RXIP[SYNTROLINK_PRIORITIES];	// in progress RX object
	bool m_RXSM;											// true if waiting for SYNTROMESSAGE header
	unsigned char *m_RXIPMsgPtr;							// pointer into message header
	int m_RXIPBytesLeft;
	SYNTRO_MESSAGE m_syntroMessage;							// for receive
	int m_RXIPPriority;										// the current priority being received

	QMutex m_RXLock;
	QMutex m_TXLock;

	QString m_logTag;
};

#endif // _SYNTROLINK_H_

