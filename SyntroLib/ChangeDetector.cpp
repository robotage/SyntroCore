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

// This is based in part on TonyJpegDecoder - original license information below:

/****************************************************************************
*	Author:			Dr. Tony Lin											*
*	Email:			lintong@cis.pku.edu.cn									*
*	Release Date:	Dec. 2002												*
*																			*
*	Name:			TonyJpegLib, rewritten from IJG codes					*
*	Source:			IJG v.6a JPEG LIB										*
*	Purpose��		Support real jpeg file, with readable code				*
*																			*
*	Acknowlegement:	Thanks for great IJG, and Chris Losinger				*
*																			*
*	Legal Issues:	(almost same as IJG with followings)					*
*																			*
*	1. We don't promise that this software works.							*
*	2. You can use this software for whatever you want.						*
*	You don't have to pay.													*
*	3. You may not pretend that you wrote this software. If you use it		*
*	in a program, you must acknowledge somewhere. That is, please			*
*	metion IJG, and Me, Dr. Tony Lin.										*
*																			*
*****************************************************************************/


#include "ChangeDetector.h"

#include <qmath.h>
#include <qimage.h>
#include <qbuffer.h>

#include "LogWrapper.h"

typedef enum {			/* JPEG marker codes */
  M_SOF0  = 0xc0,
  M_SOF1  = 0xc1,
  M_SOF2  = 0xc2,
  M_SOF3  = 0xc3,
  
  M_SOF5  = 0xc5,
  M_SOF6  = 0xc6,
  M_SOF7  = 0xc7,
  
  M_JPG   = 0xc8,
  M_SOF9  = 0xc9,
  M_SOF10 = 0xca,
  M_SOF11 = 0xcb,
  
  M_SOF13 = 0xcd,
  M_SOF14 = 0xce,
  M_SOF15 = 0xcf,
  
  M_DHT   = 0xc4,
  
  M_DAC   = 0xcc,
  
  M_RST0  = 0xd0,
  M_RST1  = 0xd1,
  M_RST2  = 0xd2,
  M_RST3  = 0xd3,
  M_RST4  = 0xd4,
  M_RST5  = 0xd5,
  M_RST6  = 0xd6,
  M_RST7  = 0xd7,
  
  M_SOI   = 0xd8,
  M_EOI   = 0xd9,
  M_SOS   = 0xda,
  M_DQT   = 0xdb,
  M_DNL   = 0xdc,
  M_DRI   = 0xdd,
  M_DHP   = 0xde,
  M_EXP   = 0xdf,
  
  M_APP0  = 0xe0,
  M_APP1  = 0xe1,
  M_APP2  = 0xe2,
  M_APP3  = 0xe3,
  M_APP4  = 0xe4,
  M_APP5  = 0xe5,
  M_APP6  = 0xe6,
  M_APP7  = 0xe7,
  M_APP8  = 0xe8,
  M_APP9  = 0xe9,
  M_APP10 = 0xea,
  M_APP11 = 0xeb,
  M_APP12 = 0xec,
  M_APP13 = 0xed,
  M_APP14 = 0xee,
  M_APP15 = 0xef,
  
  M_JPG0  = 0xf0,
  M_JPG13 = 0xfd,
  M_COM   = 0xfe,
  
  M_TEM   = 0x01,
  
  M_ERROR = 0x100
} JPEG_MARKER;


/*
* jpegNaturalOrder[i] is the natural-order position of the i'th
* element of zigzag order.
*
* When reading corrupted data, the Huffman decoders could attempt
* to reference an entry beyond the end of this array (if the decoded
* zero run length reaches past the end of the block).  To prevent
* wild stores without adding an inner-loop test, we put some extra
* "63"s after the real entries.  This will cause the extra coefficient
* to be stored in location 63 of the block, not somewhere random.
* The worst case would be a run-length of 15, which means we need 16
* fake entries.
*/	
    static const int jpegNaturalOrder[64+16] = {
			0,  1,  8, 16,  9,  2,  3, 10,
			17, 24, 32, 25, 18, 11,  4,  5,
			12, 19, 26, 33, 40, 48, 41, 34,
			27, 20, 13,  6,  7, 14, 21, 28,
			35, 42, 49, 56, 57, 50, 43, 36,
			29, 22, 15, 23, 30, 37, 44, 51,
			58, 59, 52, 45, 38, 31, 39, 46,
			53, 60, 61, 54, 47, 55, 62, 63,
			63, 63, 63, 63, 63, 63, 63, 63,//extra entries for safety
			63, 63, 63, 63, 63, 63, 63, 63
	};

