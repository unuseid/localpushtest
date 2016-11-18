/*
 * sdcard.c
 *
 *  Created on: 2014. 5. 23.
 *      Author: ryan
 */

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "inc/hw_memmap.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/interrupt.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/systick.h"
#include "fatfs/src/ff.h"
#include "fatfs/src/diskio.h"
#include "utils/uartstdio.h"
#include "utils/ustdlib.h"
#include "sdcard.h"

//*****************************************************************************
//
// Defines the size of the buffers that hold the path, or temporary data from
// the SD card.  There are two buffers allocated of this size.  The buffer size
// must be large enough to hold the longest expected full path name, including
// the file name, and a trailing null character.
//
//*****************************************************************************
#define PATH_BUF_SIZE   80

//*****************************************************************************
//
// Defines the size of the buffer that holds the command line.
//
//*****************************************************************************
#define CMD_BUF_SIZE    64

//*****************************************************************************
//
// This buffer holds the full path to the current working directory.  Initially
// it is root ("/").
//
//*****************************************************************************
static char g_pcCwdBuf[PATH_BUF_SIZE] = "/";

//*****************************************************************************
//
// A temporary data buffer used when manipulating file paths, or reading data
// from the SD card.
//
//*****************************************************************************
static char g_pcTmpBuf[PATH_BUF_SIZE];

//*****************************************************************************
//
// The following are data structures used by FatFs.
//
//*****************************************************************************
static FATFS g_sFatFs;
static DIR g_sDirObject;
static FILINFO g_sFileInfo;
static FIL g_sFileObject;

//*****************************************************************************
//
// A structure that holds a mapping between an FRESULT numerical code, and a
// string representation.  FRESULT codes are returned from the FatFs FAT file
// system driver.
//
//*****************************************************************************
typedef struct
{
    FRESULT iFResult;
    char *pcResultStr;
}
tFResultString;

//*****************************************************************************
//
// A macro to make it easy to add result codes to the table.
//
//*****************************************************************************
#define FRESULT_ENTRY(f)        { (f), (#f) }

//*****************************************************************************
//
// A table that holds a mapping between the numerical FRESULT code and it's
// name as a string.  This is used for looking up error codes for printing to
// the console.
//
//*****************************************************************************
tFResultString g_psFResultStrings[] =
{
    FRESULT_ENTRY(FR_OK),
    FRESULT_ENTRY(FR_DISK_ERR),
    FRESULT_ENTRY(FR_INT_ERR),
    FRESULT_ENTRY(FR_NOT_READY),
    FRESULT_ENTRY(FR_NO_FILE),
    FRESULT_ENTRY(FR_NO_PATH),
    FRESULT_ENTRY(FR_INVALID_NAME),
    FRESULT_ENTRY(FR_DENIED),
    FRESULT_ENTRY(FR_EXIST),
    FRESULT_ENTRY(FR_INVALID_OBJECT),
    FRESULT_ENTRY(FR_WRITE_PROTECTED),
    FRESULT_ENTRY(FR_INVALID_DRIVE),
    FRESULT_ENTRY(FR_NOT_ENABLED),
    FRESULT_ENTRY(FR_NO_FILESYSTEM),
    FRESULT_ENTRY(FR_MKFS_ABORTED),
    FRESULT_ENTRY(FR_TIMEOUT),
    FRESULT_ENTRY(FR_LOCKED),
    FRESULT_ENTRY(FR_NOT_ENOUGH_CORE),
    FRESULT_ENTRY(FR_TOO_MANY_OPEN_FILES),
    FRESULT_ENTRY(FR_INVALID_PARAMETER),
};

//*****************************************************************************
//
// Error reasons returned by ChangeDirectory().
//
//*****************************************************************************
#define NAME_TOO_LONG_ERROR 1
#define OPENDIR_ERROR       2

//*****************************************************************************
//
// A macro that holds the number of result codes.
//
//*****************************************************************************
#define NUM_FRESULT_CODES       (sizeof(g_psFResultStrings) /                 \
                                 sizeof(tFResultString))

