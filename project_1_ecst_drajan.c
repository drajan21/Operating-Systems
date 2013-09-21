/*
File Name: project_1_ecst_drajan.c
File Description: Program to display list of files in a directory with file permissions
                  ;similar to the ls -la command in UNIX
Date: 09/05/2013
*/

#include<stdio.h>
#include<dirent.h>
#include<sys/stat.h>
#include<string.h>
#include<stdlib.h>
#include<errno.h>

int  main(int argc, char*argv[])
{
	char *path;       
	int iLength;
	if(argc>1)
	{
		path=argv[1];
	}

	/*If no input provided, consider root directory*/
	else 
	{
		path="./";		
	}
	struct stat statpoint;
	DIR *dir;
	struct dirent *point;
	char cLastChar;
	iLength=strlen(path);
	/*Obtain Last character of directory*/
	cLastChar=*(path+(iLength-1));
	if(cLastChar!='/')
	{
		strcat(path,"/");
	}
	static short typesarray[3]={S_IFDIR,S_IFREG,S_IFLNK};
	static char typedenote[3]="d-l";
	static short permarray[9]={S_IRUSR,S_IWUSR,S_IXUSR,S_IRGRP,S_IWGRP,S_IXGRP,S_IROTH,S_IWOTH,S_IXOTH};	
	static char permissions[10]="rwxrwxrwx";
	char increment[10];
	int iVar,jVar;
	/*Make System call to open directory*/
	dir=opendir(path);
	/*Capturing errors*/
	if(dir==NULL)
	{
		fprintf(stderr,"%s\n",strerror(errno));
	}
	else
	{
		/*Make system call to read directory*/
		while((point=readdir(dir))!=NULL)
		{
			char *FullPath=malloc(iLength+strlen(point->d_name)+1);
			strcat(FullPath,path);
			strcat(FullPath,point->d_name);
			if(stat(FullPath,&statpoint)!=-1)
			{
				if(statpoint.st_mode &typesarray[0])
					increment[0]=typedenote[0];
				else if(statpoint.st_mode &typesarray[1])
					increment[0]=typedenote[1];
				else if(statpoint.st_mode & typesarray[2])
					increment[0]=typedenote[2];
				for(iVar=0;iVar<9;iVar++)
				{
					jVar=iVar+1;
					if(statpoint.st_mode & permarray[iVar])
						increment[jVar]=permissions[iVar];
					else
						increment[jVar]='-';
				}
				increment[10]='\0';

			}
			printf("%c%s%c  %s\n",'[',increment,']', point->d_name);
			free(FullPath);
		}
		closedir(dir);
	}
	return(1);
}