#define INPUT_2BYTES(src)  (unsigned short)(((*src)<<8)+(*(src+1)));src+=2;
#define INPUT_BYTE(src)	(unsigned char)(*src++)


ChangeDetector::ChangeDetector( )
{
    m_dataImage = NULL;
    m_oldDataImage = NULL;
    m_dataImageSize = 0;
    m_noiseThreshold = 10;
    m_deltaThreshold = 400;
    m_initialized = false;
	m_logTag = "ChangeDetector";
}

ChangeDetector::~ChangeDetector( )
{
}

void ChangeDetector::setNoiseThreshold(int threshold)
{
    m_noiseThreshold = threshold;
}

void ChangeDetector::setDeltaThreshold(int threshold)
{
    m_deltaThreshold = threshold;
}

void ChangeDetector::setTilesToSkip(int skipTiles)
{
    m_tilesToSkip = skipTiles;
}

void ChangeDetector::setIntervalsToSkip(int skipIntervals)
{
    m_intervalsToSkip = skipIntervals;
}

void ChangeDetector::setUninitialized()
{
    m_initialized = false;
    if (m_dataImage != NULL)
        delete m_dataImage;
    if (m_oldDataImage != NULL)
        delete m_oldDataImage;

   m_dataImage = NULL;
   m_oldDataImage = NULL;
}

bool ChangeDetector::imageChanged(const QByteArray &newJpeg)
{
    int width, height, size;
    long delta;
    short *oldPtr;
    short *newPtr;
    int localDelta;

    if (!readJpgHeader((unsigned char *)newJpeg.data(), newJpeg.length(), width, height, size)) {
        logDebug("Jpeg header read failed");
        return false;
    }
    if (!decompressImage((unsigned char *)newJpeg.data() + size))
        return false;
    // now check against previous image

    delta = 0;
    newPtr = m_dataImage;
    oldPtr = m_oldDataImage;

    for (int tileIndex = 0; tileIndex < m_tileCount; tileIndex++) {
        for (int chunkIndex = 0; chunkIndex < m_blocksInMcu; chunkIndex++) {
            localDelta = 0;
            for (int offset = 0; offset < CHANGEDETECTOR_SAMPLES; offset++)
                localDelta += (long)abs(*newPtr++ - *oldPtr++);

            if (localDelta >= m_noiseThreshold)
                delta += localDelta;
        }
    }

    // scale so this doesn't get affected by image size
    delta = (delta * 2000) / m_tileCount;

    // swap pointers for next time

    short *temp = m_dataImage;
    m_dataImage = m_oldDataImage;
    m_oldDataImage = temp;

    return delta >= m_deltaThreshold;
}


bool ChangeDetector::readJpgHeader(unsigned char *jpeg,	int jpegLength,	int& width, int& height, int& headSize)
{
    m_restartInterval = 0;                                  // in case it isn't defined

    if(readMarkers(jpeg, width, height, headSize )==-1) {
        logDebug("Cannot read the jpeg header");
		return false;
	}
    if(( m_width <= 0 )||( m_height <= 0 ))
		return false;
    m_dataBytesLeft = jpegLength - headSize;

    m_getBits = 0;
    m_getBuff = 0;

    m_dcY = m_dcCb = m_dcCr = 0;

    // we can assume that the huffman tables haven't changed
    if (!m_initialized) {
        computeHuffmanTable( &m_htblYDC );
        computeHuffmanTable( &m_htblYAC );
        computeHuffmanTable( &m_htblCbCrDC );
        computeHuffmanTable( &m_htblCbCrAC );
        m_initialized = true;
    }
    return true;
}