//*****************************************************************************
//
// This function returns a string representation of an error code
// that was returned from a function call to FatFs.  It can be used
// for printing human readable error messages.
//
//*****************************************************************************
static const char *
StringFromFResult(FRESULT iFResult)
{
    uint32_t ui32Idx;

    //
    // Enter a loop to search the error code table for a matching error code.
    //
    for(ui32Idx = 0; ui32Idx < NUM_FRESULT_CODES; ui32Idx++)
    {
        //
        // If a match is found, then return the string name of the error code.
        //
        if(g_psFResultStrings[ui32Idx].iFResult == iFResult)
        {
            return(g_psFResultStrings[ui32Idx].pcResultStr);
        }
    }

    //
    // At this point no matching code was found, so return a string indicating
    // an unknown error.
    //
    return("UNKNOWN ERROR CODE");
}

//*****************************************************************************
//
// This function implements the "ls" command.  It opens the current directory
// and enumerates through the contents, and prints a line for each item it
// finds.  It shows details such as file attributes, time and date, and the
// file size, along with the name.  It shows a summary of file sizes at the end
// along with free space.
//
//*****************************************************************************
int
Cmd_ls(int argc, char *argv[])
{
    uint32_t ui32TotalSize, ui32ItemCount, ui32FileCount, ui32DirCount;
    FRESULT iFResult;
    FATFS *psFatFs;
    char *pcFileName;
#if _USE_LFN
    char pucLfn[_MAX_LFN + 1];
    g_sFileInfo.lfname = pucLfn;
    g_sFileInfo.lfsize = sizeof(pucLfn);
#endif

    //
    // Open the current directory for access.
    //
    iFResult = f_opendir(&g_sDirObject, g_pcCwdBuf);

    //
    // Check for error and return if there is a problem.
    //
    if(iFResult != FR_OK)
    {
        //
        // Ensure that the error is reported.
        //
        return(iFResult);
    }

    ui32TotalSize = 0;
    ui32FileCount = 0;
    ui32DirCount = 0;
    ui32ItemCount = 0;

    //
    // Give an extra blank line before the listing.
    //
    UARTprintf("\n");

    //
    // Enter loop to enumerate through all directory entries.
    //
    for(;;)
    {
        //
        // Read an entry from the directory.
        //
        iFResult = f_readdir(&g_sDirObject, &g_sFileInfo);

        //
        // Check for error and return if there is a problem.
        //
        if(iFResult != FR_OK)
        {
            return((int)iFResult);
        }

        //
        // If the file name is blank, then this is the end of the listing.
        //
        if(!g_sFileInfo.fname[0])
        {
            break;
        }

#if _USE_LFN
        pcFileName = ((*g_sFileInfo.lfname)?g_sFileInfo.lfname:g_sFileInfo.fname);
#else
        pcFileName = g_sFileInfo.fname;
#endif

        //
        // Print the entry information on a single line with formatting to show
        // the attributes, date, time, size, and name.
        //
        UARTprintf("%c%c%c%c%c %u/%02u/%02u %02u:%02u %9u  %s\n",
                 (g_sFileInfo.fattrib & AM_DIR) ? 'D' : '-',
                 (g_sFileInfo.fattrib & AM_RDO) ? 'R' : '-',
                 (g_sFileInfo.fattrib & AM_HID) ? 'H' : '-',
                 (g_sFileInfo.fattrib & AM_SYS) ? 'S' : '-',
                 (g_sFileInfo.fattrib & AM_ARC) ? 'A' : '-',
                 (g_sFileInfo.fdate >> 9) + 1980,
                 (g_sFileInfo.fdate >> 5) & 15,
                 g_sFileInfo.fdate & 31,
                 (g_sFileInfo.ftime >> 11),
                 (g_sFileInfo.ftime >> 5) & 63,
                 g_sFileInfo.fsize,
                 pcFileName);

        //
        // If the attribute is directory, then increment the directory count.
        //
        if(g_sFileInfo.fattrib & AM_DIR)
        {
            ui32DirCount++;
        }

        //
        // Otherwise, it is a file.  Increment the file count, and
        // add in the file size to the total.
        //
        else
        {
            ui32FileCount++;
            ui32TotalSize += g_sFileInfo.fsize;
        }

        //
        // Move to the next entry in the item array we use to populate the
        // list box.
        //
        ui32ItemCount++;

        //
        // Wait for the UART transmit buffer to empty.
        //
        UARTFlushTx(false);

    }   // endfor

    //
    // Print summary lines showing the file, dir, and size totals.
    //
    UARTprintf("\n%4u File(s),%10u bytes total\n%4u Dir(s)",
                ui32FileCount, ui32TotalSize, ui32DirCount);

    //
    // Get the free space.
    //
    iFResult = f_getfree("/", (DWORD *)&ui32TotalSize, &psFatFs);

    //
    // Check for error and return if there is a problem.
    //
    if(iFResult != FR_OK)
    {
        return((int)iFResult);
    }

    //
    // Display the amount of free space that was calculated.
    //
    UARTprintf(", %10uK bytes free\n", (ui32TotalSize *
                                        psFatFs->free_clust / 2));
    //
    // Wait for the UART transmit buffer to empty.
    //
    UARTFlushTx(false);

    //
    // Made it to here, return with no errors.
    //
    return(0);
}

