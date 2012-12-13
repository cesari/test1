#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <pwd.h>
#include <dirent.h>
#include <grp.h>
#include <sys/time.h>
#include <time.h>

#define MAX_DIR_PATH 2048
#define MAX_HOST_NAME 256
char cwd[MAX_DIR_PATH];
char host[MAX_HOST_NAME];
struct passwd* user;
struct group* grp;

int alphasort(const void *a, const void *b);

int executecmd(char **incmd);
int getarvg(const char *s, const char *deli, char ***argvp);
void displayprompt();
void forkit(char **cmd);
int fexclude();
char* fileperm(mode_t mode);
void sortfiles(struct dirent **files, int numfiles);

int main(void)
{
	char inbuf[MAX_CANON];
	int len;
	int argnum;
	char **argline;

	while(1)
	{
		displayprompt();
		if(fgets(inbuf,MAX_CANON,stdin) ==NULL)
			continue;
		len = strlen(inbuf);
		if(inbuf[len-1] == '\n')
			inbuf[len-1] = 0;
		argnum = getarvg(inbuf," ",&argline);
		if(argnum<=0)
			continue;
		if(executecmd(argline)==-1)
		{
		forkit(argline);
		}
	}

	return 0;
}

void displayprompt()
{
	user = getpwuid(getuid());
	gethostname(host,MAX_HOST_NAME);
	printf("(%s@%s):",user->pw_name,host);
	if(!strcmp(user->pw_name,"root"))
		printf("#");
	else
		printf("$");
}

int getarvg(const char *s, const char *deli, char ***argvp) 
{
	int error, i , numtokens;
	const char *snew;
	char *t;

	if ((s==NULL) || (deli ==NULL) || (argvp == NULL))
	{
		errno = EINVAL;
		return -1;
	}
	*argvp = NULL;
	snew = s +strspn(s,deli);
	if ((t=malloc(strlen(snew)+1)) == NULL)
		return -1;
	strcpy(t,snew);
	numtokens = 0;
	if(strtok(t,deli) != NULL)
		for (numtokens = 1;strtok(NULL,deli) != NULL ; numtokens++);

	if ((*argvp = malloc((numtokens +1)*sizeof(char *)))==NULL)
	{
		error = errno;
		free(t);
		errno = error;
		return -1;
	}

	if(numtokens ==0)
		free(t);
	else
	{
		strcpy(t,snew);
		**argvp = strtok(t,deli);
		for (i=1; i<numtokens;i++ )
			*((*argvp)+i)= strtok(NULL,deli);
	}
	*((argvp) + numtokens)=NULL;
	return numtokens;
}
	
	
int executecmd(char **chargv)
{
	int forkflag=-1;
	if(strcmp(chargv[0],"exit")==0)
	{
		forkflag=0;
		exit(0);
	}
	else if (strcmp(chargv[0],"pwd")==0)
	{
		forkflag=0;
		if(!getcwd(cwd,MAX_DIR_PATH))
		{
		printf("Unable to display current directory\n");
		}
		else
		{
		printf("%s\n",cwd);
		}
	}
	else if (strcmp(chargv[0],"cd")==0)
	{
	forkflag=0;
	chdir(chargv[1]);
	}
	else if(strcmp(chargv[0],"cp")==0)
	{
	forkflag=0;
		if(chargv[1] == NULL || chargv[2] == NULL)
		{
		printf("Usage: cp <file1> <file2>\n");
		}
		else
		{
		int fd_read;
		int fd_write;
		int nbytes=1;
		char buff[1024];

		fd_read = open(chargv[1], O_RDONLY);
		if(fd_read < 0)
		{
		printf("Failed to open: %s\n",chargv[1]);
		exit(0);
		}

		fd_write = open(chargv[2],O_CREAT | O_RDWR | O_TRUNC,
				S_IRWXU | S_IRGRP | S_IROTH);
		
		if(fd_write < 0)
		{
		printf("Failed to open: %s\n",chargv[2]);
		exit(0);
		}
		
		while(nbytes != 0)
		{
		nbytes = read(fd_read, buff, 30);
		nbytes = write(fd_write, buff, nbytes);
		}

		close(fd_read);
		close(fd_write);
		}
	}
	else if(strcmp(chargv[0],"ls")==0)
	{
		forkflag=0;
		int i;
		struct dirent **files;
		struct stat fs;
		struct tm *timestr;
		char time[32];
		int count=0;
		
		int L=0;
		int A=0;
		int T=0;
		
		if( !(chargv[1] == 0) )
		{
		if(strrchr(chargv[1],'l'))
			L=1;
		if(strrchr(chargv[1],'a'))
			A=1;
		if(strrchr(chargv[1],'t'))
			T=1; 
		
			if( !(chargv[2] == 0) )
			{
			if(strrchr(chargv[2],'l'))
                	        L=1;
                	if(strrchr(chargv[2],'a'))
                	        A=1;
                	if(strrchr(chargv[2],'t'))
                	        T=1;
			
				if( !(chargv[3] == 0) )
				{
				if(strrchr(chargv[3],'l'))
                        		L=1;
                		if(strrchr(chargv[3],'a'))
                		        A=1;
               			if(strrchr(chargv[3],'t'))
               			        T=1;
				}
			}
		}
		if(L==1)
		{
			if(A==1)
				count = scandir(".",&files,NULL,alphasort);
			else
				count = scandir(".",&files,fexclude,alphasort);
			if(T==1)
				sortfiles(files,count);			
			
			for(i=0; i < count; i++)
                                {
                                        stat(files[i]->d_name,&fs);
                                        user=getpwuid(fs.st_uid);
                                        grp=getgrgid(fs.st_gid);
                                        timestr=localtime(&fs.st_mtime);
                                        strftime(time,sizeof(time),"%t %b %d %R %Y ",timestr);
                                        printf("%s %u %s %s %u %s %s\n",
                                                fileperm(fs.st_mode),
                                                fs.st_nlink,
                                                user->pw_name,
                                                grp->gr_name,
                                                fs.st_size,
                                                time,
                                                files[i]->d_name);
                                }

		}
		else if(A==1)
		{
			count = scandir(".",&files,NULL,alphasort);
			if(T==1)
				sortfiles(files,count);
			for(i=0; i < count; i++)
                                printf("%s  ",files[i]->d_name);
		}
		else if(T==1)
		{
			count = scandir(".",&files,fexclude,alphasort);
			sortfiles(files,count);
			for(i=0; i < count; i++)
                                printf("%s  ",files[i]->d_name);
		}
		else
		{
			count = scandir(".",&files,fexclude,alphasort);
			for(i=0; i < count; i++)
				printf("%s  ",files[i]->d_name);
		}
		printf("\n");
	}	
	
return(forkflag);
}

