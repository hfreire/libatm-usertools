/* crc32h.h -- package to compute 32-bit CRC one byte at a time using   */
/*             the high-bit first (Big-Endian) bit ordering convention  */

extern void gen_crc_table(void);
extern unsigned long update_crc(unsigned long crc_accum, char *data_blk_ptr,
                                                    int data_blk_size);