//*****************************************************************************
//
// This function implements the "cd" command.  It takes an argument
// that specifes the directory to make the current working directory.
// Path separators must use a forward slash "/".  The argument to cd
// can be one of the following:
// * root ("/")
// * a fully specified path ("/my/path/to/mydir")
// * a single directory name that is in the current directory ("mydir")
// * parent directory ("..")
//
// It does not understand relative paths, so dont try something like this:
// ("../my/new/path")
//
// Once the new directory is specified, it attempts to open the directory to
// make sure it exists.  If the new path is opened successfully, then the
// current working directory (cwd) is changed to the new path.
//
// In cases of error, the pui32Reason parameter will be written with one of
// the following values:
//
//*****************************************************************************
static FRESULT
ChangeToDirectory(char *pcDirectory, uint32_t *pui32Reason)
{
    uint32_t ui32Idx;
    FRESULT iFResult;

    //
    // Copy the current working path into a temporary buffer so it can be
    // manipulated.
    //
    strcpy(g_pcTmpBuf, g_pcCwdBuf);

    //
    // If the first character is /, then this is a fully specified path, and it
    // should just be used as-is.
    //
    if(pcDirectory[0] == '/')
    {
        //
        // Make sure the new path is not bigger than the cwd buffer.
        //
        if(strlen(pcDirectory) + 1 > sizeof(g_pcCwdBuf))
        {
            *pui32Reason = NAME_TOO_LONG_ERROR;
            return(FR_OK);
        }

        //
        // If the new path name (in argv[1])  is not too long, then
        // copy it into the temporary buffer so it can be checked.
        //
        else
        {
            strncpy(g_pcTmpBuf, pcDirectory, sizeof(g_pcTmpBuf));
        }
    }

    //
    // If the argument is .. then attempt to remove the lowest level
    // on the CWD.
    //
    else if(!strcmp(pcDirectory, ".."))
    {
        //
        // Get the index to the last character in the current path.
        //
        ui32Idx = strlen(g_pcTmpBuf) - 1;

        //
        // Back up from the end of the path name until a separator (/) is
        // found, or until we bump up to the start of the path.
        //
        while((g_pcTmpBuf[ui32Idx] != '/') && (ui32Idx > 1))
        {
            //
            // Back up one character.
            //
            ui32Idx--;
        }

        //
        // Now we are either at the lowest level separator in the current path,
        // or at the beginning of the string (root).  So set the new end of
        // string here, effectively removing that last part of the path.
        //
        g_pcTmpBuf[ui32Idx] = 0;
    }

    //
    // Otherwise this is just a normal path name from the current directory,
    // and it needs to be appended to the current path.
    //
    else
    {
        //
        // Test to make sure that when the new additional path is added on to
        // the current path, there is room in the buffer for the full new path.
        // It needs to include a new separator, and a trailing null character.
        //
        if(strlen(g_pcTmpBuf) + strlen(pcDirectory) + 1 + 1 > sizeof(g_pcCwdBuf))
        {
            *pui32Reason = NAME_TOO_LONG_ERROR;
            return(FR_INVALID_OBJECT);
        }

        //
        // The new path is okay, so add the separator and then append the new
        // directory to the path.
        //
        else
        {
            //
            // If not already at the root level, then append a /
            //
            if(strcmp(g_pcTmpBuf, "/"))
            {
                strcat(g_pcTmpBuf, "/");
            }

            //
            // Append the new directory to the path.
            //
            strcat(g_pcTmpBuf, pcDirectory);
        }
    }

    //
    // At this point, a candidate new directory path is in chTmpBuf.  Try to
    // open it to make sure it is valid.
    //
    iFResult = f_opendir(&g_sDirObject, g_pcTmpBuf);

    //
    // If it can't be opened, then it is a bad path.  Inform the user and
    // return.
    //
    if(iFResult != FR_OK)
    {
        *pui32Reason = OPENDIR_ERROR;
        return(iFResult);
    }

    //
    // Otherwise, it is a valid new path, so copy it into the CWD and update
    // the screen.
    //
    else
    {
        strncpy(g_pcCwdBuf, g_pcTmpBuf, sizeof(g_pcCwdBuf));
    }

    //
    // Return success.
    //
    return(FR_OK);
}

