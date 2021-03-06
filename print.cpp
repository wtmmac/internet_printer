/********************************************
*FileName: print.cpp
*Date: 2013年 05月 09日 星期四 16:00:22 CST
*Author: chenhuan
*Usage: define the class of Print
********************************************/

#include "print.h"

//============================================
/**
*run ro run the main flow of Print
*1. get address info of printd
*2. connect to the printd
*3. submitFile to the printd
*4. get the return information from printd
*/
void Print::run()
{
  int sockFd; 
  char *printdName;
  
  if(checkFile(fileName_) != 0)
    error("error in checkFile");

  if((printdName = getPrintdName()) == NULL)
    error("error in scanConfigFile");

  if((sockFd = makeTcpConnect(printdName , PRINTD_PORT)) < 0)
    error("error in makeConenctToPrintd");

  if(sendPrintRequest(sockFd) < 0)
    error("error in sendPrintRequest");

  if(submitFile(sockFd) < 0)
    error("error in submitFile");

  if(receivePrintReply(sockFd) < 0)
    error("error in receivePrintReply");

  close(sockFd);
}

//============================================
/**
*build the PrintRequest and send it to printd
*return value : 0 = success
*/
int Print::sendPrintRequest(int sockFd)
{
  struct PrintRequest printRequest;
  struct stat fileInfo;
  struct passwd *pwd;
  int length;

  /**********build the printRequest**************/
  if(stat(fileName_ , &fileInfo) != 0)  //size
    error("error in sendPrintRequest::stat");
  printRequest.size_ = htonl(fileInfo.st_size);

  if((pwd = getpwuid(geteuid())) == NULL)  //user name
    strcpy(printRequest.userName_ , "unknow");
  else
  {
    strcpy(printRequest.userName_ , pwd->pw_name);
    if(strlen(pwd->pw_name) > HOST_NAME_MAX)
      error("error in strcpy");
  }

  strcpy(printRequest.fileName_ , fileName_);  //file name
  if(strlen(fileName_) > SIMPLE_SIZE)
    error("error in strcpy");

  if(textFormat_ == TEXT)
    printRequest.flags_ = htonl(PLAIN_TEXT);  //flags
  else
    printRequest.flags_ = 0;

  //send printRequest and receive reply to conform
  if((length = writen(sockFd , &printRequest , sizeof(printRequest)))
                      != sizeof(printRequest))
    error("error in writen");

  return 0;
}

//==================================================
/**
*submitFile to submit the pointed file to printd
*sockFd : the socket to sumbit
*return value : 0 = operate success
*/
int Print::submitFile(int sockFd)
{
  FILE *fp;
  FILE *sockFp;
  int c;

  //sumbit the file to printd
  if((fp = fopen(fileName_ , "r")) == NULL)
    error("error in submitFile::fopen");

  if((sockFp = fdopen(sockFd , "w")) == NULL)
    error("error in submitFile::fdopen");

  while((c = getc(fp)) != EOF)
    fputc(c , sockFp);

  fputc(END_SIGN , sockFp);
  fflush(sockFp);
  fclose(fp);

  return 0;
}

//===============================================
/**
*receive the print reply from the printd
*return value : 0 = true
*/
int Print::receivePrintReply(int sockFd)
{
  struct PrintReply printReply;

  if(readnTime(sockFd , &printReply , sizeof(printReply) , WAIT_TIME) 
     != sizeof(printReply))
    error("error in receivePrintReply::readn");

  //need to analysis the PrintReply

  return 0;
}

//============================================
/**
*check if file exist and it is a regular file
*return value : 0 = success
*/
int Print::checkFile(char *fileName)
{
  struct stat fileInfo;

  //check if the file exist
  if(stat(fileName_ , &fileInfo) < 0)
  {
    perror("file is not exist");
    return -1;
  }
  if(!S_ISREG(fileInfo.st_mode))
  {
    perror("is not a regular file");
    return -1;
  }
  return 0;
}

//============================================
/**
*getPrintdName to get the printd name in print.conf
*format : printd name
*return value : 0 = success
*/
char *Print::getPrintdName()
{ 
  return scanConfigFile(CONFIG_FILE, PRINTD_NAME);
}

//==================================================

int main(int ac , char *av[])
{
  if(ac < 2)
    error("Usage: command [fileName]");

  int c;
  int textFormat = 0; 

  while((c = getopt(ac , av , "t")) != -1)
  {
    switch(c)
    {
      case 't' : textFormat = TEXT ; break;
      case '?' : break;
    }
  }

  Print print(av[optind] , textFormat);
  print.run();

  return 0;
}