bool ChangeDetector::decompressImage(unsigned char *jpegData)
{
    int tileIndex;

    m_data = jpegData;

    if (m_dataImage == NULL) {
        m_cxTile = (m_width  + m_hMcuSize - 1) / m_hMcuSize;
        m_cyTile = (m_height + m_vMcuSize - 1) / m_vMcuSize;

        if (m_restartInterval == 0) {
            // force skipping off in this case
            m_intervalsToSkip = 0;
            m_tilesToSkip = 0;
            m_tilesToProcess = m_cxTile * m_cyTile;     // process all tiles in one go
            m_tileCount = m_cxTile * m_cyTile;
        } else {
            if (m_intervalsToSkip < 0)
                m_intervalsToSkip = 0;

            if (m_tilesToSkip < 0)
                m_tilesToSkip = 0;

            if (m_tilesToSkip >= m_restartInterval)
                m_tilesToSkip = m_restartInterval-1;

            m_tileCount = (m_cxTile * m_cyTile * (m_restartInterval - m_tilesToSkip)) /
                    (m_restartInterval * (m_intervalsToSkip + 1));
        }

        logInfo(QString("Processing %1 tiles").arg(m_tileCount));

        // need to create the buffers
        m_chunkLength = 64;
        m_tileLength = m_chunkLength * m_blocksInMcu;
        m_dataImageSize = m_tileCount * CHANGEDETECTOR_SAMPLES * m_blocksInMcu;
        m_dataImage = (short *)malloc(m_dataImageSize * sizeof(short));
        m_oldDataImage = (short *)malloc(m_dataImageSize * sizeof(short));
    }

    //	Decompress all the tiles, or macroblocks, or MCUs

    m_dataImagePtr = m_dataImage;

    for (tileIndex = 0; tileIndex < m_tileCount; tileIndex++) {
        if (!decompressOneTile())
            return false;
    }
    return true;
}

bool ChangeDetector::decompressOneTile()
{
    int chunkIndex;

    if (m_restartInterval) {
        if (m_restartsToGo == m_tilesToSkip) {
            m_getBits  = 0;

            if ((m_tilesToSkip != 0) && (m_unreadMarker == 0))
                skipToNextRestart();
            readRestartMarker();

            m_dcY = m_dcCb = m_dcCr = 0;
            m_restartsToGo = m_restartInterval;

            // do restart interval skipping if necessary

            for (int i = 0; i < m_intervalsToSkip; i++) {
                skipToNextRestart();
                readRestartMarker();
            }
        }
    }

    chunkIndex = 0;
    for (int i=0; i<m_blocksInMcu; i++, chunkIndex += m_chunkLength) {
        huffmanDecode(i);
        inverseDCT();
    }
    if (m_restartInterval > 0)
        m_restartsToGo--;

    return true;
}

int ChangeDetector::readOneMarker()
{
    if (INPUT_BYTE(m_data) != 255)
		return -1;
    int marker = INPUT_BYTE(m_data);

    while (marker == 255)
        marker = INPUT_BYTE(m_data);

	return marker;
}

void ChangeDetector::skipMarker(void)
{
    int length = (int)INPUT_2BYTES(m_data);

    m_data += length - 2;
}

void ChangeDetector::getSof ()
{
    int length = (int)INPUT_2BYTES(m_data);

    m_precision = (int)INPUT_BYTE(m_data);

    m_height = (unsigned short)INPUT_2BYTES(m_data);

    m_width = (unsigned short)INPUT_2BYTES(m_data);

    m_component = (int)INPUT_BYTE(m_data);
	
	length -= 8;

    JPEG_COMPONENT_INFO *compptr;

    compptr = m_compInfo;

    int ci, c;
    for (ci = 0; ci < m_component; ci++){
        compptr->componentIndex = ci;
	
        compptr->componentId = (int)INPUT_BYTE(m_data);

        c = (int)INPUT_BYTE(m_data);

        compptr->hSampFactor = (c >> 4) & 15;
        compptr->vSampFactor = (c     ) & 15;
		
        if (ci==0) {
//            qDebug() << "Component 0 samp_factor: " << c;
        }
		
        compptr->quantTblNo = (int)INPUT_BYTE(m_data);
		
		compptr++;
	}

    if((m_compInfo[0].hSampFactor == 1) && (m_compInfo[0].vSampFactor == 1)){
        m_hMcuSize		= 8;
        m_vMcuSize		= 8;
        m_blocksInMcu	= 3;
    }
    if((m_compInfo[0].hSampFactor == 2) && (m_compInfo[0].vSampFactor == 2)){
        m_hMcuSize		= 16;
        m_vMcuSize		= 16;
        m_blocksInMcu	= 6;
    }
    if((m_compInfo[0].hSampFactor == 2) && (m_compInfo[0].vSampFactor == 1)){
        m_hMcuSize		= 16;
        m_vMcuSize		= 8;
        m_blocksInMcu	= 4;
    }
}