//*****************************************************************************
//
// This function implements the "cd" command.  It takes an argument
// that specifes the directory to make the current working directory.
// Path separators must use a forward slash "/".  The argument to cd
// can be one of the following:
// * root ("/")
// * a fully specified path ("/my/path/to/mydir")
// * a single directory name that is in the current directory ("mydir")
// * parent directory ("..")
//
// It does not understand relative paths, so dont try something like this:
// ("../my/new/path")
//
// Once the new directory is specified, it attempts to open the directory
// to make sure it exists.  If the new path is opened successfully, then
// the current working directory (cwd) is changed to the new path.
//
//*****************************************************************************
int
Cmd_cd(int argc, char *argv[])
{
    uint32_t ui32Reason;
    FRESULT iFResult;

    //
    // Try to change to the directory provided on the command line.
    //
    iFResult = ChangeToDirectory(argv[1], &ui32Reason);

    //
    // If an error was reported, try to offer some helpful information.
    //
    if(iFResult != FR_OK)
    {
        switch(ui32Reason)
        {
            case OPENDIR_ERROR:
                UARTprintf("Error opening new directory.\n");
                break;

            case NAME_TOO_LONG_ERROR:
                UARTprintf("Resulting path name is too long.\n");
                break;

            default:
                UARTprintf("An unrecognized error was reported.\n");
                break;
        }
    }

    //
    // Return the appropriate error code.
    //
    return(iFResult);
}

