#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#include <fcntl.h>

#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>

/* Extremos del pipe */
#define READ_EXT 0
#define WRITE_EXT 1

/* Resultado de los journalctl */
#define FILE_NAME "build/results.txt"

static int fd_pipe[2],fd_pipe1[2],fd_pipe2[2];
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

/* monlog -s servicio1,servicio2 -t 1 */
void print_help(char *command)
{
	fprintf(stderr,"Monitoreo en tiempo real de los archivos logs de un sistema GNU/Linux.\n");
	fprintf(stderr,"uso:\n %s -s <service1,service2> [OPCIONES]\n", command);
	fprintf(stderr," %s -h\n", command);
	fprintf(stderr,"Opciones:\n");
	fprintf(stderr," -h         \t\t\tAyuda, muestra este mensaje\n");
	fprintf(stderr," -t [tiempo]\t\t\tSetea el tiempo de actualizacion (segundos), tiempo debe ser mayor de 0 y por defecto es 1.\n");
	exit(2);
	
}

/* Parsear los servicios */
char **parse_services(char *line, char *delimiter){
	char *token, *saveptr, *saveptr1;
	char *line_copy;
	int i, num_tokens = 0;
	char **services = NULL;
	
	line_copy = (char *) malloc(strlen(line) + 1);
	strcpy(line_copy, line);
	
	token = strtok_r(line_copy, delimiter, &saveptr);
	while(token != NULL){
		token = strtok_r(NULL, delimiter, &saveptr);
		num_tokens++;
	}
	free(line_copy);
	
	if(num_tokens > 0){
		services = (char **) malloc((num_tokens + 1) * sizeof(char **));
		token = strtok_r(line, delimiter, &saveptr1);
		for(i = 0; i < num_tokens; i++){
			services[i] = (char *) malloc(strlen(token)+1);
			strcpy(services[i], token);
			token = strtok_r(NULL, delimiter, &saveptr1);
		}
		services[i] = NULL;
	}
	
	return services;
}

void * exec_journalctl(void *arg){
	char *si;
	si = (char *) arg;
	printf("Service_i %s\n", si);
	fflush(stdout);
	
	int status, pid_current;
	
	pthread_mutex_lock(&mutex); //START Critical section
         
        if (pipe(fd_pipe) == -1){ 
		  perror("ERROR>> En el pipe");
		  exit(EXIT_FAILURE);
        }
        pid_current = fork(); 
        
        switch(pid_current){
        	case -1: perror("ERROR>> En el fork1");
        	break;
        	case 0:
			close(fd_pipe[READ_EXT]);
			dup2(fd_pipe[WRITE_EXT], STDOUT_FILENO);
			close(fd_pipe[WRITE_EXT]);
			/* Llamo mi comando */
			//execlp("/bin/ls", "ls", "-l", NULL);	
			char *argv[6];
			argv[0]="journalctl";
			
			argv[1]="--no-pager";
			argv[2]="-o";
			argv[3]="json-pretty";
			argv[4]="-b";
			argv[5]=NULL;
			//journalctl --no-pager -o json-pretty -b
			execvp(argv[0],argv);        		
        	break;
        	default:
        		close(fd_pipe[WRITE_EXT]);
        		
        		pipe(fd_pipe1);
        		
        		pid_current = fork();
        		if(pid_current == 0){
        			/* En el hijo 2 */
				close(fd_pipe1[READ_EXT]);
				
        			dup2(fd_pipe[READ_EXT], STDIN_FILENO);
        			close(fd_pipe[READ_EXT]);
        			
        			dup2(fd_pipe1[WRITE_EXT], STDOUT_FILENO);
        			close(fd_pipe1[WRITE_EXT]);
        			
        			execlp("/usr/bin/grep", "grep", "\"PRIORITY\" : \"[0-9]\"", NULL);
        			//grep '"PRIORITY" : "[0-9]"'
				
        		}else{
        		        /* En el padre*/
        		        close(fd_pipe1[WRITE_EXT]);
        		        
        		        pipe(fd_pipe2);
        		        
        		        pid_current = fork();
        		        if(pid_current == 0){
        		        /* En el hijo 3*/
        		        close(fd_pipe2[READ_EXT]);
        		        
        		        dup2(fd_pipe1[READ_EXT], STDIN_FILENO);
        		        close(fd_pipe1[READ_EXT]);
        		        
        		        dup2(fd_pipe2[WRITE_EXT], STDOUT_FILENO);
        		        close(fd_pipe2[WRITE_EXT]);
        		        
        		        execlp("/usr/bin/sed","sed","s/,//",NULL);
        		        }else{
        		        /* En el padre*/
        		        close(fd_pipe1[READ_EXT]);
        		        close(fd_pipe2[WRITE_EXT]);
        		        
        		        pid_current = fork();
        		        
        		        if(pid_current == 0){
        		        /* En el hijo 4*/
        		        dup2(fd_pipe2[READ_EXT], STDIN_FILENO);
        		        close(fd_pipe2[READ_EXT]);
        		        
        		        execlp("/usr/bin/uniq","uniq","-c",NULL);
        		        }
        		}
        		}
        		
        }
        
        close(fd_pipe2[READ_EXT]);
        
        wait(&status);
        wait(&status);
        wait(&status);
        wait(&status);
        
        pthread_mutex_unlock(&mutex); //END Critical section
        
        return 0;
}

int main(int argc, char **argv){
	int opt,c,i,j;
	int seconds=1;
	char *services_par=NULL;
	char **services;
	
	/* Variables globales de getopt */
  	extern char* optarg;
  	extern int optind;
  	extern int optopt;
  	extern int opterr;
  	opterr = 0;
  	
  	/* Determinar los argumentos */
	while((c = getopt(argc, argv, "s:ht:"))!=-1){
		switch(c){
			case 's':
				services_par=optarg;
				break;
			case 't':
				seconds = atoi(optarg);
				break;
			case 'h':
				print_help(argv[0]);
				break;
			case '?':
				if(optopt == 's'){
					fprintf(stderr, "Option %c requiere un argumento!!!\n", optopt);
				}else if(optopt == 't' && isprint(optopt)){
					printf("seconds por defecto es 1.");
					break;
				}else if(isprint(optopt)){
					fprintf(stderr, "Opcion desconocida '-%c'.\n",optopt);
				}else{
					fprintf(stderr, "ERR>> Caracter desconocido '\\x%x'.\n",optopt);
				}
				print_help(argv[0]);
			
		}		
	}
	
	/* Comprobar los services */
	if(!services_par){
		fprintf(stderr, "Debe especificar al menos 2 services de forma obligatoria!\n");
		print_help(argv[0]);	
	}else{
		printf("services_par: %s\n", services_par);
		services = parse_services(services_par, ",");
		if(services){
			printf("service_1: %s\n", services[0]);
			printf("service_2: %s\n", services[1]);
		}
	}
	
	/* Comprobar el tiempo de actualizacion*/
	if(seconds){
		printf("seconds: %d.\n", seconds);
	}
	
	for(i = optind; i < argc; i++){
		printf("Ficheros que se enviaran %s\n", argv[i]);
	}
	
	printf(">> START\n");
	
	pthread_t hilos[2];
	/* Implementando hilos */
	for(j = 0; j < 2; j++){
		//pthread_t hilo_j;
		
		pthread_create(&hilos[j], NULL, exec_journalctl, services[j]);
	}
	
	/* Esperando hilos */
	for(j = 0; j < 2; j++){
	
		pthread_join(hilos[j], NULL);
	}
	
	printf(">> FIN\n");
	
	return 0;
	
}