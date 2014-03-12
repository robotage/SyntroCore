//
//  Copyright (c) 2014 Scott Ellis and Richard Barnett
//	
//  This file is part of SyntroControlLib
//
//  SyntroControlLib is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Lesser General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  SyntroControlLib is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Lesser General Public License for more details.
//
//  You should have received a copy of the GNU Lesser General Public License
//  along with SyntroControlLib.  If not, see <http://www.gnu.org/licenses/>.
//

#include "SyntroLib.h"
#include "FastUIDLookup.h"


FastUIDLookup::FastUIDLookup(void)
{
	int		i;

	for (i = 0; i < FUL_LEVEL_SIZE; i++)
		FULLevel0[i] = NULL;
}

FastUIDLookup::~FastUIDLookup(void)
{
	int indexL0, indexL1, indexL2;
	void **L1, **L2, **L3;

	for (indexL0 = 0; indexL0 < FUL_LEVEL_SIZE; indexL0++) {
		if (FULLevel0[indexL0] != NULL) {
			L1 = (void **)FULLevel0[indexL0];
			for (indexL1 = 0; indexL1 < FUL_LEVEL_SIZE; indexL1++) {
				if (L1[indexL1] != NULL) {
					L2 = (void **)L1[indexL1];
					for (indexL2 = 0; indexL2 < FUL_LEVEL_SIZE; indexL2++) {
						if (L2[indexL2] != NULL) {
							L3 = (void **)L2[indexL2];
							free(L3);
						}
					}
					free(L2);
				}
			}
			free(L1);
		}
	}
}

void *FastUIDLookup::FULLookup(SYNTRO_UID *UID)
{
	unsigned int indexL0, indexL1, indexL2, indexL3;
	void **ptr;
	SYNTRO_UC2 *U2;
	void *returnValue;

	U2 = (SYNTRO_UC2 *)UID;
	indexL0 = SyntroUtils::convertUC2ToUInt(U2[0]);						// get UID as four separate unsigned ints
	indexL1 = SyntroUtils::convertUC2ToUInt(U2[1]);
	indexL2 = SyntroUtils::convertUC2ToUInt(U2[2]);
	indexL3 = SyntroUtils::convertUC2ToUInt(U2[3]);

	QMutexLocker locker (&m_lock);

//	Do level 0 lookup

	ptr = (void **)FULLevel0[indexL0];
	if (ptr == NULL)
		return NULL;

//	Do level 1 lookup

	ptr = (void **)ptr[indexL1];
	if (ptr == NULL)
		return NULL;

//	Do level 2 lookup

	ptr = (void **)ptr[indexL2];
	if (ptr == NULL)
		return NULL;

//	Do level 3 lookup

	returnValue =  ptr[indexL3];
	return returnValue;
}

void	FastUIDLookup::FULAdd(SYNTRO_UID *UID, void *data)
{
	unsigned int indexL0, indexL1, indexL2, indexL3;
	void **L1, **L2, **L3, *ptr;
	SYNTRO_UC2 *U2;

	U2 = (SYNTRO_UC2 *)UID;
	indexL0 = SyntroUtils::convertUC2ToUInt(U2[0]);						// get UID as four separate unsigned ints
	indexL1 = SyntroUtils::convertUC2ToUInt(U2[1]);
	indexL2 = SyntroUtils::convertUC2ToUInt(U2[2]);
	indexL3 = SyntroUtils::convertUC2ToUInt(U2[3]);

	if ((ptr = FULLookup(UID)) != NULL)
		FULDelete(UID);

	QMutexLocker locker(&m_lock);

	if ((L1 = (void **)FULLevel0[indexL0]) == NULL)
	{														// need to add a level 1 array
		FULLevel0[indexL0] = (void *)calloc(FUL_LEVEL_SIZE, sizeof (void *));
		L1 = (void **)FULLevel0[indexL0];
	}
	if ((L2 = (void **)L1[indexL1]) == NULL)
	{														// need to add a level 2 array
		L1[indexL1] = (void *)calloc(FUL_LEVEL_SIZE, sizeof(void *));
		L2 = (void **)L1[indexL1];
	}	
	if ((L3 = (void **)L2[indexL2]) == NULL)
	{														// need to add a level 2 array
		L2[indexL2] = (void *)calloc(FUL_LEVEL_SIZE, sizeof(void *));
		L3 = (void **)L2[indexL2];
	}	
	L3[indexL3] = data;										// finally, record the data
}

void FastUIDLookup::FULDelete(SYNTRO_UID *UID)
{
	unsigned int indexL0, indexL1, indexL2, indexL3;
	void **L1, **L2, **L3;

	SYNTRO_UC2 *U2;

	U2 = (SYNTRO_UC2 *)UID;
	indexL0 = SyntroUtils::convertUC2ToUInt(U2[0]);	// get UID as four separate unsigned ints
	indexL1 = SyntroUtils::convertUC2ToUInt(U2[1]);
	indexL2 = SyntroUtils::convertUC2ToUInt(U2[2]);
	indexL3 = SyntroUtils::convertUC2ToUInt(U2[3]);

	QMutexLocker locker(&m_lock);

//	Do level 0 lookup

	L1 = (void **)FULLevel0[indexL0];
	if (L1 == NULL)
		return;
//	Do level 1 lookup

	L2 = (void **)L1[indexL1];
	if (L2 == NULL)
		return;

//	Do level 2 lookup

	L3 = (void **)L2[indexL2];
	if (L3 == NULL)
		return;

//	Clear data pointer

	L3[indexL3] = NULL;
}

