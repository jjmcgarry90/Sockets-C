/*
This file was written by Michael J. Donahoo and Kenneth L. Calvert
and can be found in the book "TCP/IP Sockets in C: Practical Guide for
Programmers
*/
#include <stdio.h>  /* for perror() */
#include <stdlib.h> /* for exit() */

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    exit(1);
}