void ChangeDetector::getDht(void)
{
    int length = (int)INPUT_2BYTES(m_data);
	length -= 2;

    while (length>0) {
		//0:dc_huff_tbl[0]; 16:ac_huff_tbl[0];
		//1:dc_huff_tbl[1]; 17:ac_huff_tbl[1]

        int index = INPUT_BYTE(m_data);

		// decide which table to receive data
        HUFFTABLE* htblptr = NULL;
        switch (index){
            case 0:
                htblptr = &m_htblYDC;
                break;
            case 16:
                htblptr = &m_htblYAC;
                break;
            case 1:
                htblptr = &m_htblCbCrDC;
                break;
            case 17:
                htblptr = &m_htblCbCrAC;
                break;

            default:
                return;
		}
	
		int count, i;
		//
		// read in bits[]
		//
		htblptr->bits[0] = 0;
		count = 0;

		for (i = 1; i <= 16; i++) {
            htblptr->bits[i] = INPUT_BYTE(m_data);
			count += htblptr->bits[i];
		}		
		length -= (1 + 16);
		
		//
		// read in huffval
		//
		for (i = 0; i < count; i++){
            htblptr->huffval[i] = INPUT_BYTE(m_data);
		}
		length -= count;
	}
}

void ChangeDetector::getSos(void)
{
//  int length = (int)INPUT_2BYTES(m_data);
    m_intDiscard = (int)INPUT_2BYTES(m_data);
    int n = INPUT_BYTE(m_data);

    // Collect the component-spec parameters

    int i;

	for (i = 0; i < n; i++) {
        // just skip for the moment
//      cc = INPUT_BYTE(m_data);
        m_charDiscard = INPUT_BYTE(m_data);
//      c = INPUT_BYTE(m_data);
        m_charDiscard = INPUT_BYTE(m_data);
    }
	
	// Collect the additional scan parameters Ss, Se, Ah/Al.

//  Ss	= INPUT_BYTE(m_data);
//  Se	= INPUT_BYTE(m_data);
//  c	= INPUT_BYTE(m_data);
//	Ah = (c >> 4) & 15;
//	Al = (c     ) & 15;

    m_charDiscard = INPUT_BYTE(m_data);
    m_charDiscard = INPUT_BYTE(m_data);
    m_charDiscard = INPUT_BYTE(m_data);

    m_nextRestartNum = 0;
}

void ChangeDetector::getDri()
{
//  int length = (int)INPUT_2BYTES(m_data);
    m_intDiscard = (int)INPUT_2BYTES(m_data);
    m_restartInterval = INPUT_2BYTES(m_data);

    m_restartsToGo = m_restartInterval;

//    qDebug() << "restart_interval: " << m_restartInterval;
}

