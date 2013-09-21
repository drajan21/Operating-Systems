



/******************************************************************************************************************************************

FILE NAME: csuc_http_drajan.c
STUDENT NAME:Diana Rajan
COURSE: CSCI 640
PROF. NAME: CHRIS MORRIS

******************************************************************************************************************************************/
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<time.h>
#include<dirent.h>
#include<sys/stat.h>
#include<errno.h>
#include<stdlib.h>
#include "csuc_http.h"

http_header_t *http_header;
int majversion,minversion;
char *uri_path;

/* Function to parse the HTTP versions */
int parseversion(char *version)
{
	int index=0;
	int result;
	char *output;

	/* function converts string to long int*/
	int check=strtol(version,&output,10);
	if(!*output)
	{
		result=1;
	}
	else
	{
		result=0;
	}

	return result;
}

/* Function to display datetime */
int getdatetime()
{
	 time_t rawtime;
        struct tm info;
        char datetime[100];
        time( &rawtime );
        info = *localtime( &rawtime );

        size_t bytes=strftime(datetime,80,"%a, %d %b %Y %X GMT", &info);
        printf("\n%s",datetime);
	return 0;
}

/* Function to display the response */
void displayresponse(http_response_t *http_response)
{
	/* In case of request which is valid and returns OK */
        if(http_response->status.code==200)
        {
                printf("HTTP/%d.%d %d %s",http_response->major_version,http_response->minor_version,http_response->status.code,http_response->status.reason);
                getdatetime();
                printf("\nServer:  CSUC HTTP");
                printf("\nContent-Type: %s",http_response->content_type);
                printf("\nContent-Length: %d",http_response->length);
        }

	/* In all other cases does not print length and file type */
        else
        {
                printf("\nHTTP/1.1 %d %s",http_response->status.code,http_response->status.reason);
                getdatetime();
                printf("\nServer:  CSUC HTTP\n");
        }
}
 
/* Function to Build a response */
int buildresponse(const char *filename,const http_request_t *http_request,const http_status_t *http_status,http_response_t *http_response)
{
	char *file;

	file=strdup(filename);

	char *typeofcontent;

	char *checkfiletype=strtok(file,".");
	char *checkfile;

	if(!checkfiletype)
	{
		typeofcontent="Unknown File Type";
	}

	else
	{
		while(checkfiletype)
		{
			checkfile=strdup(checkfiletype);
			checkfiletype=strtok(NULL,".");
		}
		if(strcmp(checkfile,"html")==0)
		{
			typeofcontent=strdup("text/html");
		}
		else if(strcmp(checkfile,"jpeg")==0)
		{
			typeofcontent="image/jpeg";
		}
		else if(strcmp(checkfile,"png")==0)
		{
			typeofcontent="image/png";
		}
		else
		{
			typeofcontent="Unknown File Type";
		}

	}

	http_response->content_type=strdup(typeofcontent);

	http_response->status.code=http_status->code;

	http_response->status.reason=http_status->reason;

	http_response->major_version=http_request->major_version;

	http_response->minor_version=http_request->minor_version;

	/* Calling function to display the response */
	displayresponse(http_response);

	return;
}

/* Function to parse the Headers */
int parseheaderline(const char *headerline,int headerindex,http_response_t *http_response)
{
	int returnval=0;
	char firstchar;
	char *header=strdup(headerline);

	char *tok=strtok(header,":");

	if(tok)
	{
		strcpy(http_response->headers[headerindex].field_name,tok);

		tok=strtok(NULL,":");
		strcpy(http_response->headers[headerindex].field_value,tok);

		int field_val_length=strlen(http_response->headers[headerindex].field_value);
		firstchar=http_response->headers[headerindex].field_value[0];
		if(firstchar==' ')
			/* Moves pointer by 1 position */
			memmove(http_response->headers[headerindex].field_value,http_response->headers[headerindex].field_value+1,field_val_length);
		returnval=1;
	}
	else
		returnval=0;
	return returnval;
}

