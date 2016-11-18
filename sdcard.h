/*
 * sdcard.h
 *
 *  Created on: 2014. 5. 23.
 *      Author: ryan
 */

#ifndef SDCARD_H_
#define SDCARD_H_

#ifdef __cplusplus
extern "C" {
#endif

int Cmd_ls(int argc, char *argv[]);
int Cmd_cd(int argc, char *argv[]);
int Cmd_pwd(int argc, char *argv[]);
int Cmd_cat(int argc, char *argv[]);
int Cmd_write(int argc, char *argv[]);
int Cmd_rm(int argc, char *argv[]);

void SDCARD_Configure(void);
int SDCARD_Write(const char *name, const char *data, uint32_t size);
int SDCARD_Read(const char *name, char *data, uint32_t offset, uint32_t size);
int SDCARD_ReadLine(const char *name, char *data, uint32_t offset, uint32_t size);
int SDCARD_find(const char *name);

#ifdef __cplusplus
}
#endif

#endif /* SDCARD_H_ */