int ChangeDetector::readMarkers(unsigned char *jpeg, int& width, int& height, int& headSize)
{
    m_data = jpeg;
	int retval = -1;
	for (;;) {
        int marker = readOneMarker();
		switch (marker) {
            case M_SOI:
                 break;

            case M_APP0:
            case M_APP1:
            case M_APP2:
            case M_APP3:
            case M_APP4:
            case M_APP5:
            case M_APP6:
            case M_APP7:
            case M_APP8:
            case M_APP9:
            case M_APP10:
            case M_APP11:
            case M_APP12:
            case M_APP13:
            case M_APP14:
            case M_APP15:
                skipMarker();
                break;

            case M_DQT:
                skipMarker();
                break;
			
            case M_SOF0:
            case M_SOF1:
                getSof();
                break;
			
            case M_SOF2:		//* Progressive, Huffman
                logDebug("Prog + Huff is not supported");
                return -1;
                break;
			
            case M_SOF9:		//* Extended sequential, arithmetic
                logDebug("sequential + Arith is not supported");
                return -1;
                break;
			
            case M_SOF10:		//* Progressive, arithmetic
                logDebug("Prog + Arith is not supported");
                return -1;
                break;
					
            case M_DHT:
                if (!m_initialized)
                    getDht();
                else
                    skipMarker();   // we already have the huffman tables
                break;

            case M_SOS:
                getSos();
                retval = 0;
                width = m_width;
                height = m_height;
                headSize = m_data - jpeg;
                return retval;
                break;

            //the following marker are not needed for jpg made by ms paint
            case M_COM:
                skipMarker();
                break;

            case M_DRI:
                getDri();
                break;
			
            default:
                logDebug(QString("Unsupported marker: %1").arg(marker));
                return -1;
                break;
		}
        m_unreadMarker = 0;
	}
}

void ChangeDetector::readRestartMarker()
{
    if (m_unreadMarker == 0) {
        m_unreadMarker = readOneMarker();
    }

    if (m_unreadMarker == ((int) M_RST0 + m_nextRestartNum)) {
        m_unreadMarker = 0;
    } else {
        logDebug(QString("Unexpected marker - expected: %1 got: %2").arg((int) M_RST0 + m_nextRestartNum).arg(m_unreadMarker));
    }

    m_nextRestartNum = (m_nextRestartNum + 1) & 7;
}

unsigned char ChangeDetector::imageConvert(short val)
{
    val = (val + 128) / 2;

    if (val < 0)
        val = 0;
    if (val > 255)
        val = 255;
    return (unsigned char)val;
}

QByteArray ChangeDetector::makeJpegDataImage()
{
    unsigned char *image, *imagePtr;

    unsigned short row;

    unsigned short rowLength = m_cxTile * m_hMcuSize * 3;

    image = (unsigned char *)malloc(m_width * m_height * 3);

    if (rowLength != (m_width * 3)) {
        logDebug("row length mismatch");
    }

    m_dataImagePtr = m_dataImage;

    for (unsigned short yTile = 0; yTile < m_cyTile; yTile++) {
        for (unsigned short xTile = 0; xTile < m_cxTile; xTile++) {
            for (unsigned short block = 0; block < m_blocksInMcu; block++) {
                if (block >= (m_blocksInMcu - 2)) {
                    m_dataImagePtr += 4;                    // skip chrominance blocks
                    continue;
                }

                imagePtr = image + (yTile * m_vMcuSize) * rowLength +
                        (xTile * (m_blocksInMcu - 2) + block) * 8 * 3;

                for (row = 0; row < m_vMcuSize / 2; row++) {
                    for (int i = 0; i < 12; i++)
                        imagePtr[i + row * rowLength] = imageConvert(*m_dataImagePtr);
                    for (int i = 0; i < 12; i++)
                        imagePtr[i + 12 + row * rowLength] = imageConvert(*(m_dataImagePtr + 1));
                }
                for (; row < m_vMcuSize; row++) {
                    for (int i = 0; i < 12; i++)
                        imagePtr[i + row * rowLength] = imageConvert(*(m_dataImagePtr + 2));
                    for (int i = 0; i < 12; i++)
                        imagePtr[i + 12 + row * rowLength] = imageConvert(*(m_dataImagePtr + 3));
                }
                m_dataImagePtr += 4;
            }
        }
    }
    QByteArray jpegImage;

    QImage frame(image, m_width, m_height, QImage::Format_RGB888);

    QBuffer buffer(&jpegImage);
    buffer.open(QIODevice::WriteOnly);
    frame.save(&buffer, "JPEG");

    free(image);
    return jpegImage;
}