/* Function to parse the request */
int parserequest(const char *methodline)
{
	int count=0;	
	int capacity = 10;
	int parsestatus=0;

	char *stringtoparse=strdup(methodline);

	char** result = malloc(capacity*sizeof(*result));

	/* Tokenizing based on the delimiter " " */
	char *tok=strtok(stringtoparse," ");
	result[count]=tok;
	while(tok)
    	{
        	if (count >= capacity)
            	result = realloc(result, (capacity*=2)*sizeof(*result));
        	result[count++] = tok? strdup(tok) : tok;
        	tok=strtok(NULL," ");
    	}

	/* For valid request */
	if(count>=3)
	{

		if(strcmp(result[0],"GET")==0)
		{
			parsestatus=200;

			char * httpversion=result[count-1];
			
			/* Tokenizing the version part to check its validity */
			char *version=strtok(httpversion,"/");

			if(!version)
			{
				parsestatus=400;
			}
			else
			{
				if(strcmp(version,"HTTP")!=0)
				{
					parsestatus=400;
				}
				else
				{
					/* Retrieving major and minor versions */
					char *majorminor=strtok(NULL,"/");

					if(!majorminor)
					{
						parsestatus=400;
					}
					else
					{
						int versionresult;
						majorminor[strlen(majorminor)-1]='\0';
		
						/* Retrieves the major version */
						char *majorversion=strtok(majorminor,".");

						 if(!majorversion)
                                                {
                                                        parsestatus=400;
                                                }
						else
						{
							/* Checks the validity of major version */
							versionresult=parseversion(majorversion);

							if(versionresult==0)
							{
								parsestatus=400;
							}
							else
							{
								majversion=atoi(majorversion);	
	
								/* Retrieves the minor version */
								char *minorversion=strtok(NULL,".");
							 	if(!minorversion)
	                                                	{
                	                                        	parsestatus=400;
                        	                        	}
								else
								{
									/* Checking validity of minor versio */
									versionresult=parseversion(minorversion);

									if(versionresult==0)
									{
										parsestatus=400;
									}
									else
									{
										minversion=atoi(minorversion);
										int pathnumber=count-2;
										int pointer;
										char *path;
										path=(char *)malloc(1001);

										/* if the path is separated by space, concatenating to get the full path */
										for(pointer=1;pointer<=pathnumber;pointer++)
										{
											strcat(path,result[pointer]);
											strcat(path," ");
										}

										path[strlen(path)-1]='\0';
										uri_path=strdup(path);
										free(path);
										parsestatus=200;
 									}
								}
							}
						}
					}
				}
			}
		}
		/* For all methods not implemented */
		else
		{
			parsestatus=501;
		}
	}
	/* In case the request is bad */
	else
	{
		parsestatus=400;
	}

	free(result);
	return parsestatus;	
}


int main(int argc, const char * argv[])
{
	size_t methodlinebytes=1000;
	size_t methodlinebytesread;
	char * methodline;
	int parseresult;

	http_request_t http_request;
	http_request_t *request;
	request=&http_request;

	http_response_t http_response;
	http_response_t *response;
	response=&http_response;

	http_status_t http_status;
	http_status_t *status;
	status=&http_status;

	char *filepaths;
        filepaths=(char *)malloc(1000);
	

	methodline=(char *)malloc(methodlinebytes+1);
	methodlinebytesread=getline(&methodline,&methodlinebytes,stdin);

	size_t headerlinebytes=1000;
	size_t headerlinebytesread;
	char *headerline;

	headerline=(char *)malloc(headerlinebytes+1);
	headerlinebytesread=getline(&headerline,&headerlinebytes,stdin);

	int headercount=0,parseheaderval=0;
	if(headerlinebytesread>1)
	{
		parseheaderval=parseheaderline(headerline,headercount,response);
		headercount++;
	}

	int inputdelim=0;
	
	/* Continues to accepts till user enters 2 returns */
	while(inputdelim<2)
	{
		headerlinebytesread=getline(&headerline,&headerlinebytes,stdin);
		if((*headerline)!=10 && headerlinebytesread>1)
                {
                        parseheaderval=parseheaderline(headerline,headercount,response);
			headercount++;
                }
                else
                {
                        inputdelim++;
                }
		if(headercount==63)
			break;
	}

	if(methodlinebytesread<=1||headercount==0||parseheaderval==0)
	{
	
		http_status.code=400;

		http_status.reason="Bad Request";

		buildresponse(filepaths,request,status,response);

                free(methodline);

                free(headerline);

                exit(EXIT_FAILURE);
	}

	else
	{
		parseresult=parserequest(methodline);
	
		switch(parseresult)
                {
                        case 200: http_status.code=200;
				  http_status.reason="OK";
                                  break;

                        case 400: http_status.code=400;
				  http_status.reason="Bad Request";
                                  break;

                        case 404: http_status.code=404;
				  http_status.reason="Not Found";
                                  break;

                        case 501: http_status.code=501;
				  http_status.reason="Not Implemented";
                                  break;
                }

		if(parseresult==200)
		{
			http_request.major_version=majversion;

			http_request.minor_version=minversion;

			http_request.uripath=uri_path;

			http_request.header_count=headercount-1;

                	filepaths=strdup(http_request.uripath);

                	DIR *dir;
                	FILE *fpread;
                	struct stat stfile;
                	dir=opendir(argv[1]);

                	if(dir==NULL)
                	{
                        	http_status.code=404;
				http_status.reason="Not Found";
                        	buildresponse(filepaths,request,status,response);
                	}

                	else
                	{
                        	char *fullfilepath=malloc(1000);

                        	strcat(fullfilepath,argv[1]);

                        	strcat(fullfilepath,http_request.uripath);

                        	FILE *fpread=fopen(fullfilepath,"r");

				if(fpread==NULL)
                        	{
                                	http_status.code=404;
					http_status.reason="Not Found";
					buildresponse(filepaths,request,status,response);
                        	}

                        	else
                        	{
                                	if(stat(fullfilepath,&stfile)==0)
                                	http_response.length=stfile.st_size;
                                	buildresponse(filepaths,request,status,response);
                                	char readchar;
                                	printf("\n\n");
                                	while  ( ( readchar = fgetc( fpread )) != EOF )
                                	{
                                        	printf( "%c", readchar );
                                	}
                                	printf("\n");
                                	fclose( fpread);
                        	}
			free(fullfilepath);

			}
			free(filepaths);
		}
		else
        	{
                	buildresponse(filepaths,request,status,response);
                	free(methodline);
                	free(headerline);
                	exit(EXIT_FAILURE);
		}
	}
	return(1);
}