void forkit(char **chargv)
{
	int pid, status;

	pid = fork();

	if(pid == 0)
	{
	if(execvp(chargv[0],chargv)<0)
		{
		printf("command not found\n");
		exit(1);
		}
	}	
	else
		while(wait(&status) != pid);	
}

int fexclude(struct dirent *entry)
{
	if( (strcmp(entry->d_name,".")==0) ||
	    (strcmp(entry->d_name,"..")==0) ||
	    (strspn(entry->d_name,".")==1) )
	{
		return(0);
	}	 
	else
		return(1);
}

char* fileperm(mode_t mode)
{
int i;
char *p;
static char perms[10];

p = perms;
strcpy(perms, "---------");

for (i=0; i < 3; i++) {
	if (mode & (S_IREAD >> i*3))
		*p = 'r';
       	p++;

        if (mode & (S_IWRITE >> i*3))
      		*p = 'w';
        p++;

        if (mode & (S_IEXEC >> i*3))
       		*p = 'x';
        p++;
}

static char fperms[11];
fperms[0]='-';
	if(S_ISDIR(mode))
		fperms[0]='d';
for(i=1; i < 11; i++)
	fperms[i]=perms[i];

return(fperms);
}

void sortfiles(struct dirent **files, int numfiles)
{
	int i,j;
	struct stat fsa;
	struct stat fsb;
	struct dirent *temp;
	numfiles--;	
	for(i=0; i < numfiles; i++)
	{
		for(j=0; j < numfiles-i; j++)
		{
		stat(files[j+1]->d_name,&fsa);
		stat(files[j]->d_name,&fsb);
		//printf("%s compard with %s\n", files[j+1]->d_name, files[j]->d_name);
		//printf("%i > %i\n", fsa->st_mtime, fsb->st_mtime);
			if(fsa.st_mtime > fsb.st_mtime)
			{
			//printf("swapped\n");
			temp=files[j];
			files[j]=files[j+1];
			files[j+1]=temp;	
			}
		}
	}	
}

