#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>

#define MaxLen 1000
#define MaxArgLen 100

int GetStringInRightFormat(char *str){
    char c;
    int flag = 0, i = 0;
    scanf("%c", &c);
    while(c == ' '){
        scanf("%c", &c);
    }
    if(c == '\n'){
        return 0;
    }
    while(c != '\n'){
        if(c != ' '){
            *(str + i) = c;
            flag = 0;
            i++;
        }
        else if(!flag){
            *(str + i) = c;
            flag = 1;
            i++;
        }
        scanf("%c", &c);
    }
    if(*(str + i - 1) == ' '){
        *(str + i - 1) = '\0';
    }
    else{
        *(str + i) = '\0';
    }
    return 1;
}

int ReadArgs(char *str,int *cur, char **arg){
	int i = *cur, j, k = 0;
	arg[k] = malloc(sizeof(char)*MaxArgLen);
	while(str[i] == ' ')
		i++;
	while((str[i] != '&') && (str[i] != ';') && (str[i] != '|') && (str[i] != '\0') 
	&& (str[i] != '<') && (str[i] != '>') && (str[i] != ')')){
		j = 0;
		while((str[i] != ' ') && (str[i] != '&') && (str[i] != ';') && (str[i] != '|') && (str[i] != '\0') 
		&& (str[i] != '<') && (str[i] != '>') && (str[i] != ')')){
			*(arg[k] + j) = str[i];
			i++;
			j++;
		}
		*(arg[k] + j) = '\0';
		k++;
		if((str[i] == ' ') && (str[i + 1] != '&') && (str[i + 1] != ';') && (str[i + 1] != '|') && (str[i + 1] != '\0') 
		&& (str[i + 1] != '<') && (str[i + 1] != '>') && (str[i + 1] != ')')){
			arg[k] = malloc(sizeof(char)*MaxArgLen);
			i++;
		}
		else if(str[i] == ' '){
			i++;
			arg[k] = NULL;
		}
		else{
			arg[k] = NULL;
		}
	}
	return k;
}

void GetFileName(char *str, int i, char *buf){
	while(str[i] == ' ')
		i++;
	int j = 0;
	while((str[i] != '&') && (str[i] != ';') && (str[i] != '|') && (str[i] != '\0') 
	&& (str[i] != '<') && (str[i] != '>') && (str[i] != ')') && (str[i]) != ' '){
		*(buf + j) = str[i];
		i++;
		j++;
	}
	*(buf + j) = '\0';
}

int Execute(char *str, int *cur){
	int ok, i, status;
	char *arg[10];
	i = ReadArgs(str, cur, arg); 
	if (fork() == 0){
		execvp(arg[0], arg);
		while(i >= 0){
			free(arg[i]);
			i--;
		}
		exit(1);
	}
	wait(&status);
	ok = WEXITSTATUS(status) == 0;
	while(i >= 0){
		free(arg[i]);
		i--;
	}
	return ok;
}

int Conveyor(char *str, int *cur){
	int ok = 1, i = *cur, count = 0, status, connector;
	int fd[2];
	while((str[*cur] != '&') && !((str[*cur] == '|') && (str[*cur + 1] == '|')) && (str[*cur] != '\0') && (str[*cur] != ';') && (str[*cur] != ')')){
		pipe(fd);
		if(fork() == 0){
			i = *cur;
			if(count){
				dup2(connector, 0);
				close(connector);
			}
			while((str[i] != '&') && (str[i] != '|') && (str[i] != '\0') && (str[i] != ';'))
				i++;
			if((str[i] == '|') && (str[i + 1] != '|'))//есть ли еще команды
				dup2(fd[1], 1);
			close(fd[1]);
			close(fd[0]);
			ok = Execute(str, cur);
			if(ok){
				exit(0);
			}
			else{
				exit(1);
			}
		}
		else{
			connector = fd[0];
			close(fd[1]);
			while((str[i] != '&') && (str[i] != '|') && (str[i] != '\0') && (str[i] != ';'))
				i++;
			if((str[i] == '|') && (str[i + 1] != '|')){//есть ли еще команды
				count++;
				i++;
			}
			*cur = i;
		}
	}
	while(count >= 0){
		waitpid(-1, &status, WUNTRACED);
		count--;
	}
	ok = WEXITSTATUS(status) == 0;
	return ok;
}