void ChangeDetector::inverseDCT()
{
    static qreal cosx0u0 = qCos(0);
    static qreal cosx1u0 = qCos(0);
    static qreal cosx0u1 = qCos(M_PI / 4.0);
    static qreal cosx1u1 = qCos(3.0 * M_PI / 4.0);

    static qreal scaleDC = sqrt(0.5);

    qreal tempData[CHANGEDETECTOR_SAMPLES];

    // do columns

    tempData[0 + 0] = scaleDC * (qreal)m_coeffs[0 + 0] * cosx0u0 + (qreal)m_coeffs[8 + 0] * cosx0u1;
    tempData[0 + 1] = scaleDC * (qreal)m_coeffs[0 + 1] * cosx0u0 + (qreal)m_coeffs[8 + 1] * cosx0u1;
    tempData[2 + 0] = scaleDC * (qreal)m_coeffs[0 + 0] * cosx1u0 + (qreal)m_coeffs[8 + 0] * cosx1u1;
    tempData[2 + 1] = scaleDC * (qreal)m_coeffs[0 + 1] * cosx1u0 + (qreal)m_coeffs[8 + 1] * cosx1u1;

    // do rows

    m_dataImagePtr[0] = scaleDC * tempData[0 + 0] * cosx0u0 + tempData[0 + 1] * cosx0u1;
    m_dataImagePtr[1] = scaleDC * tempData[2 + 0] * cosx0u0 + tempData[2 + 1] * cosx0u1;
    m_dataImagePtr[2] = scaleDC * tempData[0 + 0] * cosx1u0 + tempData[0 + 1] * cosx1u1;
    m_dataImagePtr[3] = scaleDC * tempData[2 + 0] * cosx1u0 + tempData[2 + 1] * cosx1u1;
    m_dataImagePtr += CHANGEDETECTOR_SAMPLES;
}


void ChangeDetector::computeHuffmanTable(HUFFTABLE * dtbl)
{
	int p, i, l, si;
	int lookbits, ctr;
	char huffsize[257];
	unsigned int huffcode[257];
	unsigned int code;

	unsigned char *pBits = dtbl->bits;
	unsigned char *pVal  = dtbl->huffval;

	/* Figure C.1: make table of Huffman code length for each symbol */
	/* Note that this is in code-length order. */
	p = 0;
	for (l = 1; l <= 16; l++) {
		for (i = 1; i <= (int) pBits[l]; i++)
			huffsize[p++] = (char) l;
	}
	huffsize[p] = 0;
	
	/* Figure C.2: generate the codes themselves */
	/* Note that this is in code-length order. */
	
	code = 0;
	si = huffsize[0];
	p = 0;
	while (huffsize[p]) {
		while (((int) huffsize[p]) == si) {
			huffcode[p++] = code;
			code++;
		}
		code <<= 1;
		si++;
	}
	
	/* Figure F.15: generate decoding tables for bit-sequential decoding */
	
	p = 0;
	for (l = 1; l <= 16; l++) {
		if (pBits[l]) {
			dtbl->valptr[l] = p; /* huffval[] index of 1st symbol of code length l */
			dtbl->mincode[l] = huffcode[p]; /* minimum code of length l */
			p += pBits[l];
			dtbl->maxcode[l] = huffcode[p-1]; /* maximum code of length l */
		} else {
			dtbl->maxcode[l] = -1;	/* -1 if no codes of this length */
		}
	}
	dtbl->maxcode[17] = 0xFFFFFL; /* ensures jpeg_huff_decode terminates */
	
	/* Compute lookahead tables to speed up decoding.
	 * First we set all the table entries to 0, indicating "too long";
	 * then we iterate through the Huffman codes that are short enough and
	 * fill in all the entries that correspond to bit sequences starting
	 * with that code.	 */
	
	memset( dtbl->look_nbits, 0, sizeof(int)*256 );
	
	int HUFF_LOOKAHEAD = 8;
	p = 0;
	for (l = 1; l <= HUFF_LOOKAHEAD; l++) 
	{
		for (i = 1; i <= (int) pBits[l]; i++, p++) 
		{
			/* l = current code's length, 
			p = its index in huffcode[] & huffval[]. Generate left-justified
			code followed by all possible bit sequences */
			lookbits = huffcode[p] << (HUFF_LOOKAHEAD-l);
			for (ctr = 1 << (HUFF_LOOKAHEAD-l); ctr > 0; ctr--) 
			{
				dtbl->look_nbits[lookbits] = l;
				dtbl->look_sym[lookbits] = pVal[p];
				lookbits++;
			}
		}
	}
}

