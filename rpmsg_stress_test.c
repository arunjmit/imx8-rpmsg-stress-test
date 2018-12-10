/*
 *  Filename          : rpmsg_stress_test.c
 *  Description       : Test the rpmsg with loading data and multi bandwidth
 *  Project Scope     : PSA
 *  Author            : Abdul Hussain (habdul)
 *  Author            : Arun Kumar (abenjam1)
 *  Target Hardware   : IMX8
 */

#include<stdio.h>
#include<fcntl.h>
#include <unistd.h>
#include <string.h>
#include <termios.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include <error.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <math.h>
#include <getopt.h>

#include "rpmsg_stress_test.h"

/* Open the rpmsg tty device file and initiate tty configuration 
 * and return file descriptor
 */

int init_uart(char *dev, struct termios *ti)
{
  int fd;

  fd = open(dev, O_RDWR |  O_NOCTTY);
  if (fd < 0) {
    perror("Can't open serial port");
    return -1;
  }

  tcflush(fd, TCIOFLUSH);

  if (tcgetattr(fd, ti) < 0) {
    perror("Can't get port settings");
    return -1;
  }

  cfmakeraw(ti);
  printf("Serial port %s opened\n", dev);
  return fd;
}

/*
 * Get the bandwidth MACRO and return Bandwidth as integer value.
 */

int bandwidth_string(speed_t speed)
{
  switch(speed) {
  case B115200:
    return 115200;
  case B230400:
    return 230400;
  case B460800:
    return 460800;
  case B500000:
    return 500000;
  case B576000:
    return 576000;
  case B921600:
    return 921600;
  case B1000000:
    return 1000000;
  case B1152000:
    return 1152000;
  case B1500000:
    return 1500000;
  case B2000000:
    return 2000000;
  case B2500000:
    return 2500000;
  case B3000000:
    return 3000000;
  case B3500000:
    return 3500000;
  case B4000000:
    return 4000000;
  }
}

/* Sets the bandwidth passed as argument
 */

int set_speed(int fd, struct termios *ti, speed_t speed)
{
  cfsetospeed(ti, speed);
  cfsetispeed(ti, speed);
  if (tcsetattr(fd, TCSANOW, ti) < 0) {
    perror("Can't set speed");
    return -1;
  }
  //printf("Speed_set_to:%d,", bandwidth_string(speed));
  return 0;
}

/* Get the current time and converts it to 
 * micro seconds and return
 */

unsigned long getMicrotime(){
  struct timeval currentTime;
  gettimeofday(&currentTime, NULL);
  return currentTime.tv_sec * (int)1e6 + currentTime.tv_usec;
}

int rpmsg_send(int fd, int data_len, char first_byte)
{
  char *data;
  int ret;

  data = (char *)malloc(data_len);
  memset(data, 0xff, data_len);
  data[0] = first_byte;
  ret = write(fd, data, data_len);
  if (ret == -1)
    error(0, errno, "rpmsg write failed");
  free(data);
  return ret;
}

int rpmsg_read(int fd, char *data, int data_len)
{
  int ret;

  do {    
    ret = read(fd, data, data_len);
    if (ret == -1) {
      error(0, errno, "rpmsg read failed");
      return ret;
    } 
    data_len -= ret;
  } while(data_len);

  return data_len;
}


int rpmsg_loop_test(int fd, struct termios *ti){
  
  char *data;
  int length;
  int ret;
  for (int i = 0; i < sizeof(bandwidth)/sizeof(bandwidth[0]); i++) { 

    set_speed(fd, ti, bandwidth[i]);
    
    for (int j = 0; ; j++) {

      /* Geometric progression: 64,128,256... */
      length = START_PACK_LEN * pow(2,j);
      
      if (length > END_PACK_LEN )
	break;

      printf("speed:%d, data_length:%d,time\n",bandwidth_string(bandwidth[i]), length);
      data = (char *)malloc(length);

      for (int k = 0; k < TOTAL_ITER; k++) {

	ret = rpmsg_send(fd, length, k);

	if (ret == -1) {

	  break;

	} else {

	  ret = rpmsg_read(fd, data, length);
	  
	  if (ret == -1) {

	    break;

	  } else {

	    printf(", ,%lu\n",getMicrotime());

	  }
	}
      }
      free(data);
    }
  }
}

void help_print(void)
{
    printf("Usage: rpmsg_stress_test --help \n \
 rpmsg_stress_test --loop <dummy-var> or rpmsg_stress_test -l <dummy-var> \n \
 rpmsg_stress_test --send <dummy-var> or rpmsg_stress_test -s <dummy-var> \n \
 rpmsg_stress_test --receive <dummy-var> or rpmsg_stress_test -r <dummy-var> \n \
\n");
}  

int main(int argc, char *argv[])
{
  int fd;
  int ret;
  int opt_index = 0;
  int opt = 0;
  
  struct termios ti;
  struct option long_options[] = { { "help", 1, 0, 'h' },
				   { "loop", 1, 0, 'l' },
				   { "send", 1, 0, 's' },
				   {"receive", 1, 0, 'r' }, };
  
  if (argc < 2) {
    help_print();
    return 0;
  }
  
  fd = init_uart(RPMSG_DEV, &ti);

  while (1) {
    opt_index = 0;
    opt = getopt_long(argc, argv, "h:l:s:r:",
		      long_options, &opt_index);
    if (-1 == opt)
      break;
    switch (opt) {
    case 'h':
      help_print();
      break;
    case 'l':
      rpmsg_loop_test(fd, &ti);
      break;
    case 's':
      break;
    case 'r':
      break;
    default:
      printf("Invalid argument.\n");
      break;
    }
  }
  close(fd);
  return 0;
} 
