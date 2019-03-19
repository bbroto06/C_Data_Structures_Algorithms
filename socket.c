/*******************************************************
* FILE NAME : http_main.c
* 
* DESCRIPTION : http_main.c contains main function
* 		1. creates http server
*		2. receives request from client
*		3. sends response to client
* 
* DATE		AUTHOR			REASON 
* 8/12/2013	Broto			Project
********************************************************/

/*******************************************************
		HEADER FILE INCLUSIONS
********************************************************/
#include "http_header.h"

/*******************************************************
		GLOBAL VARIABLES
********************************************************/
FILE *fpe = NULL;								/* File pointer to error_file.txt */
FILE *fpt = NULL;								/* File pointer to trace_file.txt */

/*******************************************************
* FUNCTION NAME : main
* 
* DESCRIPTION : main will call 
* 		1. http_signal_handler - when user presses ctrl+c
* 		2. creates TCP connection
*		3. waits for client to connect
*		4. receives request from client
*		5. sends response to client
*  
* ARGUMENTS : 	argc = no of cmd line args passed
* 		argv = pointer to SCHAR array containing
*			cmd line args
* 
* RETURN :	0 on success
*******************************************************/
S32_INT main( 
	S32_INT argc, 							/* no of cmd line args passed */
	SCHAR *argv[] 							/* pointer to char array containing the cmd line args */
	)
{
	fpe = fopen("../bin/error_file.txt", "w");			/* Open error_file.txt in write mode */
	if(NULL == fpe)
	{
		printf("ERROR : (error in opening error log file)\n");
		exit(0);
	}

	fpt = fopen("../bin/trace_file.txt", "w");			/* Open trace_file.txt in write mode */
	if(NULL == fpt)
	{
		printf("ERROR : (error in opening trace log file)\n");
		fclose(fpe);
		exit(0);
	}

	if(argc > 3)
	{
		ERROR(MAJOR_ERROR, INVALID_NUM_ARGS);
		fclose(fpe);	
		fclose(fpt);	
		exit(0);
	}

	TRACE(BRIEF_TRACE, ("main : start :: no. of args = %d\n", argc));
	fprintf(fpt, "main : start :: no. of args = %d\n", argc);

	signal(SIGINT, http_signal_handler);				/* calling signal handler if ctrl+c is encountered */

	if(NULL == argv[1])						/* Port number not specified */
	{
		ERROR(CRITICAL_ERROR, NO_PORT_NUM);
		fclose(fpe);	
		fclose(fpt);	
		return FAILURE;
	}
	else if( (atoi(argv[1]) < 2000) || (atoi(argv[1]) > 65535) )	/* Port number out of range */
	{
		ERROR(CRITICAL_ERROR, PORT_OUT_OF_RANGE);
		fclose(fpe);	
		fclose(fpt);	
		return FAILURE;
	}
	else
	{
		TRACE(DETAILED_TRACE, ("main : server's port no = %d\n", atoi(argv[1])));
		fprintf(fpt, "main : server's port no = %d\n", atoi(argv[1]));
	}

	if(NULL == argv[2])						/* No port number provided */
	{
		printf("Tracing flag is set to 1 by default (BRIEF_TRACE)\n");
		printf("To avoid by default trace set pass the trace level in command line arg 2\n");
		http_set_trace_level(1);				/* BRIEF_TRACE by default */
	}
	else if( atoi(argv[2]) < 0 )
	{
		printf("Tracing flag set to minimum of 0 (NO_TRACING)\n");
		http_set_trace_level(0);				/* NO_TRACE */
	}
	else if(atoi(argv[2]) > 2)
	{
		printf("Tracing flag set to maximum of 2\n");
		http_set_trace_level(2);				/* Set the tracing level to DETAILED_TRACING */
	}
	else
	{
		printf("Tracing flag set to %d \n", atoi(argv[2]));
		http_set_trace_level(atoi(argv[2]));			/* Set the tracing level as per user's choice */
		
	}

	server_return_et ret_http_read_data = FAILURE;			/* return value of read_data() */	
	server_return_et ret_http_init_server = FAILURE; 		/* return value of http_init_server() */
	server_return_et ret_select = FAILURE; 				/* return value of select() */
	SCHAR inet_add[MAX_50];

/***************** USING SELECT CALL START ***************************/
	fd_set master; 							/* master file descriptor list */
	fd_set read_fds; 						/* temp file descriptor list for select() */
	struct sockaddr_in myaddr; 					/* server address */
	struct sockaddr_in remoteaddr; 					/* client address */
	socklen_t addrlen;						/* length of remoteaddr */

	S32_INT fdmax; 							/* maximum file descriptor number */
	SCHAR buf[MAX_CHR]; 						/* buffer for client data */
	S32_INT nbytes; 						/* no. of bytes received from client */
	S32_INT i;							/* temp variable */
	S32_INT PORT = atoi(argv[1]);					/* port number of server */

	FD_ZERO(&master); 						/* clear the master and temp sets */
	FD_ZERO(&read_fds);

	ret_http_init_server = http_init_server(PORT,&myaddr);  	/* function call to start the server */
	if (FAILURE == ret_http_init_server)
	{
		
		ERROR(CRITICAL_ERROR, ERROR_STARTING_SERVER );
		fclose(fpe);
		fclose(fpt);
                return FAILURE;
	}

	FD_SET(g_listener, &master);					/* add the listener to the master set */
	fdmax = g_listener; 						/* keep track of the biggest file descriptor */
	memset(&remoteaddr,ZERO,sizeof(remoteaddr));
	for(;;) 							/* main loop of server */
	{
		printf("\n\nI am waiting \n");
		printf("My port number is : %d \n\n", atoi(argv[1]) );
		read_fds = master;
		ret_select = select(fdmax+1, &read_fds, NULL, NULL, NULL);	/* function call to SELECT */ 
		if (FAILURE == ret_select)
		{
		        ERROR(CRITICAL_ERROR, ERROR_IN_SELECT );
			fclose(fpe);
			fclose(fpt);
			return FAILURE;
		}

		for(i = ZERO; i <= fdmax; i++) 			/* run through the existing connections looking for data to read */
		{
			if (FD_ISSET(i, &read_fds)) 
			{ 
				if (i == g_listener) 			/* new connection available */
				{
					addrlen = sizeof(remoteaddr);
					g_newfd = accept(g_listener, (struct sockaddr *)&remoteaddr,&addrlen);
					if (FAILURE == g_newfd)
					{
						ERROR(CRITICAL_ERROR, ERROR_IN_ACCEPT);
					}
					else 
					{
						FD_SET(g_newfd, &master); /* add to master set */
						if (g_newfd > fdmax) 
						{ 
							fdmax = g_newfd; /* keep track of the maximum */
						}
						//printf("selectserver: new connection from %s on "
							//"socket %d\n", inet_ntoa(remoteaddr.sin_addr), g_newfd);
						
						strcpy(inet_add, inet_ntoa(remoteaddr.sin_addr) );
						memset(inet_add, '\0', MAX_50);
						break;
					}
				} /* end of if(i==g_listener) */

				else 					/* handle data from a client */
				{
					memset(buf, '\0', sizeof(buf) );
					nbytes = recv(i, buf, sizeof(buf), ZERO);
					if("\0" == buf)
					{
						close(i); 		/* closing the particular socket */
						FD_CLR(i, &master); 	/* remove from master set */
						break;	
					}
					if (nbytes <= ZERO)
					{
						if (nbytes == ZERO) 	/* connection closed. client hungup */
						{
							printf("selectserver : socket %d hung up\n", i);
						}
						else 			/* connection reset by peer */
						{
							printf("selectserver : connection reset by peer\n");
						}
						close(i); 		/* closing the particular socket */
						FD_CLR(i, &master); 	/* remove from master set */
					}
					else
					{
						ret_http_read_data = http_read_data(buf);	/* function call to read data */
						if(FAILURE == ret_http_read_data)
						{
							ERROR(MAJOR_ERROR, READ_DATA_ERROR);
						}
					}
				} /* end of else */
			} /* end of if FD_ISSET */
		} /* end of for */
		printf("FINISHED SERVING ONE CLIENT\n");
	} /* end of main loop of server */
/***************** USING SELECT CALL END ***************************/
	TRACE(BRIEF_TRACE, ("main : end\n"));
	fprintf(fpt, "%s\n", "main : end");
	return SUCCESS;
}

