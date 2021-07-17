#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <getopt.h>

#include <pthread.h>

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


int main(int argc, char **argv){
	int opt,c,i;
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
				printf("S\n");
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
	
	printf(">> START");
	
	return 0;
	
}