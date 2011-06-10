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
 * @filename: strc0.c                                                        *
 * @description: ATM streaming client using PVC and AAL5 emulation over ALL0.*
 * @language: C                                                              *
 * @compiled on: cc/gcc                                                      *
 * @last update at: 2007-11-01                                               *
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

#include "../include/str.h"
#include "../include/crc32h.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include <atm.h>
#include <atmsap.h>
#include <netinet/in.h>

int buflen = DEFAULT_SDU_SIZE;	/* length of buffer */
int sdu_size;
int bitrate;
char name[255];
char filename[255];
struct sockaddr_atmpvc catm;

/*****************************************************************************
 * @name						usage
 * @description  		prints to stderr the correct syntax usage
 * @param						none
 * @return          void
 */
void usage()
{
    fprintf(stderr,"usage: %s [itf.]vpi.vci filename rate [sdu_size] \n",name);
    exit(1);
}

/*****************************************************************************
 * @name						crcerr
 * @description  		prints crc error
 * @param				    none		
 * @return          void
 */
void crcerr()
{
    fprintf(stderr,"strc0: crc errors were detected.\n");
}

/*****************************************************************************
 * @name						die_with_error
 * @description     does a perror then exits with 1 
 * @param           message
 */
void die_with_error(const char *message)
{
    perror(message);
    exit(1);
}

/*****************************************************************************
 * @name            mkheader
 * @description     make header for cell 
 * @param           integer telling if last cell
 */
unsigned long mkheader(int last)
{
    unsigned long header[1];
    int pti, gfc, clp, vci;
    short vpi;

    gfc = clp = 0;
    /* set vpi and vci */
    vpi = catm.sap_addr.vpi;
    vci = catm.sap_addr.vci;
    /* if last cell, turn pti on */
    if(last) pti = 1;
    else pti = 0;
    /* apply masks */
    *header = ((gfc << ATM_HDR_GFC_SHIFT) & ATM_HDR_GFC_MASK) |
              ((vpi << ATM_HDR_VPI_SHIFT) & ATM_HDR_VPI_MASK) |
              ((vci << ATM_HDR_VCI_SHIFT) & ATM_HDR_VCI_MASK) |
              ((pti << ATM_HDR_PTI_SHIFT) & ATM_HDR_PTI_MASK) | clp;
    return *header;
}

/*****************************************************************************
 * @name            mkcell
 * @description     makes a cell from a pdu
 * @param           cell number in pdu, buffer and integer telling if last cell
 */
char* mkcell(int n,char *buf, int last)
{
    int i,j;
    char *cell;
    unsigned long header;

    /* alloc memory for cell */
    if ((cell = (char *)malloc(ATM_AAL0_SDU * sizeof(char))) == (char *)NULL)
        die_with_error("malloc() failed");
    /* create a header */
    header = mkheader(last);

    memset(cell,0,sizeof(cell));
    memcpy(cell,&header,sizeof(header));
    /* insert buffer in cell */
    for(i = n * ATM_CELL_PAYLOAD, j = 0; i < n * ATM_CELL_PAYLOAD + ATM_CELL_PAYLOAD;i++,j++)
    {
        cell[j+4] = buf[i];
    }
    return cell;
}

/*****************************************************************************
 * @name            cwrite
 * @description     cell write 
 * @param           socket, buffer and buffer lenght
 */
int cwrite(int s,char* buf,int l)
{
    int padding, n, num;
    unsigned char *pdu;
    int i,size;
    unsigned long header[2];
    unsigned int crc, lenght;
    char *cell;
    int pdu_size;

    padding = n = num = pdu_size = 0;

    /* get padding */
    if((l + PDU_CAB) <= ATM_CELL_PAYLOAD)
        padding = 40 - l;
    else
    {
        if(((l + PDU_CAB) % ATM_CELL_PAYLOAD) == 0)
            padding = 0;
        else
        {
            n = (l + PDU_CAB) / ATM_CELL_PAYLOAD + 1;
            padding = n * ATM_CELL_PAYLOAD - (l + PDU_CAB);
        }
    }
    /* alloc memory for pdu */
    pdu_size = (l + padding + PDU_CAB);
    pdu = malloc((pdu_size * sizeof(unsigned char)));

    memset(pdu,0,sizeof(pdu));
    /* insert data */
    memcpy(pdu,buf,l);

    /* insert padding */
//  num = strlen(pdu);
//  for(i = num; i < (num + padding) ;i++)
//    pdu[i] = 'L';

    /* insert lenght on pdu */
    lenght = l & 0x0000ffff;
    *header = htonl(lenght | ( 0 << 2));
    *(header+1) = 0;
    memcpy(pdu+l+padding,&header,sizeof(header));
    /* insert crc on pdu */
    crc = ~update_crc(0xffffffff,(char *)pdu,(l+padding+4));
    *(header+1) = htonl(crc);
    memcpy(pdu+padding+l,&header,sizeof(header));
    /* make cells and send them*/
    for(n = 0,i = 0;i<(pdu_size)/ATM_CELL_PAYLOAD;i++,n += size)
    {
        cell = mkcell(i,(char *)pdu, i == (((pdu_size)/ATM_CELL_PAYLOAD)-1) ? 1:0);
        if((size = write(s,cell,ATM_AAL0_SDU)) < 0)
            die_with_error("write() failed");
        free(cell);
    }
    return n;
}