void Redirection(char *str, int *cur){
	int i = *cur;
	char *buf = malloc(sizeof(char)*MaxArgLen);
	if((str[i] == '>') && (str[i + 1] == '>')){
		i += 2;
		GetFileName(str, i, buf);
		int f = open(buf, O_WRONLY|O_CREAT|O_APPEND, 0666);
		dup2 (f, 1);
		close(f);
	}
	else if (str[i] == '>'){
		i ++;
		GetFileName(str, i, buf);
		int f = open(buf, O_WRONLY|O_CREAT|O_TRUNC, 0666);
		dup2 (f, 1);
		close(f);
	}else if(str[i] == '<'){
		i ++;
		GetFileName(str, i, buf);
		int f = open(buf, O_RDONLY);
		dup2 (f, 0);
		close(f);
	}
	free(buf);
}

int ShellCommand(char *str, int *cur);

int Command(char *str, int *cur){
	int ok, status, i;
	while(str[*cur] == ' ')
		(*cur)++;
	if(str[*cur] == '('){
		(*cur)++;
		ok = ShellCommand(str, cur);
		return ok;
	}
	else{ 
		int fd1[2];
		pipe(fd1);
		if(fork() == 0){
			close(fd1[0]);
			if((str[*cur] == '>') || (str[*cur] == '<')){
				Redirection(str, cur);
				*cur = *cur + 2;
				if(str[*cur] == ' ')
					(*cur)++;
				while(str[*cur] != ' ')
					(*cur)++;
				(*cur)++;
				if((str[*cur] == '>') || (str[*cur] == '<')){
					Redirection(str, cur);
					*cur = *cur + 2;
					if(str[*cur] == ' ')
						(*cur)++;
					while(str[*cur] != ' ')
						(*cur)++;
					(*cur)++;
				}
			}
			ok = Conveyor(str, cur);
			write(fd1[1], cur, sizeof(int));
			close(fd1[1]);
			if(ok){
				exit(0);
			}
			else{
				exit(1);
			}
		}
		else{
			wait(&status);
			ok = WEXITSTATUS(status) == 0;
			read(fd1[0], cur, sizeof(int));
			close(fd1[0]);
			close(fd1[1]);
		}
	}
	return ok;
}

int ConditionalCommand(char *str, int *cur){
	int ok;
	while(!((str[*cur] == '&') && (str[*cur + 1] != '&')) && (str[*cur] != ';') && (str[*cur] != '\0') && (str[*cur] != ')')){
		ok = Command(str, cur);
		if((str[*cur] == '&') && (str[*cur + 1] == '&')){//успех -> успех
			*cur += 2;
			if(!ok){
				while(!((str[*cur] == '&') && (str[*cur + 1] != '&')) && (str[*cur] != ';') && (str[*cur] != '\0'))
					(*cur)++;
			}
		}
		else if((str[*cur] == '|') && (str[*cur + 1] == '|')){//успех -> неуспех
			*cur += 2;
			if(ok){
				while(!((str[*cur] == '&') && (str[*cur + 1] != '&')) && (str[*cur] != ';') && (str[*cur] != '\0'))
					(*cur)++;
			}
		}
	}
	return ok;
}

int ShellCommand(char *str, int *cur){
	int i, ok;
	while((str[*cur] != '\0') && (str[*cur] != ')')){
		i = *cur;
		while(!((str[i] == '&') && (str[i + 1] != '&')) && (str[i] != ';') && (str[i] != '\0') && (str[i] != ')')){
			if(str[i] == '&'){
				i++;
			}
			else if(str[i] == '('){
				while(str[i] != ')')
					i++;
			}
			i++;
		}
		if(str[i] == '&'){//фоновый режим
			if(fork() == 0){
				signal(SIGINT, SIG_IGN);
				int f = open("/dev/null", O_RDONLY);
				dup2(f, 0);
				ok = ConditionalCommand(str, cur);
				close(f);
				exit(0);
			}
			*cur = i + 1;
		}
		else{//последовательный режим
			ok = ConditionalCommand(str, cur);
			if((str[i] == '\0') || (str[i] == ')')){
				*cur = i;
			}
			else{
				*cur = i + 1;
			}
		}
	}
	return ok;
}

int main(){
	char MainCommand[MaxLen];
	int cur;
	while(1){
        printf("Enter command: ");
		if(!GetStringInRightFormat(MainCommand)){
            printf("Empty command\n");
            return 0;
        }
		cur = 0;
		ShellCommand(MainCommand, &cur);
	}
	return 0;
}