//*****************************************************************************
//
// This function implements the "pwd" command.  It simply prints the current
// working directory.
//
//*****************************************************************************
int
Cmd_pwd(int argc, char *argv[])
{
    //
    // Print the CWD to the console.
    //
    UARTprintf("%s\n", g_pcCwdBuf);

    //
    // Wait for the UART transmit buffer to empty.
    //
    UARTFlushTx(false);

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
// This function implements the "cat" command.  It reads the contents of a file
// and prints it to the console.  This should only be used on text files.  If
// it is used on a binary file, then a bunch of garbage is likely to printed on
// the console.
//
//*****************************************************************************
int
Cmd_cat(int argc, char *argv[])
{
    FRESULT iFResult;
    uint32_t ui32BytesRead;

    //
    // First, check to make sure that the current path (CWD), plus the file
    // name, plus a separator and trailing null, will all fit in the temporary
    // buffer that will be used to hold the file name.  The file name must be
    // fully specified, with path, to FatFs.
    //
    if(strlen(g_pcCwdBuf) + strlen(argv[1]) + 1 + 1 > sizeof(g_pcTmpBuf))
    {
        UARTprintf("Resulting path name is too long\n");
        return(0);
    }

    //
    // Copy the current path to the temporary buffer so it can be manipulated.
    //
    strcpy(g_pcTmpBuf, g_pcCwdBuf);

    //
    // If not already at the root level, then append a separator.
    //
    if(strcmp("/", g_pcCwdBuf))
    {
        strcat(g_pcTmpBuf, "/");
    }

    //
    // Now finally, append the file name to result in a fully specified file.
    //
    strcat(g_pcTmpBuf, argv[1]);

    //
    // Open the file for reading.
    //
    iFResult = f_open(&g_sFileObject, g_pcTmpBuf, FA_READ);

    //
    // If there was some problem opening the file, then return an error.
    //
    if(iFResult != FR_OK)
    {
        return((int)iFResult);
    }

    //
    // Enter a loop to repeatedly read data from the file and display it, until
    // the end of the file is reached.
    //
    do
    {
        //
        // Read a block of data from the file.  Read as much as can fit in the
        // temporary buffer, including a space for the trailing null.
        //
        iFResult = f_read(&g_sFileObject, g_pcTmpBuf, sizeof(g_pcTmpBuf) - 1,
                          (UINT *)&ui32BytesRead);

        //
        // If there was an error reading, then print a newline and return the
        // error to the user.
        //
        if(iFResult != FR_OK)
        {
            UARTprintf("\n");
            return((int)iFResult);
        }

        //
        // Null terminate the last block that was read to make it a null
        // terminated string that can be used with printf.
        //
        g_pcTmpBuf[ui32BytesRead] = 0;

        //
        // Print the last chunk of the file that was received.
        //
        UARTprintf("%s", g_pcTmpBuf);

        //
        // Wait for the UART transmit buffer to empty.
        //
        UARTFlushTx(false);

    //
    // Continue reading until less than the full number of bytes are
    // read.  That means the end of the buffer was reached.
    //
    }
    while(ui32BytesRead == sizeof(g_pcTmpBuf) - 1);

    //
    // Return success.
    //
    return(0);
}

//*****************************************************************************
//
//
//
//****************************************************************************
void SDCARD_Configure(void)
{
	FRESULT iFResult;

    ROM_GPIOPinConfigure(GPIO_PB5_SSI1CLK);
    ROM_GPIOPinConfigure(GPIO_PE4_SSI1XDAT0);
    ROM_GPIOPinConfigure(GPIO_PE5_SSI1XDAT1);
    ROM_GPIOPinTypeSSI(GPIO_PORTB_BASE, GPIO_PIN_5);
    ROM_GPIOPinTypeSSI(GPIO_PORTE_BASE, GPIO_PIN_4 | GPIO_PIN_5);
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTB_BASE, GPIO_PIN_4);
    ROM_GPIOPinWrite(GPIO_PORTB_BASE, GPIO_PIN_4, GPIO_PIN_4);

    // Mount the file system, using logical disk 0.
    iFResult = f_mount(0, &g_sFatFs);
    if(iFResult != FR_OK) {
        UARTprintf("f_mount error: %s\r\n", StringFromFResult(iFResult));
    }
}