void ChangeDetector::huffmanDecode(int blockIndex)
{	
    int *lastDC;
	int s, k, r;

	HUFFTABLE *dctbl, *actbl;

    if (blockIndex < m_blocksInMcu - 2){
		dctbl = &m_htblYDC;
		actbl = &m_htblYAC;
        lastDC = &m_dcY;
    } else {
		dctbl = &m_htblCbCrDC;
		actbl = &m_htblCbCrAC;
        if( blockIndex == m_blocksInMcu - 2 )
            lastDC = &m_dcCb;
		else
            lastDC = &m_dcCr;
	}

    memset(m_coeffs, 0, sizeof(short) * 64);
	
    // Section F.2.2.1: decode the DC coefficient difference
    s = getCategory(dctbl);                                 // get dc category number, s

	if (s) {
        r = getBits(s);                                     // get offset in this dc category
        s = valueFromCategory(s, r);                        // get dc difference value
    }
	
    // Convert DC difference to actual value, update last_dc_val
    s += *lastDC;
    *lastDC = s;

    // Output the DC coefficient (assumes jpeg_natural_order[0] = 0)
    m_coeffs[0] = (short) s;
    
    // Section F.2.2.2: decode the AC coefficients
    // Since zeroes are skipped, output area must be cleared beforehand

    for (k = 1; k < 64; k++) {
        s = getCategory( actbl );                           // s: (run, category)
        r = s >> 4;                                         //	r: run length for ac zero, 0 <= r < 16
        s &= 15;                                            //	s: category for this non-zero ac
		
        if (s) {
            k += r;                                         //	k: position for next non-zero ac
            r = getBits(s);                                 //	r: offset in this ac category
            s = valueFromCategory(s, r);                    //	s: ac value

            m_coeffs[jpegNaturalOrder[k]] = (short)s;
        } else {                                            // s = 0, means ac value is 0 ? Only if r = 15.
            if (r != 15)                                    //means all the left ac are zero
				break;
			k += 15;
		}
	}		
}

//  getCategory for dc, or (0 run length, ac category) for ac
//
//	The max length for Huffman codes is 15 bits; so we use 32 bits buffer	
//	m_nGetBuff, with the validated length is m_nGetBits.
//	Usually, more than 95% of the Huffman codes will be 8 or fewer bits long
//	To speed up, we should pay more attention on the codes whose length <= 8

inline int ChangeDetector::getCategory(HUFFTABLE* htbl)
{
	//	If left bits < 8, we should get more data
    if (m_getBits < 8)
        fillBitBuffer();

	//	Call special process if data finished; min bits is 1
    if (m_getBits < 8)
        return specialDecode(htbl, 1);

	//	Peek the first valid byte	
    int look = ((m_getBuff>>(m_getBits - 8))& 0xFF);
	int nb = htbl->look_nbits[look];

    if (nb) {
        m_getBits -= nb;

        if (m_getBits < 0)
            logDebug("Negative getbits in getCategory");

		return htbl->look_sym[look]; 
    } else                                                  // Decode long codes with length >= 9
        return specialDecode(htbl, 9);
}

void ChangeDetector::skipToNextRestart()
{
    unsigned char uc;

    m_getBits = 0;
    m_getBuff = 0;

    while (m_dataBytesLeft > 0) {
        uc = *m_data++;
        m_dataBytesLeft--;

        if (uc == 0xff) {
            if (m_dataBytesLeft == 0)
                return;

            do {
                uc = *m_data++;
                m_dataBytesLeft--;
            } while ((uc == 0xFF) && (m_dataBytesLeft > 0));

            if (uc != 0) {
                m_unreadMarker = uc;
                if ((uc < 0xd0) || (uc > 0xd7))
                    logDebug(QString("Unexpected marker in skip: %1").arg(uc));
                return;
            }
        }
    }
}

