 #include "apdefap.h"   
 
int ExportArchiveToCSV2(SYSTEMTIME stStart, SYSTEMTIME stStop, char * ArchiveName, char * VarName, char * FileName)   
{   
    /* 
    Function exports the values of tag VarName from Tag Logging archive ArchiveName to text file FileName. 
    Data are exported for the period from stStart to stStop. 
    Function returns integer value representing the result of export: 
        0 - Export to file was successfull 
        2 - No data were received 
        4 - Can't get data from archive 
        5 - Can't open text file 
    */ 
    #pragma code("pdertcli.dll") 
    #include "pde_glob.h" 
    #include "pdertcli.h" 
    #pragma code()   
 
    BOOL                retcode, FirstPass = TRUE;   
    CMN_ERROR            error;   
    int                nIndex;   
    PTLG_ARCHIVDATARAW      pDat;   
    PTLG_ARCHIVDATARAW      g_pTlgData = NULL;   
    DWORD                      g_TLGdwNumberOfData =10000, g_TLGdwFlags = 0;   
    FILE                * LogFile;   
 
    // Open file FileName for write   
    LogFile = fopen(FileName, "w");   
    if (LogFile == NULL)   
    {   
        // If file was not open report an error and return   
        printf("ExportArchiveToCSV:  Can't open file %s\r\n", FileName);   
        return 5;   
    }   
    printf("ExportArchiveToCSV: file succesfully created:%s\r\n", FileName);   
 
    //The cycle that calls ODK function TLGGetArchivDataEx. The cycle is needed because   
    //this function returns no more than 10000 values. If there are more values in the requested   
    //period, we need to call this function several times.   
    do   
    {

        //Adjust maximum number of returned data records before the call   
        g_TLGdwNumberOfData=10000;   
        g_pTlgData = NULL;   
        // Get fata from archive   
        retcode = TLGGetArchivDataEx(ArchiveName, VarName, &stStart, &stStop, &g_pTlgData, &g_TLGdwNumberOfData, &g_TLGdwFlags, &error);   
 
        // In case of error while getting the data   
        if ((retcode != TRUE)||(g_pTlgData == NULL))   
        {   
      //Output diagnostic message   
            printf("ExportArchiveToCSV: Error in TLGGetArchivDataEx. %s\r\n",error.szErrorText);   
          //Free memory   
            if (g_pTlgData != NULL)   
            {   
                retcode = TLGFreeMemory(g_pTlgData);   
                if (retcode != TRUE)   
                    printf("ExportArchiveToCSV: Error in TLGFreeMemory. %s\r\n",error.szErrorText);   
            }   
            //Close text file   
            fclose(LogFile);   
            return 4;   
        }   
   // If there were no data received   
        if (g_TLGdwNumberOfData <= 0 )   
        {   
            //Output diagnostic message   
            printf("ExportArchiveToCSV: TLGGetArchivDataEx returned nothing.\r\n");   
            //Free memory   
            if (g_pTlgData != NULL)   
            {   
                retcode = TLGFreeMemory(g_pTlgData);   
                if (retcode != TRUE)   
                    printf("ExportArchiveToCSV: Error in TLGFreeMemory. %s\r\n",error.szErrorText);   
            }   
            if (FirstPass == FALSE)   
                // No more points available, but we have already received some data. Everything   
                //is ok. Exit the cycle   
                break;   
            else   
            {   
                //If it was the first call to TLGGetArchivDataEx and no data received, than report error code 2 (No data available)   
                fclose(LogFile);   
                return 2;   
            }   
        }   
        //Initialize pointer to move through received data   
        pDat = g_pTlgData;   
        printf("ExportArchiveToCSV: TLGGetArchivData: Returned %d points.\r\n", g_TLGdwNumberOfData);   
 
        //Move through all received data values, and output them to text file, line by line   
        for (nIndex=0; nIndex<(int)g_TLGdwNumberOfData; nIndex++ )   
        {   
            fprintf(LogFile, "%04d.%02d.%02d %02d.%02d.%02d,%7.3f,%d\r\n", pDat->stTime.wYear, pDat->stTime.wMonth, pDat->stTime.wDay, pDat->stTime.wHour, pDat->stTime.wMinute, pDat->stTime.wSecond, pDat->doValue, pDat->dwFlags);   
            pDat++;   
        }   
        //If function returned 10000 points, than probably more data available.   
        //So adjust stStart to time of last received data point and repeat the procedure.   
        if (g_TLGdwNumberOfData >= 10000)   
        {   
//            PrintToLogFile("ExportArchiveToCSV: More than 10000 points received, going to repeat the call\r\n");   
            FirstPass = FALSE;   
            stStart = (pDat - 1)->stTime;   
            printf("next step\r\n");   
        }   
 
        // Free memory   
        retcode = TLGFreeMemory(g_pTlgData);   
        if (retcode != TRUE)   
            printf("ExportArchiveToCSV: Error in TLGFreeMemory. %s\r\n",error.szErrorText);   
    }   
    while (g_TLGdwNumberOfData >= 10000);   
    //Close file   
    fclose(LogFile);   
    return 0;   
} 