//*****************************************************************************
//
//
//
//****************************************************************************
int SDCARD_Write(const char *name, const char *data, uint32_t size)
{
    FRESULT iFResult;
    FIL g_sFileObject;
    uint32_t len;

    if(size < 1 || data == NULL) {
        return((int)FR_INVALID_PARAMETER);
    }

    // Open the file for reading.
    iFResult = f_open(&g_sFileObject, name, FA_WRITE|FA_OPEN_ALWAYS);
    if(iFResult != FR_OK) {
        return((int)iFResult);
    }

	iFResult = f_lseek (&g_sFileObject, g_sFileObject.fsize);
	if(iFResult != FR_OK) {
		return((int)iFResult);
	}

	iFResult =  f_write(&g_sFileObject, data, size, &len);
	if(iFResult != FR_OK) {
		return((int)iFResult);
	}

    f_close(&g_sFileObject);
    return iFResult;
}

//*****************************************************************************
//
//
//
//****************************************************************************
int SDCARD_Read(const char *name, char *data, uint32_t offset, uint32_t size)
{
    FRESULT iFResult;
    FIL g_sFileObject;
    uint32_t ui32BytesRead;

    iFResult = f_open(&g_sFileObject, name, FA_READ);
    if(iFResult != FR_OK) {
        return -iFResult;
    }

	iFResult = f_lseek (&g_sFileObject, offset);
	if(iFResult != FR_OK) {
		return -iFResult;
	}

    iFResult = f_read(&g_sFileObject, data, size, (UINT *)&ui32BytesRead);
    if(iFResult != FR_OK) {
    	return -iFResult;
    }

    f_close(&g_sFileObject);
    return (int)ui32BytesRead;
}

//*****************************************************************************
//
//
//
//****************************************************************************
int SDCARD_ReadLine(const char *name, char *data, uint32_t offset, uint32_t size)
{
    FRESULT iFResult;
    FIL fc;

    iFResult = f_open(&fc, name, FA_READ);
    if(iFResult != FR_OK) {
        return -iFResult;
    }

	iFResult = f_lseek (&fc, offset);
	if(iFResult != FR_OK) {
		return -iFResult;
	}

	int n = 0;
	TCHAR c, *p = data;
	BYTE s[2];
	UINT rc;
	while(n < size-1) {
		f_read(&fc, s, 1, &rc);
		if (rc != 1) break;
		c = s[0];

		*p++ = c;
		n++;
		if (c == '\n') break;		/* Break on EOL */
	}
	*p = 0;

    f_close(&fc);
    return n;
}


//*****************************************************************************
//
//
//
//****************************************************************************

int SDCARD_find(const char *name)
{
    FRESULT iFResult;
    char *pcFileName;
#if _USE_LFN
    char pucLfn[_MAX_LFN + 1];
    g_sFileInfo.lfname = pucLfn;
    g_sFileInfo.lfsize = sizeof(pucLfn);
#endif

    // Open the current directory for access.
    iFResult = f_opendir(&g_sDirObject, g_pcCwdBuf);

    // Check for error and return if there is a problem.
    if(iFResult != FR_OK)
    {
        UARTprintf("%s\r\n", StringFromFResult(iFResult));
        return((int)iFResult);
    }

    // Enter loop to enumerate through all directory entries.
    for(;;) {
        // Read an entry from the directory.
        iFResult = f_readdir(&g_sDirObject, &g_sFileInfo);

        // Check for error and return if there is a problem.
        if(iFResult != FR_OK) {
            UARTprintf("%s\r\n", StringFromFResult(iFResult));
            return((int)iFResult);
        }

        // If the file name is blank, then this is the end of the listing.
        if(!g_sFileInfo.fname[0])  {
            break;
        }

        // If the attribue is directory, then increment the directory count.
        if(g_sFileInfo.fattrib & AM_DIR) {
            continue;
        }

#if _USE_LFN
        pcFileName = ((*g_sFileInfo.lfname)?g_sFileInfo.lfname:g_sFileInfo.fname);
#else
        pcFileName = g_sFileInfo.fname;
#endif

        if(strcmp(pcFileName, name) == 0) {
            return 1;
        }
    }

    //UARTprintf("%s\r\n", global.logfile_name);
    return 0;
}

