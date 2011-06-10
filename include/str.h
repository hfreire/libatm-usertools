/*****************************************************************************
 *                                                                           *
 *                      10101000100010001010101100101010101                  *
 *                      0101010101               0101010100                  *
 *                      0101010101               0101010100                  *
 *                      1010101010        10101  0101110101                  *
 *                      1010101010       01  10  0101011101                  *
 *                      1010111010       01      0101010101                  *
 *                       010101010  01  010 0101 010101010                   *
 *                       010101010  01  010  01  010101011                   *
 *                        01010101  01  010  01  01010101                    *
 *                        11010101  01  010  01  01010110                    *
 *                         1010101  01  010  01  0101000                     *
 *                         1110101  01  010  01  0101001                     *
 *                          110101      010      010001                      *
 *                           10101     001       01011                       *
 *                            1101  10101        0101                        *
 *                             110               011                         *
 *                              10               01                          *
 *                               01             10                           *
 *                                 00         10                             *
 *                                  01       01                              *
 *                                    01   01                                *
 *                                      010                                  *
 *                                       1                                   *
 *                                                                           *
 *                        Universidade Técnica de Lisboa                     *
 *                                                                           *
 *                          Instituto Superior Técnico                       *
 *                                                                           *
 *                                                                           *
 *                                                                           *
 *                    RIC - Redes Integradas de Comunicações                 *
 *                               ATM Application                             *
 *                                                                           *
 *                       Professor Paulo Rogério Pereira                     *
 *                                                                           *
 *                                                                           *
 *****************************************************************************
 * @filename: strc.c                                                         *
 * @description: ATM streaming client.                                       *
 * @language: C                                                              *
 * @compiled on: cc/gcc                                                      *
 * @last_update_at: 2007-11-01                                               *
 *****************************************************************************
 * @students that colaborated on this file:                                  *
 *  57442 - Daniel Silvestre - daniel.silvestre@tagus.ist.utl.pt             *
 *  57476 - Hugo Miguel Pinho Freire - hugo.freire@tagus.ist.utl.pt          *
 *****************************************************************************
 * @changelog for this file:                                                 *
 *	No changelog was kept for this file.                  									 *
 *****************************************************************************
 * @comments for this file:                                                  *
 *****************************************************************************/

#ifndef __STR_H__
#define __STR_H__

#define DEFAULT_SDU_SIZE	40
#define MAX_SDU_SIZE 64000
#define DEFAULT_FILE_DIR	"/pub/"
#define FILE_NOT_FOUND "-1"
#define DEBUG_DESC stdout
#define STR_BHLI "str"
#define GET(cab,item) ((*cab & ATM_HDR_##item##_MASK) >> ATM_HDR_##item##_SHIFT)
#define GET2(cab,mask,shift) ((*cab & mask) >> shift)
#define PDU_CAB 8

#endif