/*****************************************************************************
 * @name            cread
 * @description     cell read 
 * @param           socket, buffer and buffer lenght
 * @return          returns the number of bytes red
 */
int cread(int s,char *buf, int lenght)
{

    int size, lcell, ncell, sdu_s, n;
    unsigned long aal0_cab[1];
    unsigned long pdu_cab[2];
    char *tbuf;
    unsigned char *cell;
    unsigned long crc, crc_aux;

    lcell = ncell = n = 0;
    /* alloc memory  for temporary buffers */
    tbuf = malloc(ATM_CELL_PAYLOAD*sizeof(char));
    cell = malloc(ATM_AAL0_SDU*sizeof(unsigned char));
    /* read cells */
    while(!lcell)
    {
        if ((size = read(s,cell,ATM_AAL0_SDU)) < 0)
            die_with_error("read() failed");
        n += size;
        memcpy(tbuf+(ncell*ATM_CELL_PAYLOAD),cell+4,ATM_CELL_PAYLOAD);
        memcpy(&aal0_cab,cell,sizeof(unsigned long));
        /* read pti from cell */
        lcell = GET(aal0_cab,PTI);
        if(!lcell)
        {
            ncell++;
            /* realloc memory if necessary */
            tbuf = realloc(tbuf,ncell*ATM_CELL_PAYLOAD+ATM_CELL_PAYLOAD);
        }
    }
    /* read header from last cell */
    memcpy(pdu_cab,cell+ATM_CELL_PAYLOAD-4,sizeof(unsigned long));
    memcpy((pdu_cab+1),cell+ATM_CELL_PAYLOAD,sizeof(unsigned long));
    /* get lenght and crc */
    sdu_s= ntohl(GET2(pdu_cab,0xffff0000,0));
    crc = ntohl(*(pdu_cab+1));
    /* verify crc */
    crc_aux = ~update_crc(0xffffffff,tbuf,(ncell+1)*ATM_CELL_PAYLOAD-4);
    if(crc_aux!=crc) return -1;
    /* copy to given buffer */
    memcpy(buf,tbuf,sdu_s);
    /* free temporary buffers */
    free(tbuf);
    free(cell);
    return n;
}

/*****************************************************************************
 * @name            main
 * @description     sends requests for files 
 * @param           argc, argv
 * @return          returns 0 on a successful exit
 */
int main(int argc,char **argv)
{

    struct atm_qos qos;
    int s, i, n;
    long l; /* file size */
    char *buf;	/* ptr to dynamic buffer */

    memcpy(name,argv[0],strlen(argv[0]));
    /* verify syntax */
    if (argc != 4 && argc != 5) usage();

    memcpy(filename,argv[2],strlen(argv[2]));
    bitrate = atoi(argv[3]);
    if (argc == 5) sdu_size = atoi(argv[4]);
    else sdu_size = DEFAULT_SDU_SIZE;

    memset(&catm,0,sizeof(catm));

    if(text2atm(argv[1],(struct sockaddr *) &catm,sizeof(catm),
                T2A_PVC | T2A_UNSPEC | T2A_WILDCARD) < 0) usage();

    /* create a socket */
    if ((s = socket(AF_ATMPVC,SOCK_DGRAM,ATM_AAL0)) < 0)
        die_with_error("socket() failed");

    memset(&qos,0,sizeof(qos));
    /* configure qos */
    qos.aal = ATM_AAL0;
    qos.rxtp.max_sdu = qos.txtp.max_sdu = ATM_AAL0_SDU;
    qos.rxtp.traffic_class = qos.txtp.traffic_class = ATM_UBR;
    /* set options on socket */
    if (setsockopt(s,SOL_ATM,SO_ATMQOS,&qos,sizeof(qos)) < 0)
        die_with_error("setsockopt() failed");
    /* ignore sigpipe */
    signal(SIGPIPE,SIG_IGN);
    /* generate crc table */
    gen_crc_table();
    /* pvc/svc bind */
    if(bind(s,(struct sockaddr *) &catm, sizeof(catm)) < 0)
        die_with_error("bind() failed");
    /* adjust the buffer for default sdu size */
    if ((buf = (char *)malloc(buflen)) == (char *)NULL)
        die_with_error("malloc() failed");
    /* send the filename */
    memcpy(buf,filename,strlen(filename));
    cwrite(s,buf,buflen);
    /* send the bitrate */
    sprintf(buf,"%i",bitrate);
    cwrite(s,buf,buflen);
    /* send the sdu size */
    sprintf(buf,"%i",sdu_size);
    cwrite(s,buf,buflen);
    /* read the file size */
    if((cread(s,buf,buflen)) < 0) crcerr();
    /* parse size from string */
    l = atol(buf);
    /* if file not found exit */
    if(atol(buf) < 0) exit(0);
    /* adjust the buffer for sdu size */
    if(buflen != sdu_size)
    {
        free(buf);
        buflen = sdu_size;
        if ((buf = (char *)malloc(buflen)) == (char *)NULL)
            die_with_error("malloc() failed");
    }
    /* receive the file */
    for(i = 0,n = buflen;i < l;i += buflen)
    {
        if((cread(s,buf,buflen)) < 0) crcerr();
        if(i+buflen>l) n = l-i;
        fwrite(buf,n,1,stdout);
    }
    /* close socket */
    close(s);
//	free(buf);
    return 0;
}