void ChangeDetector::fillBitBuffer()
{
	unsigned char uc;

    while (m_getBits < 25) {                                // #define MIN_GET_BITS  (32-7)
        if (m_dataBytesLeft > 0) {
            // Attempt to read a byte
            if (m_unreadMarker != 0)
                goto noMoreData;                            // can't advance past a marker

            uc = *m_data++;
            m_dataBytesLeft--;
			
			// If it's 0xFF, check and discard stuffed zero byte
			if (uc == 0xFF) {
				do {
                    uc = *m_data++;
                    m_dataBytesLeft--;
                } while (uc == 0xFF);
				
				if (uc == 0) {
					// Found FF/00, which represents an FF data byte
					uc = 0xFF;
                } else {
					// Oops, it's actually a marker indicating end of compressed data.
					// Better put it back for use later 
					
                    m_unreadMarker = uc;

noMoreData:
					// There should be enough bits still left in the data segment;
					// if so, just break out of the outer while loop.
					//if (m_nGetBits >= nbits)
                    if (m_getBits >= 0)
						break;
				}
			}

            m_getBuff = (m_getBuff << 8) | ((int) uc);
            m_getBits += 8;
        } else {
			break;
        }
	}
}

inline int ChangeDetector::getBits(int nbits)
{
    if (m_getBits < nbits)                                  //we should read nbits bits to get next data
        fillBitBuffer();
    m_getBits -= nbits;
//    qDebug() << "Negative getbits";
    return (int) (m_getBuff >> m_getBits) & ((1 << nbits)-1);
}

//	Special Huffman decode:
//	(1) For codes with length > 8
//	(2) For codes with length < 8 while data is finished

int ChangeDetector::specialDecode( HUFFTABLE* htbl, int minBits )
{
    int l = minBits;
	int code;
	
    // HUFF_DECODE has determined that the code is at least min_bits
    // bits long, so fetch that many bits in one swoop.

    code = getBits(l);
	
    // Collect the rest of the Huffman code one bit at a time.
    // This is per Figure F.16 in the JPEG spec.
	while (code > htbl->maxcode[l]) {
		code <<= 1;
        code |= getBits(1);
		l++;
	}
	
    // With garbage input we may reach the sentinel value l = 17.

	if (l > 16) {
        logDebug("SpecialDecode hit sentinel");
        return 0;                                           // fake a zero as the safest result
	}
	
    return htbl->huffval[ htbl->valptr[l] +	(int)(code - htbl->mincode[l])];
}

//	To find dc or ac value according to category and category offset

inline int ChangeDetector::valueFromCategory(int cate, int offset)
{
    // Method 1:
    // On some machines, a shift and add will be faster than a table lookup.
    // #define HUFF_EXTEND(x,s) ((x)< (1<<((s)-1)) ? (x) + (((-1)<<(s)) + 1) : (x))

    // Method 2: Table lookup
	
    // If (nOffset < half[nCate]), then value is below zero
    // Otherwise, value is above zero, and just the nOffset

    static const int half[16] =                             // entry n is 2**(n-1)
            { 0, 0x0001, 0x0002, 0x0004, 0x0008, 0x0010, 0x0020, 0x0040, 0x0080,
            0x0100, 0x0200, 0x0400, 0x0800, 0x1000, 0x2000, 0x4000 };
	
    // start[i] is the starting value in this category; surely it is below zero

    static const int start[16] =                            // entry n is (-1 << n) + 1
            { 0, ((-1)<<1) + 1, ((-1)<<2) + 1, ((-1)<<3) + 1, ((-1)<<4) + 1,
            ((-1)<<5) + 1, ((-1)<<6) + 1, ((-1)<<7) + 1, ((-1)<<8) + 1,
            ((-1)<<9) + 1, ((-1)<<10) + 1, ((-1)<<11) + 1, ((-1)<<12) + 1,
            ((-1)<<13) + 1, ((-1)<<14) + 1, ((-1)<<15) + 1 };

    return (offset < half[cate] ? offset + start[cate] : offset);
}
